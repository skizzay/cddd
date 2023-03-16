#pragma once

#include "skizzay/cddd/concurrent_repository.h"
#include "skizzay/cddd/event_stream_buffer.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/in_memory/in_memory_event_buffer.h"
#include "skizzay/cddd/repository.h"
#include "skizzay/cddd/timestamp.h"
#include "skizzay/cddd/version.h"
#include "skizzay/cddd/views.h"

#include <ranges>
#include <utility>

namespace skizzay::cddd::in_memory {
namespace event_stream_details_ {
template <typename Store> struct impl {
  friend Store;

  template <concepts::identifier Id, concepts::version Version>
  constexpr void
  commit_events(Id id_value, Version const expected_version,
                typename Store::buffer_type event_stream_buffer) {
    if (not std::ranges::empty(event_stream_buffer)) {
      concepts::timestamp auto const timestamp = now(store_.clock());
      for (auto &&[i, element] : views::enumerate(event_stream_buffer)) {
        skizzay::cddd::set_id(element, std::as_const(id_value));
        skizzay::cddd::set_version(element, expected_version +
                                                narrow_cast<Version>(i + 1));
        skizzay::cddd::set_timestamp(element, timestamp);
      }
      commit_buffered_events(std::move(id_value),
                             std::move(event_stream_buffer), expected_version);
    }
  }

  constexpr void rollback_to(concepts::identifier auto const &id_value,
                             concepts::version auto const target_version) {
    if (auto buffer_ptr =
            store_.event_buffers().get(id_value, provide_null_value);
        buffer_ptr) {
      buffer_ptr->rollback_to(target_version);
    }
  }

private:
  constexpr impl(Store &store) noexcept : store_{store} {}

  Store &store_;

  constexpr void commit_buffered_events(auto &&id_value,
                                        typename Store::buffer_type &&buffer,
                                        auto const expected_version) {
    store_.event_buffers()
        .get_or_put(std::move(id_value))
        ->append(std::move(buffer), expected_version);
  }
};
} // namespace event_stream_details_

template <typename Store>
// requires requires(Store &s) {
//   { s.clock() } -> concepts::clock;
//   typename Store::buffer_type;
//   typename Store::domain_event_sequence;
// } && concepts::event_stream_buffer<typename Store::buffer_type> &&
//     concepts::domain_event_sequence<typename Store::domain_event_sequence>
using event_stream = event_stream_details_::impl<Store>;

} // namespace skizzay::cddd::in_memory
