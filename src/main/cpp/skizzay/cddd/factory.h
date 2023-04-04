#pragma once

#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/nullable.h"
#include <concepts>
#include <memory>
#include <type_traits>

namespace skizzay::cddd {
namespace concepts {
template <typename F, typename T, typename... Args>
concept factory =
    std::is_invocable_r_v<T, F, Args...> and not std::is_void_v<T>;

template <typename F, typename Entity>
concept entity_factory = identifiable<Entity> &&
    factory<F, Entity, std::remove_cvref_t<id_t<Entity>>>;
} // namespace concepts

template <typename T> struct default_factory {
  // NOTE: noexcept clause assumes that copy-ellison is in effect
  constexpr T operator()(auto &&...) const
      noexcept(std::is_nothrow_default_constructible_v<T>) {
    return {};
  }

  // NOTE: noexcept clause assumes that copy-ellison is in effect
  template <typename... Args>
  requires(0 != sizeof...(Args)) &&
      std::constructible_from<T, Args...> constexpr T
      operator()(Args &&...args) const
      noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    return T{std::forward<Args>(args)...};
  }
};

template <typename T> struct nonnull_default_factory : default_factory<T> {};

template <typename T> struct nonnull_default_factory<std::shared_ptr<T>> {
  template <typename... Args>
  requires std::constructible_from<T, Args...>
  constexpr std::shared_ptr<T> operator()(Args &&...args) const
      noexcept(false) {
    return std::make_shared<T>(std::forward<Args>(args)...);
  }

  constexpr std::shared_ptr<T> operator()(auto &&...) const {
    return std::make_shared<T>();
  }
};

template <typename T, typename D>
struct nonnull_default_factory<std::unique_ptr<T, D>> {
  template <typename... Args>
  requires std::constructible_from<T, Args...>
  constexpr std::unique_ptr<T, D> operator()(Args &&...args) const
      noexcept(false) {
    return std::unique_ptr<T, D>{new T{std::forward<Args>(args)...}};
  }

  template <typename... Args>
  requires std::constructible_from<T, Args...>
  constexpr std::unique_ptr<T, D> operator()(D deleter, Args &&...args) const
      noexcept(false) {
    return std::unique_ptr<T, D>{new T{std::forward<Args>(args)...},
                                 std::move(deleter)};
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

template <concepts::nullable T> struct null_factory {
  constexpr T operator()(auto &&...) const noexcept { return null_value<T>; }
};

inline constexpr auto clone_of = [](auto clone) {
  return [clone](auto &&...) noexcept(
             std::is_nothrow_copy_constructible_v<decltype(clone)>) {
    return clone;
  };
};

} // namespace skizzay::cddd
