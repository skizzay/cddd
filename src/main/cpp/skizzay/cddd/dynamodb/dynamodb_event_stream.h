#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/dynamodb/dynamodb_deser.h"
#include "skizzay/cddd/dynamodb/dynamodb_operation_failed_error.h"
#include "skizzay/cddd/dynamodb/dynamodb_version_service.h"
#include "skizzay/cddd/factory.h"
#include "skizzay/cddd/optimistic_concurrency_collision.h"
#include "skizzay/cddd/timestamp.h"
#include "skizzay/cddd/version.h"

#include <algorithm>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/Put.h>
#include <aws/dynamodb/model/TransactWriteItemsRequest.h>
#include <cassert>
#include <concepts>
#include <deque>
#include <ranges>

namespace skizzay::cddd::dynamodb {
namespace event_stream_details_ {
template <typename T>
using commit_error = operation_failed_error<commit_failed, T>;

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

  template <concepts::factory<Aws::DynamoDB::Model::TransactWriteItemsRequest>
                CommitRequestFactory = default_factory<
                    Aws::DynamoDB::Model::TransactWriteItemsRequest>>
  impl(serializer<DomainEvents...> &serializer, event_log_config const &config,
       Aws::DynamoDB::DynamoDBClient &client,
       version_service<DomainEvents...> &version_service, Clock clock,
       CommitRequestFactory &&get_request = {})
      : serializer_{serializer}, config_{config}, client_{client},
        version_service_{version_service}, clock_{std::move(clock)},
        get_request_{std::forward<decltype(get_request)>(get_request)} {}

  id_type id() const noexcept { return skizzay::cddd::id(version_service_); }

  template <concepts::domain_event DomainEvent>
  requires(std::same_as<std::remove_cvref_t<DomainEvent>,
                        std::remove_cvref_t<DomainEvents>> ||
           ...) void add_event(DomainEvent &&domain_event) {
    items_.emplace_back(get_item(std::move(domain_event)));
  }

  void commit_events() {
    auto const timestamp = now(clock_);
    for (auto commit_request = get_commit_request(timestamp);
         not std::empty(items_);
         commit_request = get_commit_request(timestamp)) {
      commit(commit_request);
    }
  }

private:
  [[noreturn]] void throw_optimistic_concurrency_collision(
      auto const &error_message) noexcept(false) {
    auto const expected_version = version(version_service_);
    try {
      version_service_.update_version(client_);
    } catch (...) {
      std::throw_with_nested(optimistic_concurrency_collision{
          error_message, static_cast<decltype(expected_version)>(-1),
          expected_version});
    }
    throw optimistic_concurrency_collision{
        error_message, version(version_service_), expected_version};
  }

  [[noreturn]] void throw_exception(auto const &error) noexcept(false) {
    switch (error.GetErrorType()) {
    case Aws::DynamoDB::DynamoDBErrors::CONDITIONAL_CHECK_FAILED:
    case Aws::DynamoDB::DynamoDBErrors::DUPLICATE_ITEM:
    case Aws::DynamoDB::DynamoDBErrors::TRANSACTION_CONFLICT:
      throw_optimistic_concurrency_collision(error.GetMessage());
      break;

    default:
      throw commit_error{error};
    }
  }

  [[noreturn]] void on_commit_error(auto b, auto e, auto const &error) {
    preserve_failed_items(b, e);
    throw_exception(error);
  }

  void commit(auto &commit_request) {
    auto const outcome = client_.TransactWriteItems(commit_request);
    if (outcome.IsSuccess()) {
      auto const num_items = num_items_in_commit();
      auto const b = std::begin(items_);
      version_service_.on_commit_success(num_items);
      items_.erase(b, b + num_items);
    } else {
      throw_exception(outcome.GetError());
    }
  }

  template <concepts::domain_event DomainEvent>
  item_type get_item(DomainEvent &&domain_event) {
    deser_details_::serializer_interface<std::remove_cvref_t<DomainEvent>>
        &serializer = static_cast<deser_details_::serializer_interface<
            std::remove_cvref_t<DomainEvent>> &>(serializer_);
    auto item = serializer.serialize(std::move(domain_event));
    set_item_value(item, config_.key_name(), id());
    set_item_value(item, config_.version_name(), version(domain_event));
    set_item_value(item, config_.type_name(),
                   serializer.message_type(event_type<DomainEvent>{}));
    return item;
  }

  Aws::DynamoDB::Model::Put get_put() const {
    return Aws::DynamoDB::Model::Put()
        .WithTableName(config_.table_name())
        .WithConditionExpression("attribute_not_exists(" + config_.key_name() +
                                 ")");
  }

  std::unsigned_integral auto num_items_in_commit() const noexcept {
    return std::min(dynamodb_batch_size - 1, std::size(items_));
  }

  auto get_transaction_items(concepts::timestamp auto timestamp) {
    auto as_put = [put = get_put(), timestamp, this,
                   version = version(version_service_)](
                      auto item) mutable -> Aws::DynamoDB::Model::Put {
      set_item_value(item, config_.timestamp_name(), timestamp);
      if (config_.ttl().has_value()) {
        auto const &ttl_attributes = config_.ttl().value();
        set_item_value(
            item, ttl_attributes.name,
            std::chrono::time_point_cast<std::chrono::seconds>(timestamp) +
                ttl_attributes.value);
      }
      set_item_value(item, config_.version_name(), ++version);
      return put.WithItem(item);
    };
    auto as_transaction_item =
        [](auto &&put) -> Aws::DynamoDB::Model::TransactWriteItem {
      return Aws::DynamoDB::Model::TransactWriteItem{}.WithPut(
          std::forward<decltype(put)>(put));
    };
    return std::ranges::views::all(items_) |
           std::ranges::views::take(num_items_in_commit()) |
           std::ranges::views::transform(as_put) |
           std::ranges::views::transform(as_transaction_item);
  }

  Aws::DynamoDB::Model::TransactWriteItemsRequest
  get_commit_request(concepts::timestamp auto timestamp) {
    auto request = get_request_();
    auto add_transaction_item_to_request =
        [&request](auto &&transact_write_item) {
          request.AddTransactItems(
              std::forward<decltype(transact_write_item)>(transact_write_item));
        };

    std::ranges::for_each(get_transaction_items(timestamp),
                          add_transaction_item_to_request);
    add_transaction_item_to_request(version_service_.version_record(
        std::size(request.GetTransactItems()), timestamp));
    return request;
  }

  static inline constexpr std::size_t dynamodb_batch_size = 100;
  serializer<DomainEvents...> &serializer_;
  event_log_config const &config_;
  Aws::DynamoDB::DynamoDBClient &client_;
  version_service<DomainEvents...> &version_service_;
  [[no_unique_address]] Clock clock_;
  std::function<Aws::DynamoDB::Model::TransactWriteItemsRequest()> get_request_;
  std::deque<item_type> items_;
};
} // namespace event_stream_details_

template <concepts::clock Clock, concepts::domain_event... DomainEvents>
using event_stream = event_stream_details_::impl<Clock, DomainEvents...>;

} // namespace skizzay::cddd::dynamodb
