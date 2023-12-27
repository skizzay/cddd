#pragma once

#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/timestamp.h"
#include "skizzay/cddd/version.h"
#include <ranges>
#include <type_traits>

namespace skizzay::cddd {

namespace concepts {

template <typename T>
concept domain_event = std::is_class_v<T> && identifiable<T> && versioned<T> &&
    timestamped<T> &&
    requires(T &t, id_t<T> i, timestamp_t<T> ts, version_t<T> v) {
  { cddd::set_id(t, i)};
  { cddd::set_timestamp(t, ts)};
  { cddd::set_version(t, v)};
};

template <typename T>
concept mutable_domain_event = domain_event<T>;

template <typename T>
concept domain_event_range =
    std::ranges::range<T> && domain_event<std::ranges::range_value_t<T>>;
} // namespace concepts

template <typename, typename, typename, typename> struct basic_domain_event;

template <typename Tag, concepts::identifier Id, concepts::version Version,
          concepts::timestamp Timestamp>
struct basic_domain_event<Tag, Id, Version, Timestamp> {
  Id id = {};
  Version version = {};
  Timestamp timestamp = {};
};

template <concepts::domain_event> struct event_type {};

} // namespace skizzay::cddd