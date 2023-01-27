#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/domain_event_sequence.h"

namespace skizzay::cddd {
template <concepts::domain_event DomainEvent> struct event_visitor_interface {
  virtual void visit(DomainEvent const &domain_event) = 0;
};

template <typename> struct event_visitor;

template <concepts::domain_event... DomainEvents>
struct event_visitor<domain_event_sequence<DomainEvents...>>
    : virtual event_visitor_interface<DomainEvents>... {};
} // namespace skizzay::cddd
