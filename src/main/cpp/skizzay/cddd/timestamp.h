#pragma once

#include "skizzay/cddd/dereference.h"
#include <chrono>
#include <type_traits>

namespace skizzay::cddd {
template <typename> struct is_time_point : std::false_type {};

template <typename T>
constexpr inline bool is_time_point_v = is_time_point<T>::value;

namespace concepts {
template <typename T>
concept timestamp = is_time_point_v<T>;
} // namespace concepts

template <typename Clock, typename Duration>
struct is_time_point<std::chrono::time_point<Clock, Duration>>
    : std::true_type {};

namespace timestamp_details_ {
template <typename... Ts> void timestamp(Ts const &...) = delete;

struct timestamp_fn final {
  template <typename T>
  requires requires(T const &tc) {
    { timestamp(dereference(tc)) } -> concepts::timestamp;
  }
  constexpr concepts::timestamp auto operator()(T const &tc) const
      noexcept(noexcept(timestamp(dereference(tc)))) {
    return timestamp(dereference(tc));
  }

  template <typename T>
  requires requires(T const &tc) {
    { dereference(tc).timestamp() } -> concepts::timestamp;
  }
  constexpr concepts::timestamp auto operator()(T const &tc) const
      noexcept(noexcept(dereference(tc).timestamp())) {
    return dereference(tc).timestamp();
  }

  template <typename T>
  requires concepts::timestamp<
      decltype(std::remove_cvref_t<dereference_t<T>>::timestamp)>
  constexpr concepts::timestamp auto operator()(T const &t) const noexcept {
    return (dereference(t).timestamp);
  }

  template <typename... Ts>
  requires(
      std::invocable<timestamp_fn const, std::remove_reference_t<Ts> const &>
          &&...) constexpr concepts::timestamp auto
  operator()(std::variant<Ts...> const &v) const
      noexcept((std::is_nothrow_invocable_v<timestamp_fn const, Ts const &> &&
                ...)) {
    return std::visit(*this, v);
  }
};

void set_timestamp(auto &...) = delete;

struct set_timestamp_fn final {
  template <typename T, concepts::timestamp Timestamp>
  requires requires(T &t, Timestamp const timestamp_value) {
    {dereference(t).timestamp = std::chrono::time_point_cast<
         typename decltype(std::remove_cvref_t<
                           dereference_t<T>>::timestamp)::duration>(
         timestamp_value)};
  }
  constexpr T &operator()(T &t, Timestamp const timestamp_value) const noexcept(
      noexcept(dereference(t).timestamp = std::chrono::time_point_cast<
                   typename decltype(std::remove_cvref_t<
                                     dereference_t<T>>::timestamp)::duration>(
                   timestamp_value))) {
    dereference(t).timestamp = std::chrono::time_point_cast<
        typename decltype(std::remove_cvref_t<
                          dereference_t<T>>::timestamp)::duration>(
        timestamp_value);
    return t;
  }

  template <typename T, concepts::timestamp Timestamp>
  requires requires(T &t, Timestamp const timestamp) {
    {dereference(t).set_timestamp(timestamp)};
  }
  constexpr T &operator()(T &t, Timestamp const timestamp) const
      noexcept(noexcept(dereference(t).set_timestamp(timestamp))) {
    dereference(t).set_timestamp(timestamp);
    return t;
  }

  template <typename T, concepts::timestamp Timestamp>
  requires requires(T &t, Timestamp const timestamp) {
    {set_timestamp(dereference(t), timestamp)};
  }
  constexpr T &operator()(T &t, Timestamp const timestamp) const
      noexcept(noexcept(set_timestamp(dereference(t), timestamp))) {
    set_timestamp(dereference(t), timestamp);
    return t;
  }

  template <typename... Ts, concepts::timestamp Timestamp>
  requires(std::invocable<set_timestamp_fn const, Ts &, Timestamp>
               &&...) constexpr decltype(auto)
  operator()(std::variant<Ts...> &t, Timestamp const timestamp) const noexcept(
      (std::is_nothrow_invocable_v<set_timestamp_fn const, Ts &, Timestamp> &&
       ...)) {
    std::visit([this, timestamp](auto &t) { (*this)(t, timestamp); }, t);
    return t;
  }
};

void now(auto &...) = delete;

struct now_fn final {
  template <typename T>
  requires requires(T t) {
    { dereference(t).now() } -> concepts::timestamp;
  }
  constexpr concepts::timestamp auto operator()(T &t) const
      noexcept(noexcept(dereference(t).now())) {
    return dereference(t).now();
  }
};
} // namespace timestamp_details_

inline namespace timestamp_fn_ {
inline constexpr timestamp_details_::timestamp_fn timestamp = {};
inline constexpr timestamp_details_::set_timestamp_fn set_timestamp = {};
inline constexpr timestamp_details_::now_fn now = {};
} // namespace timestamp_fn_

namespace concepts {
template <typename T>
concept timestamped = std::invocable < decltype(skizzay::cddd::timestamp),
        std::remove_reference_t<T>
const & > ;

template <typename T>
concept clock = std::is_copy_constructible_v<T> && requires(T t) {
  { skizzay::cddd::now(t) } -> concepts::timestamp;
};
} // namespace concepts

namespace timestamp_t_details_ {
template <typename> struct impl;

template <concepts::timestamped T> struct impl<T> {
  using type = std::invoke_result_t<decltype(skizzay::cddd::timestamp),
                                    std::remove_reference_t<T> const &>;
};

template <concepts::clock T> struct impl<T> {
  using type = std::invoke_result_t<decltype(skizzay::cddd::now), T &>;
};

template <typename T>
requires(!concepts::timestamped<T>) && (!concepts::clock<T>)&&requires {
  typename T::timestamp_type;
}
&&concepts::timestamp<typename T::timestamp_type> struct impl<T> {
  using type = typename T::timestamp_type;
};
} // namespace timestamp_t_details_

template <typename... Ts>
using timestamp_t =
    std::common_type_t<typename timestamp_t_details_::impl<Ts>::type...>;

} // namespace skizzay::cddd
