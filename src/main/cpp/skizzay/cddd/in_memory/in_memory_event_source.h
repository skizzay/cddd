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
  using domain_event_sequence_type = typename Store::domain_event_sequence;

  template <
      concepts::aggregate_root_of<domain_event_sequence_type> AggregateRoot>
  constexpr void
  load_from_history(AggregateRoot &aggregate_root,
                    version_t<AggregateRoot> const target_version) const {
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

  constexpr bool
  contains(std::convertible_to<id_t<domain_event_sequence_type>> auto id_value)
      const noexcept {
    return skizzay::cddd::contains(store_.event_buffers(), id_value);
  }

private:
  constexpr impl(Store &store) noexcept : store_{store} {}
  Store &store_;

  constexpr void
  load_from_history(auto &aggregate_root,
                    std::ranges::sized_range auto const &events) const {
    auto visitor = as_event_visitor<domain_event_sequence_type>(aggregate_root);
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
