#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/dynamodb/dynamodb_deser.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_log_config.h"
#include "skizzay/cddd/dynamodb/dynamodb_operation_failed_error.h"
#include "skizzay/cddd/dynamodb/dynamodb_version_validation_error.h"
#include "skizzay/cddd/event_stream.h"
#include "skizzay/cddd/factory.h"
#include "skizzay/cddd/narrow_cast.h"
#include "skizzay/cddd/optimistic_concurrency_collision.h"
#include "skizzay/cddd/timestamp.h"
#include "skizzay/cddd/version.h"
#include "skizzay/cddd/views.h"

#include <algorithm>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/dynamodb/model/Put.h>
#include <aws/dynamodb/model/TransactWriteItem.h>
#include <aws/dynamodb/model/TransactWriteItemsRequest.h>
#include <aws/dynamodb/model/Update.h>
#include <cassert>
#include <charconv>
#include <concepts>
#include <deque>
#include <limits>
#include <ranges>

namespace skizzay::cddd::dynamodb {
namespace event_stream_details_ {
inline std::string const version_record_message_type = "version_";
inline constexpr std::size_t dynamodb_batch_size = 100;

template <typename T>
using commit_error = operation_failed_error<commit_failed, T>;

template <std::unsigned_integral Version>
Version parse_version(std::string const &version_string) {
  Version parsed_value = std::numeric_limits<Version>::max();
  auto const parse_result = std::from_chars(
      version_string.data(), version_string.data() + version_string.size(),
      parsed_value);
  std::error_code const result_code = std::make_error_code(parse_result.ec);
  if (result_code || (std::numeric_limits<Version>::max() == parsed_value)) {
    throw version_validation_error{
        result_code, "Cannot parse valid version from : " + version_string};
  } else {
    return parsed_value;
  }
}

template <concepts::clock Clock, concepts::domain_event... DomainEvents>
struct impl : event_stream_base<impl<Clock, DomainEvents...>, Clock,
                                Aws::DynamoDB::Model::Put, DomainEvents...> {
  using base_type =
      event_stream_base<impl<Clock, DomainEvents...>, Clock,
                        Aws::DynamoDB::Model::Put, DomainEvents...>;
  using typename base_type::buffer_type;
  using typename base_type::element_type;
  using typename base_type::id_type;
  using typename base_type::timestamp_type;
  using typename base_type::version_type;
  using item_type = Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>;

  template <concepts::factory<Aws::DynamoDB::Model::TransactWriteItemsRequest>
                CommitRequestFactory = default_factory<
                    Aws::DynamoDB::Model::TransactWriteItemsRequest>>
  impl(id_type id, serializer<DomainEvents...> &serializer,
       event_log_config const &config, Aws::DynamoDB::DynamoDBClient &client,
       Clock clock, CommitRequestFactory &&get_request = {})
      : base_type{std::move(clock)}, id_{id},
        serializer_{serializer}, config_{config}, client_{client},
        get_request_{std::forward<decltype(get_request)>(get_request)} {}

  id_type id() const noexcept { return id_; }

  version_type version() const {
    auto extract_version_from_outcome = [this](auto &version_record) {
      auto const max_version_iterator =
          version_record.find(config_.max_version_name());
      return (std::end(version_record) == max_version_iterator)
                 ? 0
                 : parse_version<version_type>(
                       max_version_iterator->second.GetN());
    };
    auto outcome = client_.GetItem(
        Aws::DynamoDB::Model::GetItemRequest{}
            .WithTableName(config_.table_name())
            .WithKey(key())
            .WithProjectionExpression(config_.max_version_name())
            .WithConsistentRead(true));
    if (outcome.IsSuccess()) {
      return extract_version_from_outcome(outcome.GetResult().GetItem());
    } else {
      throw operation_failed_error<std::runtime_error,
                                   Aws::DynamoDB::DynamoDBError>{
          outcome.GetError()};
    }
  }

  void commit_buffered_events(buffer_type &&buffer, timestamp_type timestamp,
                              version_type expected_version) {
    auto commit_batch =
        [this, expected_version](std::ranges::input_range auto batch) mutable {
          auto request = get_request_().WithTransactItems(
              Aws::Vector<Aws::DynamoDB::Model::TransactWriteItem>{
                  std::ranges::begin(batch), std::ranges::end(batch)});
          auto const outcome = client_.TransactWriteItems(request);
          if (!outcome.IsSuccess()) {
            throw_exception(outcome.GetError(), expected_version);
          }
        };
    auto as_transact_write_item = [](auto &put) {
      return Aws::DynamoDB::Model::TransactWriteItem{}.WithPut(std::move(put));
    };
    auto items_to_commit = views::concat(
        views::single(get_version_write_item(timestamp, expected_version)),
        buffer | views::transform(as_transact_write_item));
    std::ranges::for_each(items_to_commit | views::chunk(dynamodb_batch_size),
                          commit_batch);
  }

  template <concepts::domain_event DomainEvent>
  Aws::DynamoDB::Model::Put make_buffer_element(DomainEvent &&domain_event) {
    deser_details_::serializer_interface<std::remove_cvref_t<DomainEvent>>
        &serializer = static_cast<deser_details_::serializer_interface<
            std::remove_cvref_t<DomainEvent>> &>(serializer_);
    auto result = serializer.serialize(std::move(domain_event));
    initialize(result, serializer.message_type(event_type<DomainEvent>{}));
    return result;
  }

  void populate_commit_info(concepts::timestamp auto timestamp,
                            version_type event_version,
                            Aws::DynamoDB::Model::Put &put) {
    put.AddItem(config_.timestamp_name(), attribute_value(timestamp))
        .AddItem(config_.version_name(), attribute_value(event_version));
    if (config_.ttl().has_value()) {
      auto const &ttl_attributes = config_.ttl().value();
      put.AddItem(
          ttl_attributes.name,
          attribute_value(
              std::chrono::time_point_cast<std::chrono::seconds>(timestamp) +
              ttl_attributes.value));
    }
  }

private:
  Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> key() {
    return {{this->config_.key_name(), attribute_value(id())},
            {this->config_.version_name(), attribute_value(0)}};
  }

  void initialize(Aws::DynamoDB::Model::Put &put, std::string_view type) {
    put.WithTableName(config_.table_name())
        .AddItem(config_.key_name(), attribute_value(id()))
        .AddItem(config_.type_name(), attribute_value(type))
        .WithConditionExpression("attribute_not_exists(" + config_.key_name() +
                                 ")");
  }

  Aws::DynamoDB::Model::Put
  get_starting_version_item(concepts::timestamp auto timestamp) {
    auto result = Aws::DynamoDB::Model::Put{}.AddItem(
        config_.max_version_name(), attribute_value(std::size(items_)));
    initialize(result, version_record_message_type);
    populate_commit_info(timestamp, 0, result);
    return result;
  }

  Aws::DynamoDB::Model::Update
  get_update_version_item(concepts::timestamp auto timestamp,
                          version_type expected_version) {
    auto const expression_attribute_values = [=, this]() {
      item_type result{{":inc", attribute_value(std::size(items_))},
                       {":ver", attribute_value(expected_version)},
                       {":ts", attribute_value(timestamp)}};
      if (config_.ttl()) {
        // TODO: clock_cast is missing from GCC 11
        auto expiration = std::chrono::time_point_cast<std::chrono::seconds>(
            timestamp + config_.ttl()->value);
        result.emplace(":ttl",
                       attribute_value(std::chrono::sys_seconds{expiration}));
      }
      return result;
    };
    auto const expression_attribute_names = [this]() {
      Aws::Map<Aws::String, Aws::String> result{
          {"#ver", config_.max_version_name()},
          {"#ts", config_.timestamp_name()}};
      if (config_.ttl()) {
        result.emplace("#ttl", config_.ttl()->name);
      }
      return result;
    };
    auto const update_expression = [this]() {
      return config_.ttl().has_value()
                 ? "set #ver = #ver + :inc, #ts = :ts, #ttl = :ttl"
                 : "set #ver = #ver + :inc, #ts = :ts";
    };
    return Aws::DynamoDB::Model::Update{}
        .WithTableName(config_.table_name())
        .WithKey(key())
        .WithUpdateExpression(update_expression())
        .WithConditionExpression("#ver = :ver")
        .WithExpressionAttributeNames(expression_attribute_names())
        .WithExpressionAttributeValues(expression_attribute_values());
  }

  Aws::DynamoDB::Model::TransactWriteItem
  get_version_write_item(concepts::timestamp auto timestamp,
                         version_type expected_version) {
    using Aws::DynamoDB::Model::TransactWriteItem;
    if (0 == expected_version) {
      return TransactWriteItem{}.WithPut(get_starting_version_item(timestamp));
    } else {
      return TransactWriteItem{}.WithUpdate(
          get_update_version_item(timestamp, expected_version));
    }
  }

  [[noreturn]] void
  throw_exception(auto const &error,
                  version_type const expected_version) noexcept(false) {
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

  std::remove_cvref_t<id_type> id_;
  serializer<DomainEvents...> &serializer_;
  event_log_config const &config_;
  Aws::DynamoDB::DynamoDBClient &client_;
  [[no_unique_address]] Clock clock_;
  std::function<Aws::DynamoDB::Model::TransactWriteItemsRequest()> get_request_;
  std::deque<Aws::DynamoDB::Model::Put> items_;
};
} // namespace event_stream_details_

template <concepts::clock Clock, concepts::domain_event... DomainEvents>
using event_stream = event_stream_details_::impl<Clock, DomainEvents...>;

} // namespace skizzay::cddd::dynamodb
