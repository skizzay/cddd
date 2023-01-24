#pragma once

#include "skizzay/cddd/aggregate_root.h"
#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/dynamodb/dynamodb_client.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_dispatcher.h"
#include "skizzay/cddd/dynamodb/dynamodb_operation_failed_error.h"
#include "skizzay/cddd/factory.h"
#include "skizzay/cddd/history_load_failed.h"
#include "skizzay/cddd/version.h"

#include <concepts>
#include <type_traits>

namespace skizzay::cddd::dynamodb {
namespace event_source_details_ {
template <concepts::domain_event_sequence DomainEvents> struct impl {
  template <
      typename GetRequest = default_factory<Aws::DynamoDB::Model::QueryRequest>>
  explicit impl(std::remove_cvref_t<id_t<DomainEvents>> id,
                event_dispatcher<DomainEvents> &dispatcher,
                client<id_t<DomainEvents>, version_t<DomainEvents>,
                       timestamp_t<DomainEvents>> &client)
      : id_{std::move(id)}, event_dispatcher_{dispatcher}, client_{client} {}

  void
  load_from_history(concepts::aggregate_root<DomainEvents> auto &aggregate,
                    version_t<decltype(aggregate)> const target_version) {
    playback_events(
        client_.get_events(id_, version(aggregate) + 1, target_version),
        aggregate);
  }

private:
  void playback_events(auto const &items, auto &aggregate) {
    std::ranges::for_each(
        items, [visitor = as_event_visitor<DomainEvents>(aggregate),
                this](auto const &item) mutable {
          event_dispatcher_.dispatch(item, visitor);
        });
  }

  std::remove_cvref_t<id_t<DomainEvents>> id_;
  event_dispatcher<DomainEvents> &event_dispatcher_;
  client<id_t<DomainEvents>, version_t<DomainEvents>,
         timestamp_t<DomainEvents>> &client_;
};

template <typename GetRequest, concepts::domain_event DomainEvents>
impl(event_dispatcher<DomainEvents> &, event_log_config const &,
     Aws::DynamoDB::DynamoDBClient &, GetRequest &&) -> impl<DomainEvents>;

template <typename GetRequest, concepts::domain_event DomainEvents>
impl(event_dispatcher<DomainEvents> &, event_log_config const &,
     Aws::DynamoDB::DynamoDBClient &) -> impl<DomainEvents>;
} // namespace event_source_details_

template <concepts::domain_event_sequence DomainEvents>
using event_source = event_source_details_::impl<DomainEvents>;
} // namespace skizzay::cddd::dynamodb
