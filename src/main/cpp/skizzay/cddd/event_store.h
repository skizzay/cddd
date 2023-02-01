#pragma once

#include "skizzay/cddd/event_sourced.h"
#include "skizzay/cddd/event_stream.h"

namespace skizzay::cddd {
namespace event_store_details_ {
void get_event_stream(auto &, auto const &) = delete;

struct get_event_stream_fn final {
  template <typename T>
  requires requires(T &t) {
    { t.get_event_stream() } -> concepts::event_stream;
  }
  constexpr auto operator()(T &t) const
      noexcept(noexcept(t.get_event_stream())) {
    return t.get_event_stream();
  }

  template <typename T, typename Id>
  requires requires(std::remove_reference_t<T> &t) {
    { get_event_stream(t) } -> concepts::event_stream;
  }
  constexpr auto operator()(std::remove_reference_t<T> &t) const
      noexcept(noexcept(get_event_stream(t))) {
    return get_event_stream(t);
  }
};

void get_event_source(auto &, auto const &) = delete;

struct get_event_source_fn final {
  template <typename T, typename Id>
  requires requires(T &t, Id const &id) {
    { t.get_event_source(id) } -> std::destructible;
  }
  constexpr auto operator()(T &t, Id const &id) const {
    return t.get_event_source(id);
  }

  template <typename T, typename Id>
  requires requires(T &t, Id const &id) {
    { get_event_source(t, id) } -> std::destructible;
  }
  constexpr auto operator()(T &t, Id const &id) const {
    return get_event_source(t, id);
  }
};
} // namespace event_store_details_

inline namespace event_store_fn_ {
inline constexpr event_store_details_::get_event_stream_fn get_event_stream =
    {};
inline constexpr event_store_details_::get_event_source_fn get_event_source =
    {};
} // namespace event_store_fn_

namespace concepts {
template <typename T>
concept event_store = requires {
  typename id_t<T>;
}
&&std::invocable<decltype(skizzay::cddd::get_event_stream),
                 std::add_lvalue_reference_t<T>, id_t<T>>
    &&std::invocable<decltype(skizzay::cddd::get_event_source),
                     std::add_lvalue_reference_t<T>, id_t<T>>;
} // namespace concepts

} // namespace skizzay::cddd
