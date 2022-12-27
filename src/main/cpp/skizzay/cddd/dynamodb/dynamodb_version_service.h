#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/dynamodb/dynamodb_attribute_value.h"
#include "skizzay/cddd/dynamodb/dynamodb_deser.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_log_config.h"
#include "skizzay/cddd/dynamodb/dynamodb_operation_failed_error.h"
#include "skizzay/cddd/dynamodb/dynamodb_version_validation_error.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/narrow_cast.h"
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/AttributeValue.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/dynamodb/model/Put.h>
#include <aws/dynamodb/model/TransactWriteItem.h>
#include <aws/dynamodb/model/Update.h>
#include <charconv>
#include <concepts>
#include <system_error>

namespace skizzay::cddd::dynamodb {

struct update_version_failed
    : operation_failed_error<std::runtime_error, Aws::DynamoDB::DynamoDBError> {
  using operation_failed_error::operation_failed_error;
};

namespace version_service_details_ {
inline std::string const version_record_message_type = "version_";
inline std::string const version_field_value = "0";

struct impl {
  using item_type = Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>;
  using names_type = Aws::Map<Aws::String, Aws::String>;

  void update_version(Aws::DynamoDB::DynamoDBClient &client, auto &&key) {
    auto outcome = client.GetItem(Aws::DynamoDB::Model::GetItemRequest{}
                                      .WithTableName(config_.table_name())
                                      .WithKey(std::move(key))
                                      .WithConsistentRead(true));
    if (outcome.IsSuccess()) {
      apply_version(outcome.GetResult().GetItem());
    } else {
      throw update_version_failed{outcome.GetError()};
    }
  }

  void apply_version(item_type const &version_record) {
    auto const max_version_iterator =
        version_record.find(config_.max_version_name());
    version_ = (std::end(version_record) == max_version_iterator)
                   ? 0
                   : parse_version(max_version_iterator->second.GetN());
  }

  Aws::DynamoDB::Model::Put
  create_starting_version_record(int num_items_in_commit, item_type item,
                                 std::chrono::sys_seconds timestamp) const {
    item.emplace(config_.max_version_name(),
                 attribute_value(version_ + num_items_in_commit));
    item.emplace(config_.type_name(),
                 attribute_value(version_record_message_type));
    item.emplace(config_.timestamp_name(),
                 attribute_value(timestamp.time_since_epoch().count()));
    if (config_.ttl()) {
      std::chrono::sys_seconds const expiration =
          timestamp + config_.ttl()->value;
      item.emplace(config_.ttl()->name,
                   attribute_value(expiration.time_since_epoch().count()));
    }
    return Aws::DynamoDB::Model::Put{}
        .WithTableName(config_.table_name())
        .WithItem(std::move(item))
        .WithConditionExpression("attribute_not_exists(#hk)")
        .WithExpressionAttributeNames(names_type{{"#hk", config_.key_name()}});
  }

  Aws::DynamoDB::Model::Update update_starting_version_record(
      std::unsigned_integral auto const num_items_in_commit,
      concepts::version expected_version, item_type &&key,
      std::chrono::sys_seconds timestamp) const {
    auto const expression_attribute_values = [&]() {
      item_type result{
          {":inc", attribute_value(num_items_in_commit)},
          {":ver", attribute_value(expected_version)},
          {":ts", attribute_value(timestamp.time_since_epoch().count())}};
      if (config_.ttl()) {
        std::chrono::sys_seconds const expiration =
            timestamp + config_.ttl()->value;
        result.emplace(":ttl",
                       attribute_value(expiration.time_since_epoch().count()));
      }
      return result;
    };
    auto const expression_attribute_names = [&]() {
      names_type result{{"#ver", config_.max_version_name()},
                        {"#ts", config_.timestamp_name()}};
      if (config_.ttl()) {
        result.emplace("#ttl", config_.ttl()->name);
      }
      return result;
    };
    auto const update_expression = [&]() {
      return config_.ttl().has_value()
                 ? "set #ver = #ver + :inc, #ts = :ts, #ttl = :ttl"
                 : "set #ver = #ver + :inc, #ts = :ts";
    };
    return Aws::DynamoDB::Model::Update{}
        .WithTableName(config_.table_name())
        .WithKey(std::move(key))
        .WithUpdateExpression(update_expression())
        .WithConditionExpression("#ver = :ver")
        .WithExpressionAttributeNames(expression_attribute_names())
        .WithExpressionAttributeValues(expression_attribute_values());
  }

  int parse_version(std::string const &version_string) {
    int parsed_value = -1;
    auto const parse_result = std::from_chars(
        version_string.data(), version_string.data() + version_string.size(),
        parsed_value);
    std::error_code const result_code = std::make_error_code(parse_result.ec);
    if (result_code || (0 > parsed_value)) {
      throw version_validation_error{
          result_code, "Cannot parse valid version from : " + version_string};
    } else {
      return parsed_value;
    }
  }

  event_log_config const &config_;
  int version_ = 0; // DynamoDB uses native int for writing all integers. Beware
                    // of narrowing!!!!
};
} // namespace version_service_details_

template <concepts::domain_event... DomainEvents>
struct version_service : private version_service_details_::impl {
  version_service(std::remove_cvref_t<id_t<DomainEvents...>> id,
                  event_log_config const &config,
                  version_t<DomainEvents...> const version = 0)
      : version_service_details_::impl{config, narrow_cast<int>(version)},
        id_{std::move(id)} {}

  id_t<DomainEvents...> id() const noexcept { return id_; }

  using version_service_details_::impl::apply_version;

  Aws::DynamoDB::Model::TransactWriteItem
  version_record(concepts::identifier auto const &id,
                 std::unsigned_integral auto num_items_in_commit,
                 version_t<DomainEvents...> expected_version,
                 timestamp_t<DomainEvents...> timestamp) const {
    using Aws::DynamoDB::Model::TransactWriteItem;
    if (0 == expected_version) {
      return TransactWriteItem{}.WithPut(this->create_starting_version_record(
          narrow_cast<int>(num_items_in_commit), key(id),
          std::chrono::time_point_cast<std::chrono::sys_seconds::duration>(
              timestamp)));
    } else {
      return TransactWriteItem{}.WithUpdate(
          this->update_starting_version_record(
              num_items_in_commit, expected_version, key(id),
              std::chrono::time_point_cast<std::chrono::sys_seconds::duration>(
                  timestamp)));
    }
  }

  inline version_t<DomainEvents...> version() const
      noexcept(is_nothrow_narrow_cast_v<version_t<DomainEvents...>, int>) {
    return narrow_cast<version_t<DomainEvents...>>(this->version_);
  }

  inline Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>
  key(concepts::identifier auto const &id) const {
    return {{this->config_.key_name(), attribute_value(id)},
            {this->config_.version_name(), attribute_value(0)}};
  }

  inline Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>
  key() const {
    return key(id());
  }

  inline void
  on_commit_success(std::unsigned_integral auto const num_items_in_commit) {
    this->version_ += narrow_cast<int>(num_items_in_commit);
  }

  inline void update_version(Aws::DynamoDB::DynamoDBClient &client) {
    this->impl::update_version(client, key());
  }

  std::remove_cvref_t<id_t<DomainEvents...>> id_;
};
} // namespace skizzay::cddd::dynamodb
