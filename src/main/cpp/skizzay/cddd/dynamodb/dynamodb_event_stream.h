#pragma once

#include "skizzay/cddd/commit_failed.h"
#include "skizzay/cddd/dynamodb/dynamodb_attribute_value.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_log_config.h"
#include "skizzay/cddd/dynamodb/dynamodb_operation_failed_error.h"
#include "skizzay/cddd/event_stream_buffer.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/narrow_cast.h"
#include "skizzay/cddd/nullable.h"
#include "skizzay/cddd/optimistic_concurrency_collision.h"
#include "skizzay/cddd/version.h"
#include "skizzay/cddd/views.h"

#include <aws/dynamodb/DynamoDBErrors.h>
#include <aws/dynamodb/model/Delete.h>
#include <aws/dynamodb/model/Put.h>
#include <aws/dynamodb/model/TransactWriteItem.h>
#include <aws/dynamodb/model/TransactWriteItemsRequest.h>
#include <optional>
#include <ranges>
#include <string>

namespace skizzay::cddd::dynamodb {
namespace event_stream_details_ {
inline std::string const version_record_message_type = "version_";
inline constexpr std::size_t dynamodb_batch_size = 25;

constexpr void add_standard_field(standard_field const &field,
                                  Aws::DynamoDB::Model::Put &put,
                                  auto &&value) {
  put.AddItem(field.name,
              attribute_value(std::forward<decltype(value)>(value)));
}

constexpr void
add_standard_field(std::optional<time_to_live_field> const &field,
                   Aws::DynamoDB::Model::Put &put,
                   concepts::timestamp auto const &timestamp) {
  if (not skizzay::cddd::is_null(field)) {
    add_standard_field(*field, put,
                       // TODO: Cast this TTL value back to UTC time
                       attribute_value(timestamp + field->duration));
  }
}

inline constexpr auto as_transact_write_item = [](Aws::DynamoDB::Model::Put put)
    -> Aws::DynamoDB::Model::TransactWriteItem {
  return Aws::DynamoDB::Model::TransactWriteItem{}.WithPut(std::move(put));
};

template <typename T>
using commit_error = operation_failed_error<commit_failed, T>;

[[noreturn]] void
throw_exception(auto const &error,
                concepts::version auto const expected_version) noexcept(false) {
  switch (error.GetErrorType()) {
  case Aws::DynamoDB::DynamoDBErrors::CONDITIONAL_CHECK_FAILED:
  case Aws::DynamoDB::DynamoDBErrors::DUPLICATE_ITEM:
  case Aws::DynamoDB::DynamoDBErrors::TRANSACTION_CONFLICT:
    throw optimistic_concurrency_collision{error.GetMessage(),
                                           expected_version};

  default:
    throw commit_error{error};
  }
}

template <typename T>
concept put_range = std::ranges::range<T> &&
    std::same_as<Aws::DynamoDB::Model::Put, std::ranges::range_value_t<T>>;

template <typename Store> struct impl {
  friend Store;

  constexpr void
  commit_events(concepts::identifier auto id_value,
                concepts::version auto const expected_version,
                concepts::event_stream_buffer auto event_stream_buffer) {
    if (not std::ranges::empty(event_stream_buffer)) {
      concepts::timestamp auto const timestamp =
          skizzay::cddd::now(store_->clock());
      auto version_put = get_starting_version(
          expected_version, std::ranges::size(event_stream_buffer), timestamp,
          id_value);
      auto puts_with_commit_info =
          event_stream_buffer |
          views::transform([&](Aws::DynamoDB::Model::Put &put) {
            return add_commit_info(timestamp,
                                   store_->new_condition_expression(), put);
          });
      auto puts =
          views::concat(views::single(version_put), puts_with_commit_info);
      commit_puts_to_dynamodb(puts, expected_version);
    }
  }

  constexpr void rollback_to(concepts::identifier auto const &,
                             concepts::version auto const) {
    // TODO: Query max version record.  If max version record is greater than
    // target version, then delete all records with version greater than target
    // version.  If the target version is zero, then also delete the max version
    // record.
  }

private:
  constexpr impl(Store &store) noexcept : store_{&store} {}

  constexpr Aws::DynamoDB::Model::Put &
  add_commit_info(concepts::timestamp auto const timestamp,
                  std::string const &condition_expression,
                  Aws::DynamoDB::Model::Put &put) {
    add_standard_field(store_->config().timestamp_field(), put, timestamp);
    add_standard_field(store_->config().ttl_field(), put, timestamp);
    return put.WithConditionExpression(condition_expression)
        .WithTableName(store_->config().table().get());
  }

  constexpr Aws::DynamoDB::Model::Put
  get_starting_version(concepts::version auto const expected_version,
                       std::size_t const num_elements,
                       concepts::timestamp auto const timestamp,
                       concepts::identifier auto id_value) {
    std::string const &condition_expression =
        (0 == expected_version) ? store_->new_condition_expression()
                                : store_->update_condition_expression();
    Aws::DynamoDB::Model::Put put;
    put.AddExpressionAttributeValues(
        store_->config().max_version_field().value_expression,
        attribute_value(expected_version + num_elements));
    add_standard_field(store_->config().hash_key(), put, id_value);
    add_standard_field(store_->config().sort_key(), put, std::size_t{0});
    add_standard_field(store_->config().max_version_field(), put,
                       expected_version + num_elements);
    return add_commit_info(timestamp, condition_expression, put);
  }

  void commit_puts_to_dynamodb(put_range auto puts,
                               concepts::version auto const expected_version) {
    auto transactions = puts | views::transform(as_transact_write_item) |
                        views::chunk(dynamodb_batch_size);
    std::ranges::for_each(transactions, [this, expected_version](auto items) {
      commit_transaction(transact_write_items_request(items), expected_version);
    });
  }

  void commit_transaction(auto &&request,
                          concepts::version auto const expected_version) {
    auto const outcome = store_->client().commit(std::move(request));
    if (!outcome.IsSuccess()) {
      throw_exception(outcome.GetError(), expected_version);
    }
  }

  template <std::ranges::input_range Items>
  requires std::same_as<Aws::DynamoDB::Model::TransactWriteItem,
                        std::ranges::range_value_t<Items>>
      Aws::DynamoDB::Model::TransactWriteItemsRequest
      transact_write_items_request(Items &items) {
    Aws::Vector<Aws::DynamoDB::Model::TransactWriteItem> transaction;
    transaction.reserve(dynamodb_batch_size);
    std::ranges::move(std::move(items), std::back_inserter(transaction));
    Aws::DynamoDB::Model::TransactWriteItemsRequest request;
    request.SetTransactItems(std::move(transaction));
    return request;
  }

  Store *store_;
};
} // namespace event_stream_details_

template <typename Store>
using event_stream = event_stream_details_::impl<Store>;

} // namespace skizzay::cddd::dynamodb
