#pragma once

#include "skizzay/cddd/aggregate_root.h"
#include "skizzay/cddd/dynamodb/dynamodb_attribute_value.h"
#include "skizzay/cddd/dynamodb/dynamodb_operation_failed_error.h"
#include "skizzay/cddd/history_load_failed.h"
#include "skizzay/cddd/version.h"

#include <aws/dynamodb/model/QueryRequest.h>

namespace skizzay::cddd::dynamodb {
template <typename E>
using history_load_error = operation_failed_error<history_load_failed, E>;

namespace event_source_details_ {
// template <concepts::domain_event... DomainEvents> struct impl {
//   template <
//       typename GetRequest =
//       default_factory<Aws::DynamoDB::Model::QueryRequest>>
//   explicit impl(event_dispatcher<DomainEvents...> &dispatcher,
//                 event_log_config const &config,
//                 Aws::DynamoDB::DynamoDBClient &client,
//                 GetRequest get_request = {})
//       : event_dispatcher_{dispatcher}, config_{config}, client_{client},
//         get_request_{std::move_if_noexcept(get_request)} {}

//   void
//   load_from_history(concepts::aggregate_root<DomainEvents...> auto
//   &aggregate,
//                     version_t<decltype(aggregate)> const target_version) {
//     auto const outcome = client_.Query(
//         query_request(id(aggregate), version(aggregate) + 1,
//         target_version));
//     if (outcome.IsSuccess()) {
//       playback_events(outcome.GetResult().GetItems(), aggregate);
//     } else {
//       throw history_load_error{outcome.GetError()};
//     }
//   }

// private:
//   Aws::DynamoDB::Model::QueryRequest
//   query_request(id_t<DomainEvents...> id,
//                 version_t<DomainEvents...> const begin_version,
//                 version_t<DomainEvents...> const target_version) {
//     return get_request_()
//         .WithTableName(config_.table_name())
//         .WithConsistentRead(true)
//         .WithKeyConditionExpression(
//             "(#pk = :pk) AND (#sk BETWEEN :sk_min AND :sk_max)")
//         .WithExpressionAttributeNames(make_expression_attribute_names())
//         .WithExpressionAttributeValues(make_expression_attribute_values(
//             id, begin_version, target_version));
//   }

//   void playback_events(auto const &items, auto &aggregate) {
//     std::ranges::for_each(
//         items, [visitor = as_event_visitor<DomainEvents...>(aggregate),
//                 this](auto const &item) mutable {
//           event_dispatcher_.dispatch(item, visitor);
//         });
//   }

//   Aws::Map<Aws::String, Aws::String> make_expression_attribute_names() const
//   {
//     return {{"#pk", config_.key_name()}, {"#sk", config_.version_name()}};
//   }

//   Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>
//   make_expression_attribute_values(
//       auto const &id, std::unsigned_integral auto const min_version,
//       decltype(min_version) const max_version) const {
//     return {{":pk", attribute_value(id)},
//             {":sk_min", attribute_value(min_version)},
//             {":sk_max", attribute_value(max_version)}};
//   }

//   event_dispatcher<DomainEvents...> &event_dispatcher_;
//   event_log_config const &config_;
//   Aws::DynamoDB::DynamoDBClient &client_;
//   std::function<Aws::DynamoDB::Model::QueryRequest()> get_request_;
// };

// template <typename GetRequest, concepts::domain_event... DomainEvents>
// impl(event_dispatcher<DomainEvents...> &, event_log_config const &,
//      Aws::DynamoDB::DynamoDBClient &, GetRequest &&) ->
//      impl<DomainEvents...>;

// template <typename GetRequest, concepts::domain_event... DomainEvents>
// impl(event_dispatcher<DomainEvents...> &, event_log_config const &,
//      Aws::DynamoDB::DynamoDBClient &) -> impl<DomainEvents...>;

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

  bool contains(concepts::identifier auto id_value) const { return false; }

private:
  constexpr impl(Store &store) noexcept : store_{&store} {}

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
