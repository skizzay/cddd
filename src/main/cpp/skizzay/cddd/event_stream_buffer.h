#pragma once

#include "skizzay/cddd/dereference.h"
#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/domain_event_sequence.h"

#include <concepts>
#include <ranges>
#include <type_traits>
#include <vector>

namespace skizzay::cddd {

namespace event_stream_buffer_details_ {

template <typename... Ts> void add_event(Ts const &...) = delete;

struct add_event_fn final {
  template <typename T, concepts::domain_event DomainEvent>
  requires requires(T &t, DomainEvent &&domain_event) {
    {dereference(t).emplace_back(static_cast<DomainEvent &&>(domain_event))};
  }
  constexpr void operator()(T &t, DomainEvent &&domain_event) const
      noexcept(noexcept(dereference(t).emplace_back(
          std::forward<DomainEvent>(domain_event)))) {
    dereference(t).emplace_back(std::forward<DomainEvent>(domain_event));
  }

  template <typename T, concepts::domain_event DomainEvent>
  requires requires(T &t, DomainEvent const &domain_event) {
    {dereference(t).push_back(domain_event)};
  } &&(!requires(T & t, DomainEvent &&domain_event) {
    {dereference(t).emplace_back(static_cast<DomainEvent &&>(domain_event))};
  }) constexpr void
  operator()(T &t, DomainEvent &&domain_event) const
      noexcept(noexcept(dereference(t).push_back(domain_event))) {
    dereference(t).push_back(domain_event);
  }

  template <typename T, concepts::domain_event DomainEvent>
  requires requires(T &t, DomainEvent domain_event) {
    {add_event(dereference(t), std::move(domain_event))};
  }
  constexpr void operator()(T &t, DomainEvent &&domain_event) const
      noexcept(noexcept(add_event(dereference(t),
                                  std::forward<DomainEvent>(domain_event)))) {
    add_event(dereference(t), std::forward<DomainEvent>(domain_event));
  }

  template <typename T, concepts::domain_event DomainEvent>
  requires requires(T &t, DomainEvent domain_event) {
    {dereference(t).add_event(std::move(domain_event))};
  }
  constexpr void operator()(T &t, DomainEvent &&domain_event) const
      noexcept(noexcept(
          dereference(t).add_event(std::forward<DomainEvent>(domain_event)))) {
    dereference(t).add_event(std::forward<DomainEvent>(domain_event));
  }
};

template <typename T> struct supports_add_event {
  template <concepts::domain_event DomainEvent>
  using test = std::is_invocable<add_event_fn const &,
                                 std::add_lvalue_reference_t<T>, DomainEvent>;
};

template <typename Transform> struct map_helper {
  template <typename T> using type = std::invoke_result<Transform, T>;
};

template <concepts::domain_event_sequence DomainEvents, typename Transform>
using buffered_value_t = typename DomainEvents::template mapped_type<map_helper<
    Transform>::template type>::template reduced_type<std::common_type>::type;

template <concepts::domain_event_sequence DomainEvents, typename Transform>
using default_alloc_t =
    std::allocator<buffered_value_t<DomainEvents, Transform>>;

template <typename T> void get_event_stream_buffer(T &&t) = delete;

struct get_event_stream_buffer_fn final {
  template <typename T>
  requires requires(T &t) {
    { dereference(t).get_event_stream_buffer() } -> std::ranges::sized_range;
  }
  constexpr std::ranges::sized_range auto operator()(T const &t) const
      noexcept(noexcept(dereference(t).get_event_stream_buffer())) {
    return dereference(t).get_event_stream_buffer();
  }

  template <typename T>
  requires requires(T &t) {
    { get_event_stream_buffer(dereference(t)) } -> std::ranges::sized_range;
  }
  constexpr std::ranges::sized_range auto operator()(T const &t) const
      noexcept(noexcept(get_event_stream_buffer(dereference(t)))) {
    return get_event_stream_buffer(dereference(t));
  }
};
} // namespace event_stream_buffer_details_

inline namespace event_stream_buffer_fn_ {
inline constexpr event_stream_buffer_details_::add_event_fn add_event = {};
inline constexpr event_stream_buffer_details_::get_event_stream_buffer_fn
    get_event_stream_buffer = {};
} // namespace event_stream_buffer_fn_

template <typename T>
using event_stream_buffer_t = std::remove_cvref_t<std::invoke_result_t<
    event_stream_buffer_details_::get_event_stream_buffer_fn const,
    std::add_lvalue_reference_t<T>>>;

namespace concepts {
template <typename T>
concept event_stream_buffer = std::ranges::sized_range<T>;

template <typename T, typename DomainEvents>
concept event_stream_buffer_of =
    event_stream_buffer<T> && DomainEvents::template all<
        event_stream_buffer_details_::supports_add_event<T>::template test>;
} // namespace concepts

template <typename Value, std::copy_constructible Transform,
          typename Alloc = std::allocator<Value>>
struct mapped_event_stream_buffer
    : std::ranges::view_interface<
          mapped_event_stream_buffer<Value, Transform, Alloc>> {
  explicit mapped_event_stream_buffer(Transform transform = {})
      : transform_{std::move(transform)} {}

  constexpr void clear() { buffer_.clear(); }
  constexpr void reserve(std::size_t const size) { buffer_.reserve(size); }

  constexpr auto begin() { return std::ranges::begin(buffer_); }
  constexpr auto end() { return std::ranges::end(buffer_); }
  constexpr auto begin() const { return std::ranges::begin(buffer_); }
  constexpr auto end() const { return std::ranges::end(buffer_); }
  constexpr auto size() const noexcept { return std::ranges::size(buffer_); }

  template <concepts::domain_event DomainEvent>
  requires std::invocable<Transform, DomainEvent &&> &&
      std::constructible_from<Value,
                              std::invoke_result_t<Transform, DomainEvent &&>>
  constexpr void emplace_back(DomainEvent &&domain_event) {
    buffer_.emplace_back(std::invoke(transform_, std::move(domain_event)));
  }

  constexpr void emplace_back(Value &&value) { buffer_.emplace_back(value); }

  template <concepts::domain_event DomainEvent>
  requires std::invocable<Transform, DomainEvent const &> &&
      std::constructible_from<
          Value, std::invoke_result_t<Transform, DomainEvent const &>>
  constexpr void push_back(DomainEvent const &domain_event) {
    buffer_.emplace_back(std::invoke(transform_, domain_event));
  }

  constexpr void push_back(Value const &value) { buffer_.push_back(value); }

private:
  std::vector<Value, Alloc> buffer_;
  [[no_unique_address]] Transform transform_;
};

} // namespace skizzay::cddd
