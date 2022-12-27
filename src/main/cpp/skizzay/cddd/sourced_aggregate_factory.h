#pragma once

#include "skizzay/cddd/event_sourced.h"

namespace skizzay::cddd {

template <typename SnapshotSource, typename EventSource>
struct sourced_aggregate_factory {
  constexpr sourced_aggregate_factory(SnapshotSource &snapshot_source,
                                      EventSource &event_source) noexcept
      : snapshot_source_{snapshot_source}, event_source_{event_source} {}

  template <concepts::versioned Aggregate>
  constexpr void load_from_history(Aggregate &aggregate,
                                   version_t<Aggregate> const target_version) {
    skizzay::cddd::load_from_snapshot(snapshot_source_, aggregate,
                                      target_version);
    skizzay::cddd::load_from_history(event_source_, aggregate, target_version);
  }

private:
  SnapshotSource &snapshot_source_;
  EventSource &event_source_;
};
} // namespace skizzay::cddd
