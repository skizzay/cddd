#pragma once

#include "skizzay/cddd/aggregate_root.h"
#include "skizzay/cddd/dynamodb/dynamodb_attribute_value.h"
#include "skizzay/cddd/dynamodb/dynamodb_operation_failed_error.h"
#include "skizzay/cddd/dynamodb/dynamodb_version_validation_error.h"
#include "skizzay/cddd/history_load_failed.h"
#include "skizzay/cddd/version.h"

#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/dynamodb/model/QueryRequest.h>

namespace skizzay::cddd::dynamodb {
template <typename E>
using history_load_error = operation_failed_error<history_load_failed, E>;

namespace event_source_details_ {
template <concepts::version Version>
constexpr Version parse_version(std::string const &version_string) {
  Version parsed_value = std::numeric_limits<Version>::max();
  auto const parse_result = std::from_chars(
      version_string.data(), version_string.data() + version_string.size(),
      parsed_value);
  std::error_code const result_code = std::make_error_code(parse_result.ec);
  if (result_code) {
    throw version_validation_error{
        result_code, "Cannot parse valid version from : " + version_string};
  } else {
    return parsed_value;
  }
}

template <typename Store> struct impl {
  friend Store;

  void load_from_history(
      concepts::aggregate_root auto &aggregate_root,
      version_t<decltype(aggregate_root)> const target_version) const {
    auto const outcome = store_->client().get_events(query_request(
        id(aggregate_root), version(aggregate_root) + 1, target_version));
    if (outcome.IsSuccess()) {
      playback_events(outcome.GetResult().GetItems(), aggregate_root);
    } else {
      throw history_load_error{outcome.GetError()};
    }
  }

  bool contains(concepts::identifier auto id_value) const {
    return head(id_value) > std::uintmax_t{};
  }

  template <concepts::version Version = std::uintmax_t>
  Version head(concepts::identifier auto id_value) const {
    auto const outcome = store_->client().get_item(head_request(id_value));
    if (outcome.IsSuccess()) {
      auto const &item = outcome.GetResult().GetItem();
      auto const iter = item.find(store_->config().max_version_field().name);
      return std::end(item) == iter
                 ? Version{}
                 : parse_version<Version>(iter->second.GetN());
    } else {
      throw history_load_error{outcome.GetError()};
    }
  }

private:
  constexpr impl(Store &store) noexcept : store_{&store} {}

  Aws::DynamoDB::Model::GetItemRequest
  head_request(concepts::identifier auto id_value) const {
    return Aws::DynamoDB::Model::GetItemRequest{}
        .WithTableName(store_->config().table().get())
        .WithConsistentRead(false)
        .AddKey(store_->config().hash_key().name, attribute_value(id_value))
        .AddKey(store_->config().sort_key().name, attribute_value(0))
        .WithProjectionExpression(
            store_->config().max_version_field().name_expression)
        .AddExpressionAttributeNames(
            store_->config().max_version_field().name_expression,
            store_->config().max_version_field().name);
  }

  Aws::DynamoDB::Model::QueryRequest
  query_request(concepts::identifier auto id_value,
                concepts::version auto const begin_version,
                decltype(begin_version) const target_version) const {
    return Aws::DynamoDB::Model::QueryRequest{}
        .WithTableName(store_->config().table().get())
        .WithConsistentRead(true)
        .WithKeyConditionExpression(store_->query_key_condition_expression())
        .AddExpressionAttributeNames(
            store_->config().hash_key().name_expression,
            store_->config().hash_key().name)
        .AddExpressionAttributeNames(
            store_->config().sort_key().name_expression,
            store_->config().sort_key().name)
        .AddExpressionAttributeValues(
            store_->config().hash_key().value_expression,
            attribute_value(id_value))
        .AddExpressionAttributeValues(
            store_->config().sort_key().value_expression + "_min",
            attribute_value(begin_version))
        .AddExpressionAttributeValues(
            store_->config().sort_key().value_expression + "_max",
            attribute_value(target_version));
  }

  constexpr void playback_events(auto const &items,
                                 auto &aggregate_root) const {
    std::ranges::for_each(items, [this, &aggregate_root](record const &item) {
      store_->event_dispatcher().dispatch(item, aggregate_root);
    });
  }

  Store *store_;
};
} // namespace event_source_details_

template <typename Store>
using event_source = event_source_details_::impl<Store>;
} // namespace skizzay::cddd::dynamodb
