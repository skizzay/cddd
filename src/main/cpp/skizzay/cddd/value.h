#pragma once

#include <type_traits>
#include <utility>

namespace skizzay::cddd {
template <typename Tag, typename T> struct value final {
  using value_type = std::remove_cvref_t<T>;

  constexpr value() noexcept(
      std::is_nothrow_default_constructible_v<value_type>) requires
      std::is_default_constructible_v<T>
  = default;

  constexpr value(value_type &&v) noexcept(
      std::is_nothrow_move_constructible_v<value_type>)
      : value_{std::move_if_noexcept(v)} {}

  constexpr value(value_type const &v) noexcept(
      std::is_nothrow_copy_constructible_v<value_type>)
      : value_{v} {}

  constexpr value_type const &get() const noexcept { return value_; }

  constexpr value_type &get() &noexcept { return value_; }

  constexpr value_type &&get() &&noexcept { return std::move(value_); }

  constexpr auto operator<=>(value const &) const noexcept = default;

private:
  value_type value_;
};
} // namespace skizzay::cddd
