#pragma once

#include "skizzay/cddd/aggregate_root.h"
#include "skizzay/cddd/dereference.h"
#include "skizzay/cddd/event_sourced.h"
#include "skizzay/cddd/nullable.h"
#include "skizzay/cddd/repository.h"

namespace skizzay::cddd::in_memory {
namespace event_source_details_ {
template <typename Store> struct impl {
  friend Store;

  template <concepts::aggregate_root_of<typename Store::domain_event_sequence>
                AggregateRoot>
  constexpr void
  load_from_history(AggregateRoot &aggregate_root,
                    version_t<AggregateRoot> const target_version) {
    if (auto event_buffer = skizzay::cddd::get(
            store_.event_buffers(), skizzay::cddd::id(aggregate_root));
        !skizzay::cddd::is_null(event_buffer)) {
      this->load_from_history(
          aggregate_root,
          dereference(event_buffer)
              .get_events(skizzay::cddd::version(aggregate_root) + 1,
                          target_version));
    }
  }

private:
  constexpr impl(Store &store) noexcept : store_{store} {}
  Store &store_;

  constexpr void
  load_from_history(auto &aggregate_root,
                    std::ranges::sized_range auto const &events) {
    auto visitor =
        as_event_visitor<typename Store::domain_event_sequence>(aggregate_root);
    for (auto const &event : events) {
      dereference(event).accept_event_visitor(visitor);
    }
  }
};
} // namespace event_source_details_

template <typename Store>
// requires concepts::domain_event_sequence<typename
// Store::domain_event_sequence>
using event_source = event_source_details_::impl<Store>;
} // namespace skizzay::cddd::in_memory
