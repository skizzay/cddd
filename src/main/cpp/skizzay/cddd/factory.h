#pragma once

#include <type_traits>

namespace skizzay::cddd {
namespace concepts {
template <typename F, typename T, typename... Args>
concept factory =
    std::is_invocable_r_v<T, F, Args...> and not std::is_void_v<T>;
} // namespace concepts

template <typename T> struct default_factory {
  // NOTE: noexcept clause assumes that copy-ellison is in effect
  constexpr T operator()() const
      noexcept(std::is_nothrow_default_constructible_v<T>) {
    return {};
  }

  // NOTE: noexcept clause assumes that copy-ellison is in effect
  template <typename... Args>
  requires(0 != sizeof...(Args)) constexpr T operator()(Args &&...args) const
      noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    return {std::forward<Args>(args)...};
  }
};

template <std::copy_constructible T> struct clone_factory {
  constexpr clone_factory() noexcept(std::is_nothrow_default_constructible_v<T>)
      : prototype_{} {}

  template <typename... Args>
  requires(0 != sizeof...(Args)) constexpr explicit clone_factory(
      Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
      : prototype_{std::forward<Args>(args)...} {}

  constexpr T operator()() const
      noexcept(std::is_nothrow_copy_constructible_v<T>) {
    return prototype_;
  }

private:
  T prototype_;
};

} // namespace skizzay::cddd
