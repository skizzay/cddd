#pragma once

#include <skizzay/cddd/event_handler.h>
#include <skizzay/cddd/event_sourced.h>
#include <skizzay/cddd/handle.h>

namespace skizzay::cddd {
template <concepts::handle EventSource, concepts::handle SnapshotSource,
          concepts::event_handler<handled_t<EventSource>> Aggregate>
requires concepts::versioned<Aggregate> && concepts::identifiable<Aggregate>
struct event_sourced_aggregate_provider {
  constexpr Aggregate
  get(id_t<Aggregate> const id,
      version_t<Aggregate> const target_version =
          std::numeric_limits<version_t<Aggregate>>::max()) {
    Aggregate aggregate{id};
    load_from_snapshot(*snapshot_source_, aggregate, target_version);
    load_from_history(*event_source_, aggregate, version(aggregate),
                      target_version);
    return aggregate;
  }

  EventSource event_source_;
  SnapshotSource snapshot_source_;
};
} // namespace skizzay::cddd
