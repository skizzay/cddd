#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/identifier.h"

#include <concepts>
#include <type_traits>
#include <utility>

namespace skizzay::cddd {
namespace event_stream_details_ {

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

  constexpr void operator()(auto &, auto &&) const noexcept {}
};

template <typename... Ts> void commit_events(Ts const &...) = delete;

struct commit_events_fn final {
  template <typename T>
  requires requires(T &t) { {commit_events(t)}; }
  constexpr void operator()(T &t) const noexcept(noexcept(commit_events(t))) {
    commit_events(t);
  }

  template <typename T>
  requires requires(T &t) { {t.commit_events()}; }
  constexpr void operator()(T &t) const noexcept(noexcept(t.commit_events())) {
    t.commit_events();
  }
};

template <typename... Ts> void rollback(Ts const &...) = delete;

struct rollback_fn final {
  template <typename T>
  requires requires(T &t) { {rollback(t)}; }
  constexpr void operator()(T &t) const noexcept(noexcept(rollback(t))) {
    rollback(t);
  }

  template <typename T>
  requires requires(T &t) { {t.rollback()}; }
  constexpr void operator()(T &t) const noexcept(noexcept(t.rollback())) {
    t.rollback();
  }
};
} // namespace event_stream_details_

inline namespace event_stream_fn_ {
inline constexpr event_stream_details_::add_event_fn add_event = {};
inline constexpr event_stream_details_::commit_events_fn commit_events = {};
inline constexpr event_stream_details_::rollback_fn rollback = {};
} // namespace event_stream_fn_

namespace concepts {
template <typename T>
concept event_stream = std::invocable<decltype(skizzay::cddd::commit_events),
                                      std::add_lvalue_reference_t<T>> &&
    std::invocable<decltype(skizzay::cddd::rollback),
                   std::add_lvalue_reference_t<T>>;

template <typename T, typename... DomainEvents>
concept event_stream_of = event_stream<T> &&
    (... &&domain_event<DomainEvents>)&&(
        ... &&std::invocable<decltype(skizzay::cddd::add_event),
                             std::add_lvalue_reference_t<T>,
                             std::add_rvalue_reference_t<DomainEvents>>);
} // namespace concepts

namespace event_stream_details_ {
template <concepts::domain_event DomainEvent> struct add_event_interace {
  virtual void add_event(DomainEvent &&domain_event) = 0;
};

template <typename Derived, concepts::domain_event DomainEvent>
struct add_event_impl : virtual add_event_interace<DomainEvent> {
  void add_event(DomainEvent &&domain_event) override {
    using skizzay::cddd::add_event;
    add_event(static_cast<Derived *>(this)->get_impl(),
              std::move(domain_event));
  }
};
} // namespace event_stream_details_

template <concepts::domain_event... DomainEvents>
struct event_stream_interface
    : virtual event_stream_details_::add_event_interace<
          std::remove_cvref_t<DomainEvents>>... {
  virtual void commit_events() = 0;
  virtual void rollback() = 0;
};

template <concepts::domain_event... DomainEvents>
std::unique_ptr<event_stream_interface<DomainEvents...>>
type_erase(concepts::event_stream auto &&event_stream) {
  struct impl final : event_stream_interface<DomainEvents...>,
                      event_stream_details_::add_event_impl<
                          impl, std::remove_cvref_t<DomainEvents>>... {
    concepts::event_stream auto &get_impl() noexcept { return impl_; }

    std::remove_cvref_t<decltype(event_stream)> impl_;
  };

  return std::make_unique<impl>(std::move(event_stream));
}

} // namespace skizzay::cddd
