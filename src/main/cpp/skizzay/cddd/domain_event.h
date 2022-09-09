#pragma once

#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/timestamp.h"
#include "skizzay/cddd/version.h"
#include <concepts>
#include <ranges>
#include <type_traits>

namespace skizzay::cddd {

namespace concepts {

template <typename T>
concept domain_event =
    std::is_class_v<T> && identifiable<T> && versioned<T> && timestamped<T>;

template <typename T>
concept domain_event_range =
    std::ranges::range<T> && domain_event<std::ranges::range_value_t<T>>;
} // namespace concepts

template <typename, typename, typename, typename> struct basic_domain_event;

template <typename Tag, concepts::identifier Id, std::unsigned_integral Version,
          concepts::timestamp Timestamp>
struct basic_domain_event<Tag, Id, Version, Timestamp> {
  Id id = {};
  Version version = {};
  Timestamp timestamp = {};
};

template <concepts::domain_event...> struct domain_of_events final {};

} // namespace skizzay::cddd