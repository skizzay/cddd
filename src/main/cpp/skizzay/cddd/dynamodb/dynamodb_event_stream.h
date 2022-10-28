#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/dynamodb/dynamodb_deser.h"
#include "skizzay/cddd/dynamodb/dynamodb_operation_failed_error.h"
#include "skizzay/cddd/timestamp.h"

#include <algorithm>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/Put.h>
#include <cassert>
#include <ranges>

namespace skizzay::cddd::dynamodb {
namespace event_stream_details_ {
auto set_item_value = [](auto &item, std::string_view key, auto &&value) {
  auto [iterator, is_new_entry] =
      item.emplace(key, attribute_value(std::forward<decltype(value)>(value)));
  if (not is_new_entry) {
    iterator->second = attribute_value(std::forward<decltype(value)>(value));
  }
};

template <concepts::clock Clock, concepts::domain_event... DomainEvents>
struct impl {
  using id_type = id_t<DomainEvents...>;
  using version_type = version_t<DomainEvents...>;
  using item_type = Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>;

  id_type id() const noexcept { return id_; }

  std::string_view key_name() const noexcept { return serializer_.key_name(); }

  std::string_view type_name() const noexcept {
    return serializer_.type_name();
  }

  std::string_view timestamp_name() const noexcept {
    return serializer_.timestamp_name();
  }

  std::string_view table_name() const noexcept {
    return serializer_.timestamp_name();
  }

  template <concepts::domain_event DomainEvent>
  requires(std::same_as<std::remove_cvref_t<DomainEvent>,
                        std::remove_cvref_t<DomainEvents>> ||
           ...) void add_event(DomainEvent &&domain_event) {
    items_.emplace(get_item(std::move(domain_event)));
  }

  void commit_events() {
    auto const timestamp = now(clock_);
    prepend_failed_items_to_commit();
    for (auto commit_request = get_commit_request(timestamp);
         not std::empty(commit_request.GetTransactItems());
         commit_request = get_commit_request(timestamp)) {
      commit(commit_request);
    }
  }

private:
  [[noreturn]] void
  on_optimistic_concurrency_collision(auto const &error_message) {
    version_type const expected_version = starting_version_;
    update_starting_version();
    throw optimistic_concurrency_collision{error_message, starting_version_,
                                           expected_version};
  }

  [[noreturn]] void throw_exception(auto const &error) {
    switch (error.GetCode()) {
    case Aws::DynamoDB::Model::BatchStatementErrorCodeEnum::
        ConditionalCheckFailed:
    case Aws::DynamoDB::Model::BatchStatementErrorCodeEnum::DuplicateItem:
    case Aws::DynamoDB::Model::BatchStatementErrorCodeEnum::TransactionConflict:
      on_optimistic_concurrency_collision(error.GetMessage());
      break;

    default:
      throw operation_failed_error{error};
    }
  }

  void preserve_failed_items(auto b, auto e) {
    failed_items_.reserve(std::size(failed_items_) +
                          std::size(commit_request.GetTransactItems()));
    std::ranges::move(b, e, std::back_inserter(failed_items_));
    items_.erase(b, e);
  }

  [[noreturn]] void on_commit_error(auto b, auto e, auto const &error) {
    preserve_failed_items(b, e);
    throw_exception(error);
  }

  void on_commit_success(auto const b, auto const e,
                         version_type const num_items) {
    starting_version_ += num_items;
    items_.erase(b, e);
  }

  void commit(auto &commit_request) {
    auto const outcome = client_.TransactWriteItems(commit_request);
    auto const b = std::begin(items_);
    auto const e = b + std::size(commit_request.GetTransactItems());
    if (outcome.IsSuccess()) {
      on_commit_success(b, e,
                        narrow_cast<version_type>(
                            std::size(commit_request.GetTransactItems())))
    } else {
      on_commit_error(b, e, outcome.GetError());
    }
  }

  template <concepts::domain_event DomainEvent>
  item_type get_item(DomainEvent &&domain_event) {
    deser_details_::serializer_interface<std::remove_cvref_t<DomainEvent>>
        &serializer = static_cast<deser_details_::serializer_interface<
            std::remove_cvref_t<DomainEvent>> &>(serializer_);
    auto item = serializer.serialize(std::move(domain_event));
    set_item_value(item, serializer_.key_name(), id());
    set_item_value(item, serializer_.version_name(), version(domain_event));
    set_item_value(item, serializer_.type_name(), serializer.message_type());
    return item;
  }

  Aws::DynamoDB::Model::Put get_put() const {
    return Aws::DynamoDB::Model::Put()
        .WithTableName(table_name())
        .WithReturnValuesOnConditionCheckFailure(
            ReturnValuesOnConditionCheckFailure::ALL_OLD)
        .WithConditionExpression("attribute_not_exists(" + key_name() + ")");
  }

  std::unsigned_integral auto num_items_in_commit() const noexcept {
    return std::min(dynamodb_batch_size - 1, std::size(items_));
  }

  auto get_transaction_items(concepts::timestamp auto timestamp) {
    auto as_put = [put = get_put(), timestamp,
                   timestamp_name = serializer_.timestamp_name(),
                   version = starting_version_,
                   version_name = serializer_.version_name()](
                      auto item) mutable -> Aws::DynamoDB::Model::Put {
      set_item_value(item, timestampe_name, timestamp);
      if (serializer_.ttl_attributes().has_value()) {
        auto const &ttl_attributes = serializer_.ttl_attributes().value();
        set_item_value(
            item, ttl_attributes.name,
            std::chrono::time_point_cast<std::chrono::seconds>(timestamp) +
                ttl_attributes.value)
      }
      set_item_value(item, version_name, ++version);
      return put.WithItem(item);
    };
    auto as_transaction_item =
        [](auto &&put) -> Aws::DynamoDB::Model::TransactWriteItem {
      return TransactWriteItem().WithPut(std::forward<decltype(put)>(put));
    };
    return std::ranges::views::all(items_) |
           std::ranges::take(num_items_in_commit()) |
           std::ranges::transform(as_put) |
           std::ranges::transform(as_transaction_item);
  }

  Aws::DynamoDB::Model::Put create_starting_version_record() {
    item_type event_data_item;
    set_item_value(event_data_item, "version",
                   starting_version_ +
                       narrow_cast<version_type>(num_items_in_commit()));
    item_type put_item;
    set_item_value(put_item, serializer_.key_name(), id());
    set_item_value(put_item, serializer_.version_name(), 0);
    set_item_value(put_item, serializer_.type_name(), "version_");
    set_item_value(put_item, serializer_.event_data_name(),
                   std::move(event_data_item));
    return get_put().WithItem(put_item);
  }

  Aws::DynamoDB::Model::Update update_starting_version() {}

  Aws::DynamoDB::Model::TransactWriteItem ensure_consistent_starting_version() {
    if (0 == starting_version_) {
      return TransactWriteItem{}.WithPut(create_starting_version_record());
    } else {
      return TransactWriteItem{}.WithUpdate(update_starting_version());
    }
  }

  Aws::DynamoDB::Model::TransactWriteItemsRequest
  get_commit_request(concepts::timestamp auto timestamp) {
    auto request = get_request_();
    auto add_transaction_item_to_request =
        [&request](auto &&transact_write_item) {
          request.AddTransactItems(
              std::forward<decltype(transact_write_item)>(transact_write_item));
        };

    add_transaction_item_to_request(ensure_consistent_starting_version());
    std::ranges::for_each(get_transaction_items(timestamp),
                          add_transaction_item_to_request);
    return request;
  }

  void prepend_failed_items_to_commit() {
    std::ranges::move(failed_items_, std::begin(items_));
    failed_items_.clear();
  }

  static inline constexpr std::size_t dynamodb_batch_size = 100;
  std::remove_cvref_t<id_type> id_;
  serializer &serializer_;
  Aws::DynamoDB::DynamoDBClient &client_;
  std::function<Aws::TransactWriteItemsRequest()> get_request_;
  std::queue<item_type> items_;
  std::vector<item_type> failed_items_;
  [[no_unique_address]] Clock clock_ = {};
};
} // namespace event_stream_details_

} // namespace skizzay::cddd::dynamodb
