#pragma once

#include <concepts>
#include <exception>

namespace skizzay::cddd {
namespace finally_details_ {
template <std::invocable F> struct impl final {
  ~impl() noexcept(std::is_nothrow_invocable_v<F>) { std::forward<F>(f_)(); }

  F &&f_;
};

struct finally_fn final {
  template <std::invocable F> impl<F> operator()(F &&f) const noexcept {
    return {std::forward<F>(f)};
  }

  auto operator()(std::invocable auto &&on_success,
                  std::invocable auto &&on_failure) const noexcept {
    return impl{[exceptions = std::uncaught_exceptions(),
                 on_success = std::move(on_success),
                 on_failure = std::move(
                     on_failure)]() noexcept(std::
                                                 is_nothrow_invocable_v<
                                                     decltype(on_success)> &&
                                             std::is_nothrow_invocable_v<
                                                 decltype(on_failure)>) {
      if (std::uncaught_exceptions() == exceptions) {
        on_success();
      } else {
        on_failure();
      }
    }};
  }
};
} // namespace finally_details_

inline constexpr finally_details_::finally_fn finally = {};

inline constexpr auto on_exit_success = [](std::invocable auto &&f) noexcept {
  return finally(
      [exceptions = std::uncaught_exceptions(),
       f = std::move(f)]() noexcept(std::is_nothrow_invocable_v<decltype(f)>) {
        if (std::uncaught_exceptions() == exceptions) {
          f();
        }
      });
};

inline constexpr auto on_exit_failure = [](std::invocable auto &&f) noexcept {
  return finally(
      [exceptions = std::uncaught_exceptions(),
       f = std::move(f)]() noexcept(std::is_nothrow_invocable_v<decltype(f)>) {
        if (std::uncaught_exceptions() != exceptions) {
          f();
        }
      });
};
} // namespace skizzay::cddd
