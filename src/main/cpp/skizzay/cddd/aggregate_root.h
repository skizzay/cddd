#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/domain_event_visitor.h"
#include "skizzay/cddd/event_sourced.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/version.h"

namespace skizzay::cddd {
namespace concepts {
template <typename T, typename DomainEvents>
concept aggregate_root = versioned<T> && identifiable<T> &&
                         domain_event_sequence<DomainEvents> &&
                         (!DomainEvents::empty) &&
                         DomainEvents::template is_handler<T>;
} // namespace concepts

template <typename Derived, concepts::domain_event DomainEvent>
struct aggregate_visitor_impl : virtual event_visitor_interface<DomainEvent> {
  void visit(DomainEvent const &domain_event) override {
    using skizzay::cddd::apply;
    apply(static_cast<Derived *>(this)->get_aggregate(), domain_event);
  }
};

template <typename, typename> struct aggregate_visitor;

template <typename Aggregate, concepts::domain_event... DomainEvents>
requires concepts::aggregate_root<Aggregate,
                                  domain_event_sequence<DomainEvents...>>
struct aggregate_visitor<Aggregate, domain_event_sequence<DomainEvents...>>
    final
    : virtual event_visitor<domain_event_sequence<DomainEvents...>>,
      aggregate_visitor_impl<
          aggregate_visitor<Aggregate, domain_event_sequence<DomainEvents...>>,
          std::remove_cvref_t<DomainEvents>>... {
  aggregate_visitor(Aggregate &aggregate) noexcept : aggregate_{aggregate} {}

  Aggregate &get_aggregate() noexcept { return aggregate_; }

private:
  Aggregate &aggregate_;
};

template <typename Aggregate, concepts::domain_event_sequence DomainEvents>
requires concepts::aggregate_root<Aggregate, DomainEvents>
aggregate_visitor(Aggregate &)
->aggregate_visitor<Aggregate, DomainEvents>;

template <concepts::domain_event_sequence DomainEvents,
          concepts::aggregate_root<DomainEvents> AggregateRoot>
aggregate_visitor<AggregateRoot, DomainEvents>
as_event_visitor(AggregateRoot &aggregate) {
  return {aggregate};
}

} // namespace skizzay::cddd
