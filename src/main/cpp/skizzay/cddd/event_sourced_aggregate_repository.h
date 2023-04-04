#pragma once

#include "skizzay/cddd/aggregate_root.h"
#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/event_sourced.h"
#include "skizzay/cddd/event_store.h"
#include "skizzay/cddd/key_not_found.h"
#include "skizzay/cddd/optimistic_concurrency_collision.h"
#include "skizzay/cddd/repository.h"

#include <type_traits>

namespace skizzay::cddd {

namespace event_sourced_aggregate_repository_details_ {
template <typename F, typename Id, typename EventStore>
concept aggregate_provider = concepts::event_store<EventStore> &&
    requires(F &aggregate_provider, Id const &id_value,
             event_stream_buffer_t<EventStore> buffer) {
  { aggregate_provider(id_value, buffer) } -> concepts::aggregate_root;
};
} // namespace event_sourced_aggregate_repository_details_

template <concepts::event_store EventStore, typename AggregateProvider>
struct event_sourced_aggregate_repository {
  event_sourced_aggregate_repository(EventStore &&event_store,
                                     AggregateProvider &&create_aggregate)
      : event_store_{std::move(event_store)}, create_aggregate_{std::move(
                                                  create_aggregate)} {}

  template <concepts::identifier Id>
  requires event_sourced_aggregate_repository_details_::aggregate_provider<
      AggregateProvider, Id, EventStore>
  constexpr concepts::aggregate_root auto get(Id const &id_value) const {
    concepts::aggregate_root auto aggregate = create_aggregate_(
        id_value, skizzay::cddd::get_event_stream_buffer(event_store_));
    concepts::event_source auto event_source =
        skizzay::cddd::get_event_source(event_store_);
    skizzay::cddd::load_from_history(event_source, aggregate);
    // In case we hit something only move constructible, such as std::unique_ptr
    // We want to target copy ellision
    if constexpr (std::is_copy_constructible_v<decltype(aggregate)>) {
      return aggregate;
    } else {
      return std::move(aggregate);
    }
  }

  constexpr void put(concepts::aggregate_root auto &&aggregate_root) {
    upsert(std::move(aggregate_root), [](auto const &aggregate_root) noexcept {
      return skizzay::cddd::version(aggregate_root) -
             skizzay::cddd::uncommitted_events_size(aggregate_root);
    });
  }

  // throws optimistic_concurrency_collision if aggregate_root already exists
  constexpr void add(concepts::aggregate_root auto &&aggregate_root) {
    upsert(std::move(aggregate_root), [](auto const &) noexcept { return 0; });
  }

  // throws key_not_found if aggregate_root does not exist
  constexpr void update(concepts::aggregate_root auto &&aggregate_root) {
    upsert(std::move(aggregate_root),
           [](auto const &aggregate_root) noexcept(false) {
             auto const aggregate_root_version =
                 skizzay::cddd::version(aggregate_root);
             auto const buffer_size =
                 skizzay::cddd::uncommitted_events_size(aggregate_root);
             if (aggregate_root_version == buffer_size) {
               throw key_not_found{skizzay::cddd::id(aggregate_root)};
             } else {
               return aggregate_root_version - buffer_size;
             }
           });
  }

  constexpr bool contains(concepts::identifier auto const &id_value) const {
    concepts::event_source auto event_source = get_event_source(event_store_);
    return skizzay::cddd::contains(event_source, id_value);
  }

private:
  constexpr void upsert(concepts::aggregate_root auto &&aggregate_root,
                        auto calculate_expected_version) {
    concepts::version auto const expected_version =
        calculate_expected_version(aggregate_root);
    concepts::identifier auto const &id_value =
        skizzay::cddd::id(aggregate_root);
    concepts::event_stream auto event_stream =
        skizzay::cddd::get_event_stream(event_store_);
    try {
      skizzay::cddd::commit_events(
          event_stream, id_value, expected_version,
          skizzay::cddd::uncommitted_events(std::move(aggregate_root)));
    } catch (optimistic_concurrency_collision const &e) {
      throw e;
    } catch (...) {
      skizzay::cddd::rollback_to(event_stream, id_value, expected_version);
      throw;
    }
  }

  EventStore event_store_;
  AggregateProvider create_aggregate_;
}; // namespace skizzay::cddd

template <concepts::event_store EventStore, typename AggregateProvider>
event_sourced_aggregate_repository(EventStore &&, AggregateProvider &&)
    -> event_sourced_aggregate_repository<EventStore, AggregateProvider>;
} // namespace skizzay::cddd
