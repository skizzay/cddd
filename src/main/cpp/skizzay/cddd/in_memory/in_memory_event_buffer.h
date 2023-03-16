#pragma once

#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/domain_event_wrapper.h"
#include "skizzay/cddd/narrow_cast.h"
#include "skizzay/cddd/optimistic_concurrency_collision.h"

#include <cassert>
#include <shared_mutex>
#include <sstream>

namespace skizzay::cddd::in_memory {
template <concepts::domain_event_sequence DomainEvents> struct buffer final {
  using event_t = std::unique_ptr<event_wrapper<DomainEvents>>;

  constexpr version_t<DomainEvents> version() const noexcept {
    std::shared_lock l_{m_};
    return narrow_cast<version_t<DomainEvents>>(std::size(events_),
                                                std::terminate);
  }

  template <std::ranges::sized_range EventBuffer>
  requires std::same_as<event_t, std::ranges::range_value_t<EventBuffer>>
  constexpr void append(EventBuffer &&events,
                        version_t<DomainEvents> const expected_version) {
    std::lock_guard l_{m_};
    concepts::version auto const actual_version = std::size(events_);
    if (expected_version == actual_version) {
      events_.insert(std::end(events_),
                     std::move_iterator(std::ranges::begin(events)),
                     std::move_iterator(std::ranges::end(events)));
    } else {
      std::ostringstream what;
      what << "Saving events, expected version " << expected_version
           << ", but found " << actual_version;
      throw optimistic_concurrency_collision{what.str(), expected_version};
    }
  }

  std::ranges::sized_range auto
  get_events(version_t<DomainEvents> const begin_version,
             version_t<DomainEvents> const target_version) {
    assert(begin_version <= target_version);
    std::shared_lock l_{m_};
    auto const begin_iterator = std::ranges::begin(events_);
    return std::ranges::subrange(
        begin_iterator + begin_version - 1,
        begin_iterator + std::min(std::size(events_), target_version));
  }

  void rollback_to(concepts::version auto const target_version) {
    std::lock_guard l_{m_};
    if (target_version < std::size(events_)) {
      events_.erase(std::ranges::begin(events_) + target_version,
                    std::ranges::end(events_));
    }
  }

private:
  mutable std::shared_mutex m_;
  std::vector<event_t> events_;
};
} // namespace skizzay::cddd::in_memory
