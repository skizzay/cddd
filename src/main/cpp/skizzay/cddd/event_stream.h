#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/views.h"

#include <algorithm>
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
  template <typename T, std::unsigned_integral V>
  requires requires(T &t, V const v) { {commit_events(t, v)}; }
  constexpr void operator()(T &t, V const v) const
      noexcept(noexcept(commit_events(t, v))) {
    commit_events(t, v);
  }

  template <typename T, std::unsigned_integral V>
  requires requires(T &t, V const v) { {t.commit_events(v)}; }
  constexpr void operator()(T &t, V const v) const
      noexcept(noexcept(t.commit_events(v))) {
    t.commit_events(v);
  }

  template <typename T, std::signed_integral I>
  requires std::invocable<commit_events_fn const,
                          std::add_lvalue_reference_t<T>,
                          std::make_unsigned_t<I>>
  constexpr void operator()(T &t, I const i) const
      noexcept(std::is_nothrow_invocable_v<commit_events_fn const,
                                           std::add_lvalue_reference_t<T>,
                                           std::make_unsigned_t<I>>) {
    (*this)(t, narrow_cast<std::make_unsigned_t<I>>(i));
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
concept event_stream = std::invocable<decltype(skizzay::cddd::rollback),
                                      std::add_lvalue_reference_t<T>>;

template <typename T, typename... DomainEvents>
concept event_stream_of = event_stream<T> &&
    std::invocable<decltype(skizzay::cddd::commit_events),
                   std::add_lvalue_reference_t<T>,
                   version_t<DomainEvents...>> &&
    (... &&domain_event<DomainEvents>)&&(
        ... &&std::invocable<
            decltype(skizzay::cddd::add_event), std::add_lvalue_reference_t<T>,
            std::add_rvalue_reference_t<DomainEvents>>)&&versioned<T>
        &&std::convertible_to<
            decltype(skizzay::cddd::version(std::declval<T const>())),
            version_t<DomainEvents...>>;
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
  virtual version_t<DomainEvents...> version() const = 0;
  virtual void commit_events(version_t<DomainEvents...>) = 0;
  virtual void rollback() = 0;
};

template <concepts::domain_event... DomainEvents>
std::unique_ptr<event_stream_interface<DomainEvents...>>
type_erase(concepts::event_stream_of<DomainEvents...> auto event_stream) {
  struct impl final : event_stream_interface<DomainEvents...>,
                      event_stream_details_::add_event_impl<
                          impl, std::remove_cvref_t<DomainEvents>>... {

    version_t<DomainEvents...> version() const
        noexcept(noexcept(skizzay::cddd::version(this->impl_))) override {
      return skizzay::cddd::version(impl_);
    }

    void commit_events(version_t<DomainEvents...> expected_version) noexcept(
        noexcept(skizzay::cddd::commit_events(this->impl_,
                                              expected_version))) override {
      return skizzay::cddd::commit_events(impl_, expected_version);
    }

    void rollback() noexcept(
        noexcept(skizzay::cddd::rollback(this->impl_))) override {
      return skizzay::cddd::rollback(impl_);
    }

    std::remove_cvref_t<decltype(event_stream)> impl_;
  };

  return std::make_unique<impl>(std::move(event_stream));
}

template <typename Derived, concepts::clock Clock, typename Element,
          concepts::domain_event... DomainEvents>
struct event_stream_base {
  using id_type = id_t<DomainEvents...>;
  using element_type = Element;
  using buffer_type = std::vector<element_type>;
  using version_type = version_t<DomainEvents...>;
  using timestamp_type = timestamp_t<DomainEvents...>;

  template <concepts::mutable_domain_event DomainEvent>
  requires(std::same_as<std::remove_cvref_t<DomainEvent>,
                        std::remove_cvref_t<DomainEvents>> ||
           ...) void add_event(DomainEvent &&domain_event) {
    buffer_.emplace_back(
        derived().make_buffer_element(std::move(domain_event)));
  }

  constexpr void
  commit_events(std::convertible_to<version_type> auto const expected_version) {
    buffer_type buffer = std::exchange(buffer_, buffer_type{});
    if (not std::empty(buffer)) {
      timestamp_t<DomainEvents...> const timestamp = now(clock_);
      for (auto &&[i, element] : views::enumerate(buffer)) {
        version_type const event_version =
            narrow_cast<version_type>(i) + expected_version + 1;
        derived().populate_commit_info(timestamp, event_version, element);
      }
      derived().commit_buffered_events(
          std::move(buffer), timestamp,
          narrow_cast<version_type>(expected_version));
    }
  }

  constexpr void rollback() noexcept { buffer_.clear(); }

  constexpr bool empty() const {
    return 0 == skizzay::cddd::version(std::as_const(derived()));
  }

protected:
  explicit event_stream_base(
      Clock clock,
      typename std::vector<element_type>::size_type reserve_capacity = 25)
      : clock_{std::move_if_noexcept(clock)} {
    buffer_.reserve(reserve_capacity);
  }

private:
  constexpr Derived &derived() noexcept {
    return *static_cast<Derived *>(this);
  }

  [[no_unique_address]] Clock clock_;
  std::vector<element_type> buffer_;
};

} // namespace skizzay::cddd
