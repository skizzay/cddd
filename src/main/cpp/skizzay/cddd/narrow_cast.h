#pragma once

#include <concepts>
#include <type_traits>
#include <typeinfo>

namespace skizzay::cddd {

struct bad_narrow_cast : std::bad_cast {
  char const *what() const noexcept override { return "Failed to narrow"; }
};

inline constexpr auto throw_bad_cast = []() { throw bad_narrow_cast{}; };

template <typename, typename>
struct is_nothrow_narrow_cast : std::false_type {};

template <typename To, typename From>
inline constexpr bool is_nothrow_narrow_cast_v =
    is_nothrow_narrow_cast<To, From>::value;

template <std::floating_point To, std::floating_point From>
struct is_nothrow_narrow_cast<To, From>
    : std::bool_constant<std::is_same_v<To, From> ||
                         (sizeof(To) >= sizeof(From))> {};

template <std::integral To, std::integral From>
struct is_nothrow_narrow_cast<To, From>
    : std::bool_constant<std::is_same_v<To, From> ||
                         (sizeof(To) >= sizeof(From) &&
                          std::is_signed_v<To> == std::is_signed_v<From>)> {};

template <std::signed_integral To, std::unsigned_integral From>
struct is_nothrow_narrow_cast<To, From>
    : std::bool_constant<(sizeof(To) > sizeof(From))> {};

template <typename To, typename From,
          std::invocable OnFailure = decltype(throw_bad_cast)>
requires is_nothrow_narrow_cast_v<To, From>
constexpr To
narrow_cast(From const from,
            [[maybe_unused]] OnFailure &&on_failure = {}) noexcept {
  return static_cast<To>(from);
}

template <typename To, typename From,
          std::invocable OnFailure = decltype(throw_bad_cast)>
requires std::is_arithmetic_v<To> && std::is_arithmetic_v<From> &&
    (not is_nothrow_narrow_cast_v<To, From>)constexpr To
    narrow_cast(From const from, OnFailure &&on_failure = {}) noexcept(
        std::is_nothrow_invocable_v<OnFailure>) {
  To const to = static_cast<To>(from);
  if (static_cast<From>(to) != from) {
    on_failure();
  }
  return to;
}

} // namespace skizzay::cddd
