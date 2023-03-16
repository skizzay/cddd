#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/timestamp.h"

#include <cstddef>
#include <string>

namespace skizzay::cddd::test {

struct fake_clock {
  std::chrono::system_clock::time_point now() noexcept {
    result = skizzay::cddd::now(system_clock);
    return result;
  }

  [[no_unique_address]] std::chrono::system_clock system_clock;
  std::chrono::system_clock::time_point result;
};

template <std::size_t N>
struct fake_event
    : skizzay::cddd::basic_domain_event<fake_event<N>, std::string, std::size_t,
                                        timestamp_t<fake_clock>> {};

namespace fake_event_sequence_details_ {
template <concepts::domain_event_sequence DomainEvents, std::size_t N,
          std::size_t I>
struct impl;

template <concepts::domain_event... DomainEvents, std::size_t N, std::size_t I>
requires(sizeof...(DomainEvents) <
         N) struct impl<domain_event_sequence<DomainEvents...>, N, I> {
  using type =
      typename impl<domain_event_sequence<DomainEvents..., fake_event<I>>, N,
                    I + 1>::type;
};

template <concepts::domain_event... DomainEvents, std::size_t N, std::size_t I>
requires(sizeof...(DomainEvents) ==
         N) struct impl<domain_event_sequence<DomainEvents...>, N, I> {
  using type = domain_event_sequence<DomainEvents...>;
};
} // namespace fake_event_sequence_details_

template <std::size_t N, std::size_t StartAt = 1>
using fake_event_sequence =
    typename fake_event_sequence_details_::impl<domain_event_sequence<>, N,
                                                StartAt>::type;

template <std::ranges::sized_range EventBuffer> struct fake_aggregate {
  template <std::size_t N>
  requires(1 == N) || (2 == N) void apply(fake_event<N> const &event) {
    if (skizzay::cddd::id(event) != id) {
      throw std::invalid_argument{"Unexpected id encountered"};
    }
    version = skizzay::cddd::version(event);
    timestamp = skizzay::cddd::timestamp(event);
  }

  std::string id;
  EventBuffer uncommitted_events;
  std::size_t version = {};
  timestamp_t<fake_clock> timestamp = {};
};

template <std::ranges::sized_range EventBuffer>
fake_aggregate(std::string, EventBuffer) -> fake_aggregate<EventBuffer>;
} // namespace skizzay::cddd::test
