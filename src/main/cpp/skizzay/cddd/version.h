#pragma once

#include "skizzay/cddd/dereference.h"
#include "skizzay/cddd/narrow_cast.h"
#include <concepts>
#include <exception>

namespace skizzay::cddd {

namespace concepts {
template <typename T>
concept version = std::unsigned_integral<T>;
} // namespace concepts

namespace version_details {

template <typename... Ts> void version(Ts const &...) = delete;

struct version_fn final {
  template <typename T>
  requires concepts::version<
      decltype(std::remove_cvref_t<dereference_t<T>>::version)>
  constexpr concepts::version auto operator()(T const &t) const noexcept {
    return dereference(t).version;
  }

  template <typename T>
  requires requires(T const &t) {
    { dereference(t).version() } -> concepts::version;
  }
  constexpr concepts::version auto operator()(T const &t) const
      noexcept(noexcept(dereference(t).version())) {
    return dereference(t).version();
  }

  template <typename T>
  requires requires(T const &t) {
    { version(dereference(t)) } -> concepts::version;
  }
  constexpr concepts::version auto operator()(T const &t) const
      noexcept(noexcept(version(dereference(t)))) {
    return version(dereference(t));
  }

  template <typename... Ts>
  requires(std::invocable<version_fn const, std::remove_reference_t<Ts> const &>
               &&...) constexpr concepts::version auto
  operator()(std::variant<Ts...> const &v) const
      noexcept((std::is_nothrow_invocable_v<
                    version_fn const, std::remove_reference_t<Ts> const &> &&
                ...)) {
    return std::visit(*this, v);
  }
};

void set_version(auto &...) = delete;

struct set_version_fn final {
  template <typename T, concepts::version Version>
  requires concepts::version<
      decltype(std::remove_cvref_t<dereference_t<T>>::version)>
  constexpr T &operator()(T &t, Version const v) const noexcept {
    dereference(t).version =
        narrow_cast<decltype(std::remove_cvref_t<dereference_t<T>>::version)>(
            v, &std::terminate);
    return t;
  }

  template <typename T, concepts::version Version>
  requires requires(T &t, Version const version) {
    {dereference(t).set_version(version)};
  }
  constexpr T &operator()(T &t, Version const version) const
      noexcept(noexcept(dereference(t).set_version(version))) {
    dereference(t).set_version(version);
    return t;
  }

  template <typename T, concepts::version Version>
  requires requires(T &t, Version const version) { {set_version(t, version)}; }
  constexpr T &operator()(T &t, Version const version) const
      noexcept(noexcept(set_version(dereference(t), version))) {
    set_version(dereference(t), version);
    return t;
  }

  template <typename... Ts, concepts::version Version>
  requires(std::invocable<set_version_fn const, Ts &, Version>
               &&...) constexpr decltype(auto)
  operator()(std::variant<Ts...> &t, Version const version) const noexcept(
      (std::is_nothrow_invocable_v<set_version_fn const, Ts &, Version> &&
       ...)) {
    std::visit([this, version](auto &t) { (*this)(t, version); }, t);
    return t;
  }
};

} // namespace version_details

inline namespace version_fn_ {
inline constexpr version_details::version_fn version = {};
inline constexpr version_details::set_version_fn set_version = {};
} // namespace version_fn_

namespace concepts {
template <typename T>
concept versioned = std::invocable < decltype(cddd::version),
        std::remove_reference_t<T>
const & >
    &&concepts::version<std::invoke_result_t<
        decltype(cddd::version), std::remove_reference_t<T> const &>>;
} // namespace concepts

namespace version_t_details_ {
template <typename> struct impl;

template <concepts::versioned T>
struct impl<T> : std::invoke_result<decltype(version),
                                    std::remove_reference_t<T> const &> {};

template <typename T>
requires(!concepts::versioned<T>) && requires { typename T::version_type; }
&&concepts::version<typename T::version_type> struct impl<T> {
  using type = typename T::version_type;
};
} // namespace version_t_details_

template <typename... Ts>
using version_t =
    std::common_type_t<typename version_t_details_::impl<Ts>::type...>;

} // namespace skizzay::cddd
