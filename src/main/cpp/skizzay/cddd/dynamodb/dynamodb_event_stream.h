#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/dynamodb/dynamodb_client.h"
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

template <concepts::timestamp Timestamp, std::unsigned_integral Version>
struct event_stream_item_strategy {
  virtual ~event_stream_item_strategy() = default;

  virtual void populate_commit_info(Timestamp timestamp, Version event_version,
                                    Aws::DynamoDB::Model::Put &put) const = 0;
  virtual Aws::DynamoDB::Model::Update
  get_update_version_item(std::size_t const num_items, Timestamp timestamp,
                          Version expected_version) const = 0;

  static inline event_stream_item_strategy const &
  with_ttl(Aws::String timestamp_name, Aws::String version_name,
           Aws::String table_name, Aws::String ttl_name,
           typename Timestamp::duration ttl_value) {
    static struct {
      Aws::String const timestamp_name_;
      Aws::String const version_name_;
      Aws::String const table_name_;
      Aws::String const ttl_name_;
      typename Timestamp::duration const ttl_value_;

      void populate_commit_info(Timestamp timestamp, Version event_version,
                                Aws::DynamoDB::Model::Put &put) const final {
        put.AddItem(timestamp_name_, attribute_value(timestamp))
            .AddItem(version_name_, attribute_value(event_version))
            .AddItem(ttl_name_,
                     attribute_value(
                         std::chrono::time_point_cast<std::chrono::seconds>(
                             timestamp + ttl_value_)));
      }

      Aws::DynamoDB::Model::Update
      get_update_version_item(std::size_t const num_items, Timestamp timestamp,
                              Version expected_version) const final {
        // TODO: clock_cast is missing from GCC 11
        auto expiration = std::chrono::time_point_cast<std::chrono::seconds>(
            timestamp + ttl_value_);
        Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>
            expression_attribute_values = {
                {":inc", attribute_value(num_items)},
                {":ver", attribute_value(expected_version)},
                {":ts", attribute_value(timestamp)},
                {":ttl",
                 attribute_value(std::chrono::sys_seconds{expiration})}};
        Aws::Map<Aws::String, Aws::String> expression_attribute_names = {
            {"#ver", max_version_name_},
            {"#ts", timestamp_name_},
            {"#ttl", ttl_name_}};
        Aws::String update_expression = ;
        return Aws::DynamoDB::Model::Update{}
            .WithTableName(table_name_)
            .WithKey(client_.key(id(), 0))
            .WithUpdateExpression(
                "set #ver = #ver + :inc, #ts = :ts, #ttl = :ttl")
            .WithConditionExpression("#ver = :ver")
            .WithExpressionAttributeNames(expression_attribute_names())
            .WithExpressionAttributeValues(expression_attribute_values());
      }

    } singleton{std::move(timestamp_name), std::move(version_name),
                std::move(table_name), std::move(ttl_name), ttl_value};
    return singleton;
  }

  static inline event_stream_item_strategy const &
  without_ttl(Aws::String timestamp_name, Aws::String version_name) {
    static struct {
      Aws::String const timestamp_name_;
      Aws::String const version_name_;

      void populate_commit_info(Timestamp timestamp, Version event_version,
                                Aws::DynamoDB::Model::Put &put) const final {
        put.AddItem(timestamp_name_, attribute_value(timestamp))
            .AddItem(version_name_, attribute_value(event_version));
      }

    } singleton{std::move(timestamp_name), std::move(version_name)};
    return singleton;
  }
};

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

template <concepts::clock Clock, concepts::domain_event_sequence DomainEvents>
struct impl : event_stream_base<impl<Clock, DomainEvents>, Clock,
                                Aws::DynamoDB::Model::Put, DomainEvents> {
  using base_type =
      event_stream_base<impl<Clock, DomainEvents>, Clock,
                        Aws::DynamoDB::Model::Put, DomainEvents>;
  using typename base_type::buffer_type;
  using typename base_type::element_type;
  using typename base_type::id_type;
  using typename base_type::timestamp_type;
  using typename base_type::version_type;
  using item_type = Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>;

  impl(id_type id, serializer<DomainEvents> &serializer,
       client<id_type, version_type, timestamp_type> &client,
       Aws::String timestamp_name, Aws::String version_name, Clock clock)
      : base_type{std::move(clock)}, id_{id},
        serializer_{serializer}, client_{client},
        strategy_{event_stream_item_strategy<timestamp_type, version_type>::
                      without_ttl(std::move(timestamp_name),
                                  std::move(version_name))} {}

  id_type id() const noexcept { return id_; }

  version_type version() const {
    return parse_version<version_type>(client_.get_version(id()).GetN());
  }

  void commit_buffered_events(buffer_type &&buffer, timestamp_type timestamp,
                              version_type expected_version) {
    auto commit_batch =
        [this, expected_version](std::ranges::input_range auto batch) mutable {
          try {
            client_.commit(Aws::Vector<Aws::DynamoDB::Model::TransactWriteItem>{
                std::move_iterator(std::ranges::begin(batch)),
                std::move_iterator(std::ranges::end(batch))});
          } catch (commit_error<Aws::DynamoDB::DynamoDBError> const &e) {
            switch (e.error()) {
            case Aws::DynamoDB::DynamoDBErrors::CONDITIONAL_CHECK_FAILED:
            case Aws::DynamoDB::DynamoDBErrors::DUPLICATE_ITEM:
            case Aws::DynamoDB::DynamoDBErrors::TRANSACTION_CONFLICT:
              std::throw_with_nested(
                  optimistic_concurrency_collision{e.what(), expected_version});

            default:
              throw;
            }
          }
        };
    auto as_transact_write_item = [](auto &put) {
      return Aws::DynamoDB::Model::TransactWriteItem{}.WithPut(std::move(put));
    };
    auto items_to_commit =
        views::concat(views::single(get_version_write_item(
                          std::size(buffer), timestamp, expected_version)),
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
    strategy_.populate_commit_info(timestamp, event_version, put);
  }

private:
  Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> key() {
    return {{this->config_.key_name(), attribute_value(id())},
            {this->config_.version_name(), attribute_value(0)}};
  }

  void initialize(Aws::DynamoDB::Model::Put &put, std::string_view type) {
    client_.initialize(put, type, id());
  }

  Aws::DynamoDB::Model::Put
  get_starting_version_item(std::size_t const num_items,
                            concepts::timestamp auto timestamp) {
    auto result = Aws::DynamoDB::Model::Put{}.AddItem(
        config_.max_version_name(), attribute_value(num_items));
    initialize(result, version_record_message_type);
    populate_commit_info(timestamp, 0, result);
    return result;
  }

  Aws::DynamoDB::Model::Update
  get_update_version_item(std::size_t const num_items,
                          concepts::timestamp auto timestamp,
                          version_type expected_version) {
    auto const expression_attribute_values = [=, this]() {
      item_type result{{":inc", attribute_value(num_items)},
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
      Aws::Map<Aws::String, Aws::String> result{{"#ver", max_version_name_},
                                                {"#ts", timestamp_name_}};
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
        .WithTableName(C)
        .WithKey(client_.key(id(), 0))
        .WithUpdateExpression(update_expression())
        .WithConditionExpression("#ver = :ver")
        .WithExpressionAttributeNames(expression_attribute_names())
        .WithExpressionAttributeValues(expression_attribute_values());
  }

  Aws::DynamoDB::Model::TransactWriteItem
  get_version_write_item(std::size_t const num_items,
                         concepts::timestamp auto timestamp,
                         version_type expected_version) {
    using Aws::DynamoDB::Model::TransactWriteItem;
    if (0 == expected_version) {
      return TransactWriteItem{}.WithPut(
          get_starting_version_item(num_items, timestamp));
    } else {
      return TransactWriteItem{}.WithUpdate(
          get_update_version_item(num_items, timestamp, expected_version));
    }
  }

  std::remove_cvref_t<id_type> id_;
  Aws::String max_version_name_;
  Aws::String table_name_;
  serializer<DomainEvents...> &serializer_;
  client<id_type, version_type, timestamp_type> &client_;
  event_stream_item_strategy<timestamp_type, version_type> const &strategy_;
};
} // namespace event_stream_details_

template <concepts::clock Clock, concepts::domain_event... DomainEvents>
using event_stream = event_stream_details_::impl<Clock, DomainEvents...>;

} // namespace skizzay::cddd::dynamodb
