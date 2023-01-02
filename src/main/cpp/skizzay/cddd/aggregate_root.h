#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/event_sourced.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/version.h"

namespace skizzay::cddd {
namespace concepts {
template <typename T, typename... DomainEvents>
concept aggregate_root =
    versioned<T> && identifiable<T> &&(0 != sizeof...(DomainEvents)) &&
    (domain_event<DomainEvents> && ...) &&
    std::same_as<std::remove_cvref_t<id_t<T>>,
                 std::remove_cvref_t<id_t<DomainEvents...>>>
        &&std::same_as<version_t<T>, version_t<DomainEvents...>> &&
    (std::invocable<decltype(skizzay::cddd::apply),
                    std::add_lvalue_reference_t<T>,
                    std::remove_reference_t<DomainEvents> const &> &&
     ...);
} // namespace concepts

template <typename Derived, concepts::domain_event DomainEvent>
struct aggregate_visitor_impl : virtual event_visitor_interface<DomainEvent> {
  void visit(DomainEvent const &domain_event) override {
    using skizzay::cddd::apply;
    apply(static_cast<Derived *>(this)->get_aggregate(), domain_event);
  }
};

template <typename Aggregate, concepts::domain_event... DomainEvents>
requires concepts::aggregate_root<Aggregate, DomainEvents...>
struct aggregate_visitor final
    : virtual event_visitor<DomainEvents...>,
      aggregate_visitor_impl<aggregate_visitor<Aggregate, DomainEvents...>,
                             std::remove_cvref_t<DomainEvents>>... {
  aggregate_visitor(Aggregate &aggregate) noexcept : aggregate_{aggregate} {}

  Aggregate &get_aggregate() noexcept { return aggregate_; }

private:
  Aggregate &aggregate_;
};

template <typename Aggregate, concepts::domain_event... DomainEvents>
requires concepts::aggregate_root<Aggregate, DomainEvents...>
aggregate_visitor(Aggregate &)
->aggregate_visitor<Aggregate, DomainEvents...>;

template <concepts::domain_event... DomainEvents,
          concepts::aggregate_root<DomainEvents...> AggregateRoot>
aggregate_visitor<AggregateRoot, DomainEvents...>
as_event_visitor(AggregateRoot &aggregate) {
  return {aggregate};
}

} // namespace skizzay::cddd
