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

template <typename... Ts>
using version_t = std::common_type_t<std::invoke_result_t<
    decltype(version), std::remove_reference_t<Ts> const &>...>;

namespace concepts {
template <typename T>
concept versioned = requires {
  typename version_t<T>;
};
} // namespace concepts

} // namespace skizzay::cddd
