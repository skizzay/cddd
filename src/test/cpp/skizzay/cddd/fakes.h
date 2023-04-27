#pragma once

#include "skizzay/cddd/aggregate_root.h"
#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/timestamp.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <cstddef>
#include <string>

namespace skizzay::cddd::test {

inline auto random_number_generator =
    Catch::Generators::random(std::size_t{1}, std::size_t{50});

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

template <std::ranges::sized_range EventBuffer>
struct fake_aggregate
    : skizzay::cddd::aggregate_root_base<
          fake_aggregate<EventBuffer>, EventBuffer, std::string, std::size_t> {
  using base_type =
      skizzay::cddd::aggregate_root_base<fake_aggregate<EventBuffer>,
                                         EventBuffer, std::string, std::size_t>;

  using base_type::base_type;
  using base_type::id;
  using base_type::update;
  using base_type::version;

  fake_aggregate(std::string id, EventBuffer events = {})
      : base_type{std::move(id), std::move(events)} {}

  template <std::size_t N>
  requires(1 == N) || (2 == N) void apply_event(fake_event<N> const &event) {
    this->update(skizzay::cddd::id(event), skizzay::cddd::version(event));
    timestamp = skizzay::cddd::timestamp(event);
  }

  template <std::size_t N>
  requires(3 == N) ||
      (4 == N) friend void apply_event(fake_aggregate &self,
                                       fake_event<N> const &event) {
    self.update(skizzay::cddd::id(event), skizzay::cddd::version(event));
    self.timestamp = skizzay::cddd::timestamp(event);
  }

  timestamp_t<fake_clock> timestamp = {};
};

template <std::ranges::sized_range EventBuffer>
fake_aggregate(std::string, EventBuffer) -> fake_aggregate<EventBuffer>;
} // namespace skizzay::cddd::test
