#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/domain_event_wrapper.h"
#include "skizzay/cddd/event_sourced.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/version.h"

namespace skizzay::cddd {
namespace concepts {

namespace aggregate_root_details_ {
template <typename T> struct can_apply_event {
  template <domain_event DomainEvent>
  using test = std::is_invocable<decltype(skizzay::cddd::apply),
                                 std::add_lvalue_reference_t<T>,
                                 std::remove_reference_t<DomainEvent> const &>;
};
} // namespace aggregate_root_details_

template <typename T, typename DomainEvents>
concept aggregate_root =
    versioned<T> && identifiable<T> && domain_event_sequence<DomainEvents> &&
    (!DomainEvents::empty) &&
    std::same_as<std::remove_cvref_t<id_t<T>>, id_t<DomainEvents>>
        &&std::same_as<version_t<T>, version_t<DomainEvents>>
            &&DomainEvents::template all<
                aggregate_root_details_::can_apply_event<T>::template test>;

template <typename T, typename DomainEvents, typename AggregateRoot>
concept event_source_for = aggregate_root<AggregateRoot, DomainEvents> &&
    std::invocable<decltype(skizzay::cddd::load_from_history),
                   std::add_lvalue_reference_t<T>,
                   std::add_lvalue_reference_t<AggregateRoot>>;
} // namespace concepts

template <typename Derived, concepts::domain_event DomainEvent>
struct aggregate_visitor_impl : virtual event_visitor_interface<DomainEvent> {
  void visit(DomainEvent const &domain_event) override {
    using skizzay::cddd::apply;
    apply(static_cast<Derived *>(this)->get_aggregate(), domain_event);
  }
};

template <typename, typename> struct aggregate_visitor;

template <
    concepts::domain_event... DomainEvents,
    concepts::aggregate_root<domain_event_sequence<DomainEvents...>> Aggregate>
struct aggregate_visitor<domain_event_sequence<DomainEvents...>, Aggregate>
    final
    : virtual event_visitor<domain_event_sequence<DomainEvents...>>,
      aggregate_visitor_impl<
          aggregate_visitor<domain_event_sequence<DomainEvents...>, Aggregate>,
          DomainEvents>... {
  aggregate_visitor(Aggregate &aggregate) noexcept : aggregate_{aggregate} {}

  Aggregate &get_aggregate() noexcept { return aggregate_; }

private:
  Aggregate &aggregate_;
};

template <concepts::domain_event_sequence DomainEvents,
          concepts::aggregate_root<DomainEvents> AggregateRoot>
aggregate_visitor<DomainEvents, AggregateRoot>
as_event_visitor(AggregateRoot &aggregate) {
  return {aggregate};
}

} // namespace skizzay::cddd
