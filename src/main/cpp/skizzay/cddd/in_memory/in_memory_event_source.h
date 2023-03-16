#pragma once

#include "skizzay/cddd/aggregate_root.h"
#include "skizzay/cddd/event_sourced.h"

namespace skizzay::cddd::in_memory {
namespace event_source_details_ {
template <typename Store> struct impl {
  template <concepts::aggregate_root AggregateRoot>
  constexpr void load_from_history(
      [[maybe_unused]] AggregateRoot &aggregate_root,
      [[maybe_unused]] version_t<AggregateRoot> const target_version) {}
};
} // namespace event_source_details_

template <typename Store>
using event_source = event_source_details_::impl<Store>;
} // namespace skizzay::cddd::in_memory
