#pragma once

#include <type_traits>

namespace skizzay::cddd {
template <typename...> struct type_sequence;

template <typename> struct is_type_sequence : std::false_type {};
template <typename... Ts>
struct is_type_sequence<type_sequence<Ts...>> : std::true_type {};

namespace concepts {
template <typename T>
concept type_sequence = is_type_sequence<T>::value;
}

namespace type_sequence_details_ {
template <typename, typename> struct is_subset_impl : std::false_type {};
template <typename, typename> struct contains_impl : std::false_type {};

template <typename... Ts, typename T>
struct contains_impl<type_sequence<Ts...>, T>
    : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

template <typename... Ts, typename... Us>
requires(sizeof...(Us) <=
         sizeof...(Ts)) struct is_subset_impl<type_sequence<Ts...>,
                                              type_sequence<Us...>>
    : std::conjunction<contains_impl<type_sequence<Ts...>, Us>...> {
};
} // namespace type_sequence_details_

template <typename... Ts> struct type_sequence {
  static inline constexpr std::size_t size = sizeof...(Ts);
  static inline constexpr bool empty = (0 == sizeof...(Ts));
  template <typename T>
  static inline constexpr bool contains =
      type_sequence_details_::contains_impl<type_sequence<Ts...>, T>::value;
  template <typename TypeSequence>
  static inline constexpr bool is_subset_of =
      type_sequence_details_::is_subset_impl<TypeSequence,
                                             type_sequence<Ts...>>::value;
  template <template <typename> typename Predicate>
  static inline constexpr bool all = std::conjunction_v<Predicate<Ts>...>;
  template <template <typename> typename Predicate>
  static inline constexpr bool any = std::disjunction_v<Predicate<Ts>...>;
  template <template <typename> typename Predicate>
  static inline constexpr bool none =
      std::negation_v<std::disjunction<Predicate<Ts>...>>;
};
} // namespace skizzay::cddd
