#pragma once

#include "skizzay/cddd/aggregate_root.h"
#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/event_sourced.h"
#include "skizzay/cddd/event_store.h"
#include "skizzay/cddd/factory.h"
#include "skizzay/cddd/nullable.h"
#include "skizzay/cddd/optimistic_concurrency_collision.h"

namespace skizzay::cddd {

namespace event_sourced_aggregate_repository_details_ {
template <typename F, typename T>
concept aggregate_provider =
    concepts::factory<F, T> || concepts::factory<F, T, id_t<T>>;
}

template <concepts::event_store EventStore, typename AggregateProvider>
struct event_sourced_aggregate_repository {
  event_sourced_aggregate_repository(EventStore event_store,
                                     AggregateProvider create_aggregate)
      : event_store_{std::move(event_store)}, create_aggregate_{std::move(
                                                  create_aggregate)} {}

  template <concepts::identifier Id>
  constexpr concepts::aggregate_root auto get(Id const &id_value) {
    concepts::aggregate_root auto aggregate =
        create_aggregate(id_value, get_event_stream_buffer(event_store_));
    concepts::event_source auto event_source = get_event_source(event_store_);
    load_from_history(event_source, aggregate);
    return aggregate;
  }

  constexpr void put(concepts::aggregate_root auto &&aggregate_root) {
    concepts::event_stream auto event_stream = get_event_stream(event_store_);
    concepts::identifier auto const &id_value = id(aggregate_root);
    concepts::event_stream_buffer auto event_stream_buffer =
        uncommitted_events(aggregate_root);
    concepts::version auto const expected_version =
        version(aggregate_root) - std::size(event_stream_buffer);
    try {
      commit_events(event_stream, id_value, expected_version,
                    std::move(event_stream_buffer));
    } catch (optimistic_concurrency_collision const &e) {
      throw e;
    } catch (...) {
      rollback_to(event_stream, id_value, expected_version);
      throw;
    }
  }

private:
  template <concepts::identifier Id, concepts::event_stream_buffer Buffer>
  requires std::invocable<AggregateProvider, Id const &, Buffer>
  constexpr std::invoke_result_t<AggregateProvider, Id const &, Buffer>
  create_aggregate(Id const &id_value, Buffer buffer) {
    return create_aggregate_(id_value, std::move(buffer));
  }

  template <concepts::identifier Id>
  requires std::invocable<AggregateProvider> &&
      (!std::invocable<AggregateProvider, Id const &>)constexpr std::
          invoke_result_t<AggregateProvider, Id const &> create_aggregate(
              Id const &) {
    return create_aggregate_();
  }

  EventStore event_store_;
  AggregateProvider create_aggregate_;
};
} // namespace skizzay::cddd
