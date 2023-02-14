#pragma once

#include <concepts>
#include <memory>
#include <optional>
#include <type_traits>
#include <variant>

namespace skizzay::cddd {

template <typename T> struct nullable_traits {
  using nullable_type = std::optional<T>;
  using unwrapped_type = T;
  static inline constexpr nullable_type null_value = std::nullopt;
};

template <typename T> struct nullable_traits<std::optional<T>> {
  using nullable_type = std::optional<T>;
  using unwrapped_type = T;
  static inline constexpr nullable_type null_value = std::nullopt;
};

template <typename T> struct nullable_traits<T *> {
  using nullable_type = T *;
  using unwrapped_type =
      typename std::indirectly_readable_traits<T>::value_type;
  static inline constexpr nullable_type null_value = nullptr;
};

template <std::indirectly_readable T>
requires std::default_initializable<T>
struct nullable_traits<T> {
  using nullable_type = T;
  using unwrapped_type =
      typename std::indirectly_readable_traits<T>::value_type;
  static inline constinit nullable_type null_value = {};
  static inline constexpr bool is_constexpr = false;
};

// template <typename T> struct nullable_traits<std::shared_ptr<T>> {
//   using nullable_type = T;
//   using unwrapped_type =
//       typename std::indirectly_readable_traits<T>::value_type;
//   static inline constinit std::add_const_t<nullable_type> null_value = {};
//   static inline constexpr bool is_constexpr = false;
// };

template <typename... Ts>
requires(std::same_as<Ts, std::monostate> ||
         ...) struct nullable_traits<std::variant<Ts...>> {
  using nullable_type = std::variant<Ts...>;
  using unwrapped_type = std::variant<Ts...>;
  static inline constexpr nullable_type null_value = std::monostate{};
};

template <typename T>
using nullable_t = typename nullable_traits<T>::nullable_type;

template <typename T>
using unwrap_nullable_t = typename nullable_traits<T>::unwrapped_type;

template <typename T> inline constexpr bool is_null_value_constexpr = true;

template <typename T>
inline constexpr nullable_t<T> null_value = nullable_traits<T>::null_value;

template <typename T>
requires(!nullable_traits<T>::is_constexpr) inline std::add_const_t<
    nullable_t<T>> null_value<T>{nullable_traits<T>::null_value};

namespace concepts {
template <typename T>
concept nullable = std::same_as<T, nullable_t<T>>;

template <typename T, typename U>
concept nullable_for = nullable<T> && std::same_as<unwrap_nullable_t<T>, U>;
} // namespace concepts
} // namespace skizzay::cddd
