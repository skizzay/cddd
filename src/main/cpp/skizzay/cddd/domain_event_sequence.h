#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/event_sourced.h"
#include "skizzay/cddd/type_sequence.h"

#include <concepts>
#include <type_traits>

namespace skizzay::cddd {

template <concepts::domain_event... DomainEvents>
struct domain_event_sequence : type_sequence<DomainEvents...> {
  using id_type = std::remove_cvref_t<id_t<DomainEvents...>>;
  using timestamp_type = timestamp_t<DomainEvents...>;
  using version_type = version_t<DomainEvents...>;
  template <typename T>
  static inline constexpr bool is_handler =
      (std::invocable<decltype(skizzay::cddd::apply),
                      std::remove_reference_t<T> &,
                      std::remove_reference_t<DomainEvents> const &> &&
       ...);
};

namespace concepts {

namespace domain_event_sequence_details_ {
template <typename> struct impl : std::false_type {};

template <domain_event... DomainEvents>
struct impl<skizzay::cddd::domain_event_sequence<DomainEvents...>>
    : std::true_type {};
} // namespace domain_event_sequence_details_

template <typename T>
concept domain_event_sequence = domain_event_sequence_details_::impl<T>::value;

} // namespace concepts
} // namespace skizzay::cddd
