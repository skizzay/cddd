#pragma once

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
    {t.emplace_back(static_cast<DomainEvent &&>(domain_event))};
  }
  constexpr void operator()(T &t, DomainEvent &&domain_event) const noexcept(
      noexcept(t.emplace_back(std::forward<DomainEvent>(domain_event)))) {
    t.emplace_back(std::forward<DomainEvent>(domain_event));
  }

  template <typename T, concepts::domain_event DomainEvent>
  requires requires(T &t, DomainEvent const &domain_event) {
    {t.push_back(domain_event)};
  } &&(!requires(T & t, DomainEvent &&domain_event) {
    {t.emplace_back(static_cast<DomainEvent &&>(domain_event))};
  }) constexpr void
  operator()(T &t, DomainEvent &&domain_event) const
      noexcept(noexcept(t.push_back(domain_event))) {
    t.push_back(domain_event);
  }

  template <typename T, concepts::domain_event DomainEvent>
  requires requires(T &t, DomainEvent domain_event) {
    {add_event(t, std::move(domain_event))};
  }
  constexpr void operator()(T &t, DomainEvent &&domain_event) const
      noexcept(noexcept(add_event(t,
                                  std::forward<DomainEvent>(domain_event)))) {
    add_event(t, std::forward<DomainEvent>(domain_event));
  }

  template <typename T, concepts::domain_event DomainEvent>
  requires requires(T &t, DomainEvent domain_event) {
    {t.add_event(std::move(domain_event))};
  }
  constexpr void operator()(T &t, DomainEvent &&domain_event) const
      noexcept(noexcept(t.add_event(std::forward<DomainEvent>(domain_event)))) {
    t.add_event(std::forward<DomainEvent>(domain_event));
  }

  template <std::indirectly_readable Pointer,
            concepts::domain_event DomainEvent>
  requires std::invocable<add_event_fn const,
                          std::add_lvalue_reference_t<Pointer>, DomainEvent>
  constexpr void operator()(Pointer &ptr, DomainEvent &&domain_event) const
      noexcept(std::is_nothrow_invocable_v<add_event_fn const,
                                           std::add_lvalue_reference_t<Pointer>,
                                           DomainEvent>) {
    (*this)(*ptr, std::forward<DomainEvent>(domain_event));
  }
};

template <typename T> struct supports_add_event {
  template <concepts::domain_event DomainEvent>
  using test = std::is_invocable<add_event_fn const &,
                                 std::add_lvalue_reference_t<T>, DomainEvent>;
};

template <typename Transform> struct map_helper {
  template <typename T> using type = std::invoke_result_t<Transform, T>;
};

template <concepts::domain_event_sequence DomainEvents, typename Transform>
using buffered_value_t = typename DomainEvents::template mapped_type<map_helper<
    Transform>::template type>::template reduced_type<std::common_type>;

template <concepts::domain_event_sequence DomainEvents, typename Transform>
using default_alloc_t =
    std::allocator<buffered_value_t<DomainEvents, Transform>>;
} // namespace event_stream_buffer_details_

inline namespace event_stream_buffer_fn_ {
inline constexpr event_stream_buffer_details_::add_event_fn add_event = {};
}

namespace concepts {
template <typename T, typename DomainEvents>
concept event_stream_buffer =
    std::ranges::sized_range<T> && DomainEvents::template all<
        event_stream_buffer_details_::supports_add_event<T>::template test>;
}

template <concepts::domain_event_sequence DomainEvents, typename Transform,
          typename Alloc = event_stream_buffer_details_::default_alloc_t<
              DomainEvents, Transform>>
struct mapped_event_stream_buffer
    : std::ranges::view_interface<
          mapped_event_stream_buffer<DomainEvents, Transform, Alloc>> {
  constexpr void clear() { buffer_.clear(); }
  constexpr void reserve(std::size_t const size) { buffer_.reserve(size); }

  constexpr auto begin() { return std::ranges::begin(buffer_); }
  constexpr auto end() { return std::ranges::end(buffer_); }
  constexpr auto begin() const { return std::ranges::begin(buffer_); }
  constexpr auto end() const { return std::ranges::end(buffer_); }

  constexpr void
  emplace_back(concepts::domain_event_of<DomainEvents> auto &&domain_event) {
    buffer_.emplace_back(std::invoke(transform_, std::move(domain_event)));
  }

  constexpr void
  push_back(concepts::domain_event_of<DomainEvents> auto const &domain_event) {
    buffer_.emplace_back(std::invoke(transform_, domain_event));
  }

private:
  std::vector<
      event_stream_buffer_details_::buffered_value_t<DomainEvents, Transform>,
      Alloc>
      buffer_;
  [[no_unique_address]] Transform transform_;
};

} // namespace skizzay::cddd
