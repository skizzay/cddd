#pragma once

#include <concepts>

namespace skizzay::cddd {
namespace version_details {

template <typename... Ts> void version(Ts const &...) = delete;

struct version_fn_ final {
  template <typename T>
  requires std::unsigned_integral<decltype(T::version)>
  constexpr std::unsigned_integral auto operator()(T const &tc) const noexcept {
    return tc.version;
  }

  template <typename T>
  requires requires(T const &t) {
    { t.version() } -> std::unsigned_integral;
  }
  constexpr std::unsigned_integral auto operator()(T const &t) const
      noexcept(noexcept(t.version())) {
    return t.version();
  }

  template <typename T>
  requires requires(T const &t) {
    { version(t) } -> std::unsigned_integral;
  }
  constexpr std::unsigned_integral auto operator()(T const &t) const
      noexcept(noexcept(version(t))) {
    return version(t);
  }

  template <typename... Ts>
  constexpr std::unsigned_integral auto
  operator()(std::variant<Ts...> const &v) const
      noexcept((std::is_nothrow_invocable_v<version_fn_ const, Ts const &> &&
                ...)) {
    return std::visit(*this, v);
  }

  constexpr std::unsigned_integral auto
  operator()(std::indirectly_readable auto const &pointer) const {
    return std::invoke(*this, *pointer);
  }
};

void set_version(auto &...) = delete;

struct set_version_fn final {
  template <typename T, std::unsigned_integral Version>
  requires std::unsigned_integral<decltype(T::version)>
  constexpr T &operator()(T &t, Version const version) const noexcept {
    if constexpr (sizeof(Version) > sizeof(decltype(T::version))) {
      assert(
          (version < static_cast<Version>(
                         std::numeric_limits<decltype(T::version)>::max())) &&
          "Version parameter value larger than maximum capable value");
    }
    t.version = version;
    return t;
  }

  template <typename T, std::unsigned_integral Version>
  requires requires(T &t, Version const version) { {t.set_version(version)}; }
  constexpr T &operator()(T &t, Version const version) const
      noexcept(noexcept(t.set_version(version))) {
    t.set_version(version);
    return t;
  }

  template <typename T, std::unsigned_integral Version>
  requires requires(T &t, Version const version) { {set_version(t, version)}; }
  constexpr T &operator()(T &t, Version const version) const
      noexcept(noexcept(set_version(t, version))) {
    set_version(t, version);
    return t;
  }
};

} // namespace version_details

inline namespace version_fn_ {
inline constexpr version_details::version_fn_ version = {};
inline constexpr version_details::set_version_fn set_version = {};
} // namespace version_fn_

namespace concepts {
template <typename T>
concept versioned = std::invocable < decltype(skizzay::cddd::version),
        std::remove_reference_t<T>
const & >
    &&std::unsigned_integral<std::invoke_result_t<
        decltype(skizzay::cddd::version), std::remove_reference_t<T> const &>>;
} // namespace concepts

namespace version_t_details_ {
template <typename> struct impl;

template <concepts::versioned T>
struct impl<T> : std::invoke_result<decltype(skizzay::cddd::version),
                                    std::remove_reference_t<T> const &> {};

template <typename T>
requires(!concepts::versioned<T>) && requires { typename T::version_type; }
&&std::unsigned_integral<typename T::version_type> struct impl<T> {
  using type = typename T::version_type;
};
} // namespace version_t_details_

template <typename... Ts>
using version_t =
    std::common_type_t<typename version_t_details_::impl<Ts>::type...>;

} // namespace skizzay::cddd
