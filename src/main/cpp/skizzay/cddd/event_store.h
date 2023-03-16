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

  template <typename T>
  requires requires(std::remove_reference_t<T> &t) {
    { get_event_stream(t) } -> concepts::event_stream;
  }
  constexpr auto operator()(std::remove_reference_t<T> &t) const
      noexcept(noexcept(get_event_stream(t))) {
    return get_event_stream(t);
  }

  template <std::indirectly_readable Pointer>
  requires std::invocable<
      get_event_stream_fn const,
      typename std::indirectly_readable_traits<Pointer>::reference>
  constexpr concepts::event_stream auto operator()(Pointer &ptr) const
      noexcept(std::is_nothrow_invocable_v<
               get_event_stream_fn const,
               typename std::indirectly_readable_traits<Pointer>::reference>) {
    return (*this)(*ptr);
  }
};

void get_event_source(auto &, auto const &) = delete;

struct get_event_source_fn final {
  template <typename T>
  requires requires(T &t) {
    { t.get_event_source() } -> std::destructible;
  }
  constexpr auto operator()(T &t) const { return t.get_event_source(); }

  template <typename T>
  requires requires(T &t) {
    { get_event_source(t) } -> std::destructible;
  }
  constexpr auto operator()(T &t) const { return get_event_source(t); }
};

template <typename T, typename DomainEvents>
concept provides_event_stream_of =
    std::invocable<get_event_stream_fn const &, T &> &&
    concepts::domain_event_sequence<DomainEvents> && concepts::event_stream_of<
        std::invoke_result_t<get_event_stream_fn const &, T &>, DomainEvents>;
} // namespace event_store_details_

inline namespace event_store_fn_ {
inline constexpr event_store_details_::get_event_stream_fn get_event_stream =
    {};
inline constexpr event_store_details_::get_event_source_fn get_event_source =
    {};
} // namespace event_store_fn_

namespace concepts {
template <typename T>
concept event_store = std::invocable<decltype(skizzay::cddd::get_event_stream),
                                     std::add_lvalue_reference_t<T>> &&
    std::invocable<decltype(skizzay::cddd::get_event_source),
                   std::add_lvalue_reference_t<T>> &&
    std::invocable<decltype(skizzay::cddd::get_event_stream_buffer),
                   std::add_lvalue_reference_t<T>>;

template <typename T, typename DomainEvents>
concept event_store_of =
    event_store<T> && domain_event_sequence<DomainEvents> &&
    event_store_details_::provides_event_stream_of<T, DomainEvents>;
} // namespace concepts

} // namespace skizzay::cddd
