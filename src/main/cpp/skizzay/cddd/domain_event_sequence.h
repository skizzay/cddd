#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/timestamp.h"
#include "skizzay/cddd/version.h"

namespace skizzay::cddd {
namespace domain_event_sequence_details_ {
template <template <typename...> typename, typename...>
struct reduce_impl;

template <template <typename...> typename Reduce, typename T, typename U,
          typename... Us>
struct reduce_impl<Reduce, T, U, Us...> {
  using type =
      typename reduce_impl<Reduce, typename Reduce<T, U>::type, Us...>::type;
};

template <template <typename...> typename Reduce, typename T>
struct reduce_impl<Reduce, T> {
  using type = T;
};
} // namespace domain_event_sequence_details_

template <concepts::domain_event... DomainEvents> struct domain_event_sequence {
  using id_type = std::remove_cvref_t<id_t<DomainEvents...>>;
  using id_reference_type = id_type const &;
  using version_type = version_t<DomainEvents...>;
  using timestamp_type = timestamp_t<DomainEvents...>;
  static inline constexpr std::size_t size = sizeof...(DomainEvents);
  static inline constexpr bool empty = 0 == size;
  template <concepts::domain_event DomainEvent>
  static inline constexpr bool
      contains = (std::same_as<DomainEvent, DomainEvents> || ...);
  template <template <typename> typename Predicate>
  static inline constexpr bool all =
      std::conjunction_v<Predicate<DomainEvents>...>;
  template <template <typename> typename Predicate>
  static inline constexpr bool any =
      std::disjunction_v<Predicate<DomainEvents>...>;
  template <template <typename> typename Map>
  using mapped_type =
      domain_event_sequence<typename Map<DomainEvents>::type...>;
  template <template <typename...> typename Reduce>
  using reduced_type =
      domain_event_sequence_details_::reduce_impl<Reduce, DomainEvents...>;
};

namespace concepts {
namespace domain_event_sequence_details_ {
template <typename> struct impl : std::false_type {};

template <domain_event... DomainEvents>
struct impl<domain_event_sequence<DomainEvents...> >
    : std::true_type {};
} // namespace domain_event_sequence_details_

template <typename T>
concept domain_event_sequence = domain_event_sequence_details_::impl<T>::value;

template <typename T, typename DomainEvents>
concept domain_event_of = domain_event<T> &&
    domain_event_sequence<DomainEvents> && DomainEvents::template contains<T>;

} // namespace concepts
} // namespace skizzay::cddd
