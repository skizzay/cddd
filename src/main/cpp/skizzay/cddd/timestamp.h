#pragma once

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
    { timestamp(tc) } -> concepts::timestamp;
  }
  constexpr concepts::timestamp auto operator()(T const &tc) const
      noexcept(noexcept(timestamp(tc))) {
    return timestamp(tc);
  }

  template <typename T>
  requires requires(T const &tc) {
    { tc.timestamp() } -> concepts::timestamp;
  }
  constexpr concepts::timestamp auto operator()(T const &tc) const
      noexcept(noexcept(tc.timestamp())) {
    return tc.timestamp();
  }

  template <typename T>
  requires concepts::timestamp<decltype(T::timestamp)>
  constexpr concepts::timestamp auto operator()(T const &tc) const noexcept {
    return tc.timestamp;
  }

  template <std::indirectly_readable Pointer>
  constexpr concepts::timestamp auto operator()(Pointer const &pointer) const
      noexcept(
          std::is_nothrow_invocable_v<timestamp_fn const, decltype(*pointer)>) {
    return std::invoke(*this, *pointer);
  }

  template <typename... Ts>
  constexpr concepts::timestamp auto
  operator()(std::variant<Ts...> const &v) const
      noexcept((std::is_nothrow_invocable_v<timestamp_fn const, Ts const &> &&
                ...)) {
    return std::visit(*this, v);
  }
};

void set_timestamp(auto &...) = delete;

struct set_timestamp_fn final {
  template <typename T, concepts::timestamp Timestamp>
  requires concepts::timestamp<decltype(T::timestamp)>
  constexpr T &operator()(T &t, Timestamp const timestamp) const noexcept {
    t.timestamp =
        std::chrono::time_point_cast<typename decltype(T::timestamp)::duration>(
            timestamp);
    return t;
  }

  template <typename T, concepts::timestamp Timestamp>
  requires requires(T &t, Timestamp const timestamp) {
    {t.set_timestamp(timestamp)};
  }
  constexpr T &operator()(T &t, Timestamp const timestamp) const
      noexcept(noexcept(t.set_timestamp(timestamp))) {
    t.set_timestamp(timestamp);
    return t;
  }

  template <typename T, concepts::timestamp Timestamp>
  requires requires(T &t, Timestamp const timestamp) {
    {set_timestamp(t, timestamp)};
  }
  constexpr T &operator()(T &t, Timestamp const timestamp) const
      noexcept(noexcept(set_timestamp(t, timestamp))) {
    set_timestamp(t, timestamp);
    return t;
  }
};

void now(auto &...) = delete;

struct now_fn final {
  template <typename T>
  requires std::invocable<decltype(T::now)> &&
      concepts::timestamp<std::invoke_result_t<decltype(T::now)>>
  constexpr auto operator()(T &) const { return T::now(); }

  template <typename T>
  requires std::invocable<decltype(&T::now), T &> &&
      concepts::timestamp<std::invoke_result_t<decltype(&T::now), T &>>
  constexpr concepts::timestamp auto operator()(T &t) const
      noexcept(std::is_nothrow_invocable_v<decltype(&T::now), T &>) {
    return t.now();
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
