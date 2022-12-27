#pragma once

#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/timestamp.h"
#include "skizzay/cddd/version.h"
#include <concepts>
#include <ranges>
#include <type_traits>

namespace skizzay::cddd {

namespace concepts {

template <typename T>
concept domain_event =
    std::is_class_v<T> && identifiable<T> && versioned<T> && timestamped<T>;

template <typename T>
concept mutable_domain_event = domain_event<T> &&
    requires(T &t, id_t<T> i, timestamp_t<T> ts, version_t<T> v) {
  {skizzay::cddd::set_id(t, i)};
  {skizzay::cddd::set_timestamp(t, ts)};
  {skizzay::cddd::set_version(t, v)};
};

template <typename T>
concept domain_event_range =
    std::ranges::range<T> && domain_event<std::ranges::range_value_t<T>>;
} // namespace concepts

template <typename, typename, typename, typename> struct basic_domain_event;

template <typename Tag, concepts::identifier Id, std::unsigned_integral Version,
          concepts::timestamp Timestamp>
struct basic_domain_event<Tag, Id, Version, Timestamp> {
  Id id = {};
  Version version = {};
  Timestamp timestamp = {};
};

template <concepts::domain_event DomainEvent> struct event_visitor_interface {
  virtual void visit(DomainEvent const &domain_event) = 0;
};

template <concepts::domain_event... DomainEvents>
struct event_visitor
    : virtual event_visitor_interface<std::remove_cvref_t<DomainEvents>>... {};

template <concepts::domain_event... DomainEvents> struct event_holder_impl;

template <concepts::domain_event... DomainEvents> struct event_interface {
  using id_type = std::add_const_t<id_t<DomainEvents...>>;
  using version_type = version_t<DomainEvents...>;
  using timestamp_type = timestamp_t<DomainEvents...>;

  virtual id_type id() const noexcept = 0;
  virtual version_type version() const noexcept = 0;
  virtual timestamp_type timestamp() const noexcept = 0;
  virtual void set_id(id_type) = 0;
  virtual void set_version(version_type const) noexcept = 0;
  virtual void set_timestamp(timestamp_type const) noexcept = 0;
  inline event_visitor<DomainEvents...> &
  accept_event_visitor(event_visitor<DomainEvents...> &visitor) const {
    this->do_accept_event_visitor(visitor);
    return visitor;
  }

  template <concepts::domain_event DomainEvent>
  requires(
      std::same_as<std::remove_cvref_t<DomainEvent>, DomainEvents> ||
      ...) static inline event_holder_impl<std::remove_cvref_t<DomainEvent>,
                                           DomainEvents...> from_domain_event(DomainEvent
                                                                                  &&domain_event);

private:
  virtual void
  do_accept_event_visitor(event_visitor<DomainEvents...> &) const = 0;
};

template <concepts::domain_event DomainEvent,
          concepts::domain_event... DomainEvents>
struct event_holder_impl<DomainEvent, DomainEvents...> final
    : public event_interface<DomainEvents...> {

  explicit event_holder_impl(
      std::remove_reference_t<DomainEvent>
          &&domain_event) noexcept(std::
                                       is_nothrow_move_constructible_v<
                                           std::remove_cvref_t<DomainEvent>>)
      : event{std::forward<decltype(domain_event)>(domain_event)} {}

  typename event_interface<DomainEvents...>::id_type
  id() const noexcept override {
    using skizzay::cddd::id;
    return id(event);
  }

  typename event_interface<DomainEvents...>::version_type
  version() const noexcept override {
    using skizzay::cddd::version;
    return version(event);
  }

  typename event_interface<DomainEvents...>::timestamp_type
  timestamp() const noexcept override {
    using skizzay::cddd::timestamp;
    return timestamp(event);
  }

  void set_id(typename event_interface<DomainEvents...>::id_type id) override {
    using skizzay::cddd::set_id;
    set_id(event, id);
  }

  void set_version(typename event_interface<DomainEvents...>::version_type const
                       version) noexcept override {
    using skizzay::cddd::set_version;
    set_version(event, version);
  }

  void
  set_timestamp(typename event_interface<DomainEvents...>::timestamp_type const
                    timestamp) noexcept override {
    using skizzay::cddd::set_timestamp;
    set_timestamp(event, timestamp);
  }

  std::remove_cvref_t<DomainEvent> event;

private:
  void do_accept_event_visitor(
      event_visitor<DomainEvents...> &visitor) const override {
    static_cast<event_visitor_interface<std::remove_cvref_t<DomainEvent>> &>(
        visitor)
        .visit(event);
  }
};

template <concepts::domain_event... DomainEvents>
template <concepts::domain_event DomainEvent>
requires(
    std::same_as<std::remove_cvref_t<DomainEvent>, DomainEvents> ||
    ...) inline event_holder_impl<std::remove_cvref_t<DomainEvent>,
                                  DomainEvents...> event_interface<DomainEvents...>::
    from_domain_event(DomainEvent &&domain_event) {
  return {std::forward<DomainEvent>(domain_event)};
}

template <concepts::domain_event> struct event_type {};

} // namespace skizzay::cddd