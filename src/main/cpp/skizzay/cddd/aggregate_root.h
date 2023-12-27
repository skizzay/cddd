#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/event_sourced.h"
#include "skizzay/cddd/event_stream_buffer.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/version.h"

#include <utility>

namespace skizzay::cddd {

namespace aggregate_root_details_ {
concepts::event_stream_buffer auto uncommitted_events(auto &&) = delete;

struct uncommitted_events_fn final {
  template <typename T>
  requires requires(T &&t) {
    {
      std::move(dereference(t)).uncommitted_events()
      } -> concepts::event_stream_buffer;
  }
  constexpr concepts::event_stream_buffer auto operator()(T &&t) const
      noexcept(noexcept(std::move(dereference(t)).uncommitted_events())) {
    return std::move(dereference(t)).uncommitted_events();
  }

  template <typename T>
  requires requires(T &&t) {
    {
      uncommitted_events(std::move(dereference(t)))
      } -> concepts::event_stream_buffer;
  }
  constexpr concepts::event_stream_buffer auto operator()(T &&t) const
      noexcept(noexcept(uncommitted_events(std::move(dereference(t))))) {
    return uncommitted_events(std::move(dereference(t)));
  }

  template <typename T>
  requires requires(T &&t) {
    { dereference(t).uncommitted_events } -> concepts::event_stream_buffer;
  }
  constexpr concepts::event_stream_buffer auto
  operator()(T &&t) const noexcept {
    return std::move(dereference(t).uncommitted_events);
  }
};

struct uncommitted_events_size_fn final {
  template <typename T>
  requires requires(T const &t) {
    { dereference(t).uncommitted_events_size() }
    noexcept->concepts::version;
  }
  constexpr concepts::version auto operator()(T const &t) const noexcept {
    return dereference(t).uncommitted_events_size();
  }

  template <typename T>
  requires requires(T const &t) {
    { uncommitted_events_size(dereference(t)) }
    noexcept->concepts::version;
  }
  constexpr concepts::version auto operator()(T const &t) const noexcept {
    return uncommitted_events_size(dereference(t));
  }

  template <typename T>
  requires requires(T const &t) {
    { dereference(t).uncommitted_events } -> concepts::event_stream_buffer;
  }
  constexpr concepts::version auto operator()(T const &t) const noexcept {
    return std::ranges::size(dereference(t).uncommitted_events);
  }
};

template <typename... Ts> struct aggregate_root_of_impl : std::conjunction<aggregate_root_of_impl<Ts>...> {};

template <typename T, concepts::domain_event DomainEvent>
struct aggregate_root_of_impl<T, DomainEvent>
    : can_apply_event<T, DomainEvent> {};

template <typename T, concepts::domain_event... DomainEvents>
requires(0 < sizeof...(DomainEvents)) struct aggregate_root_of_impl<
    T, domain_event_sequence<DomainEvents...>>
    : std::conjunction<aggregate_root_of_impl<T, DomainEvents>...> {
};

} // namespace aggregate_root_details_

inline namespace aggregage_root_fn_ {
inline constexpr aggregate_root_details_::uncommitted_events_fn
    uncommitted_events = {};
inline constexpr aggregate_root_details_::
    uncommitted_events_size_fn uncommitted_events_size = {};
} // namespace aggregage_root_fn_

namespace concepts {

template <typename T>
concept aggregate_root = versioned<T> && identifiable<T>;

template <typename T, typename... DomainEvents>
concept aggregate_root_of =
    aggregate_root<T> &&
    (aggregate_root_details_::aggregate_root_of_impl<T, DomainEvents>::value
         &&...) &&
    std::same_as<id_value_t<T>, id_value_t<DomainEvents...>>
        &&std::same_as<version_t<T>, version_t<DomainEvents...>>;
} // namespace concepts

template <typename Derived, concepts::event_stream_buffer EventStreamBuffer,
          concepts::identifier Id, concepts::version Version>
struct aggregate_root_base {
  using id_type = std::remove_cvref_t<Id>;

  constexpr id_type const &id() const noexcept { return id_; }

  constexpr Version version() const noexcept { return version_; }

  constexpr EventStreamBuffer const &uncommitted_events() const &noexcept {
    return uncommitted_events_;
  }

  constexpr EventStreamBuffer &&uncommitted_events() &&noexcept {
    return std::move(uncommitted_events_);
  }

  constexpr concepts::version auto uncommitted_events_size() const noexcept {
    return std::ranges::size(uncommitted_events_);
  }

  template <concepts::domain_event DomainEvent>
  requires concepts::aggregate_root_of<Derived, DomainEvent>
  constexpr void apply_and_add_event(DomainEvent &&event) {
    set_id(event, id_);
    set_version(event, version_ + 1);
    apply_event(*static_cast<Derived *>(this), std::as_const(event));
    add_event(uncommitted_events_, std::forward<decltype(event)>(event));
  }

  constexpr void update(id_type const &id_value,
                        Version const v) noexcept(false) {
    constexpr auto validate = [](std::string const &field, auto const &expected,
                                 auto const &actual) noexcept(false) {
      if (expected != actual) {
        std::ostringstream message;
        message << field << " expected: " << expected << " actual: " << actual;
        throw std::invalid_argument{std::move(message).str()};
      }
    };

    validate("id", id_, id_value);
    validate("version", version_ + 1, v);
    ++version_;
  }

protected:
  constexpr explicit aggregate_root_base(
      id_type id_value, EventStreamBuffer uncommitted_events = {},
      Version v = {}) noexcept(std::is_nothrow_move_constructible_v<id_type>
                                   &&std::is_nothrow_move_constructible_v<
                                       EventStreamBuffer>)
      : id_{std::move(id_value)}, version_{v}, uncommitted_events_{std::move(
                                                   uncommitted_events)} {}

private:
  id_type id_;
  Version version_;
  EventStreamBuffer uncommitted_events_;
};

} // namespace skizzay::cddd
