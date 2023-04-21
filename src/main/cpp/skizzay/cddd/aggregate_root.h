#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/domain_event_wrapper.h"
#include "skizzay/cddd/event_sourced.h"
#include "skizzay/cddd/event_stream_buffer.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/version.h"

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

template <typename...> struct aggregate_root_of_impl;

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
inline constexpr skizzay::cddd::aggregate_root_details_::uncommitted_events_fn
    uncommitted_events = {};
inline constexpr skizzay::cddd::aggregate_root_details_::
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

// template <typename Derived, concepts::domain_event DomainEvent>
// struct aggregate_visitor_impl : virtual event_visitor_interface<DomainEvent>
// {
//   void visit(DomainEvent const &domain_event) override {
//     skizzay::cddd::apply_event(static_cast<Derived *>(this)->get_aggregate(),
//                                domain_event);
//   }
// };

// template <typename, typename> struct aggregate_visitor;

// template <concepts::domain_event... DomainEvents,
//           concepts::aggregate_root_of<domain_event_sequence<DomainEvents...>>
//               Aggregate>
// struct aggregate_visitor<domain_event_sequence<DomainEvents...>, Aggregate>
//     final
//     : virtual event_visitor<domain_event_sequence<DomainEvents...>>,
//       aggregate_visitor_impl<
//           aggregate_visitor<domain_event_sequence<DomainEvents...>,
//           Aggregate>, DomainEvents>... {
//   aggregate_visitor(Aggregate &aggregate) noexcept : aggregate_{aggregate} {}

//   Aggregate &get_aggregate() noexcept { return aggregate_; }

// private:
//   Aggregate &aggregate_;
// };

// template <concepts::domain_event_sequence DomainEvents,
//           concepts::aggregate_root_of<DomainEvents> AggregateRoot>
// constexpr aggregate_visitor<DomainEvents, AggregateRoot>
// as_event_visitor(AggregateRoot &aggregate) {
//   return {aggregate};
// }

} // namespace skizzay::cddd
