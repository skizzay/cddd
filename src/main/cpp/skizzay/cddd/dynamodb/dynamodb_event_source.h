#pragma once

#include "skizzay/cddd/aggregate_root.h"
#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/dynamodb/dynamodb_attribute_value.h"
#include "skizzay/cddd/dynamodb/dynamodb_deser.h"
#include "skizzay/cddd/version.h"

#include <concepts>
#include <type_traits>

namespace skizzay::cddd::dynamodb {
namespace event_source_details_ {
template <concepts::domain_event... DomainEvents> struct impl {
  void
  load_from_history(concepts::aggregate_root<DomainEvents...> auto &aggregate,
                    version_t<decltype(aggregate)> const target_version) {
    auto query_request =
        get_request_()
            .WithTableName(serializer_.table_name())
            .WithConsistentRead(true)
            .WithKeyConditionExpression(
                "#pk = :pk AND #sk BETWEEN :sk_min AND :sk_max")
            .WithExpressionAttributeNames(make_expression_attribute_names())
            .WithExpressionAttributeValues(make_expression_attribute_values(
                id(aggregate), version(aggregate), target_version));
    auto outcome = client_.Query(query_request);
    if (outcome.IsSuccess()) {
      playback_events(outcome.GetResult().GetItems(), aggregate);
    } else {
      throw outcome.GetError();
    }
  }

  void playback_events(auto const &items, auto &aggregate) {
   
  }

  Aws::Map<Aws::String, Aws::String> make_expression_attribute_names() const {
    return {{"#pk", serializer_.key_name()},
            {"#sk", serializer_.version_name()}};
  }

  Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>
  make_expression_attribute_values(
      auto const &id, std::unsigned_integral auto const min_version,
      decltype(min_version) const max_version) const {
    return {{":pk", attribute_value(id)},
            {":sk_min", attribute_value(min_version)},
            {":sk_max", attribute_value(max_version)}};
  }

  serializer &serializer_;
  Aws::DynamoDB::DynamoDBClient &client_;
  std::function<Aws::DynamoDB::Model::QueryRequest()> get_request_;
};
} // namespace event_source_details_
} // namespace skizzay::cddd::dynamodb
