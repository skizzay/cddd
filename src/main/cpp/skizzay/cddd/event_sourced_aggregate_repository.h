#pragma once

#include "skizzay/cddd/aggregate_root.h"
#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/event_sourced.h"
#include "skizzay/cddd/event_store.h"
#include "skizzay/cddd/factory.h"
#include "skizzay/cddd/nullable.h"

namespace skizzay::cddd {

namespace event_sourced_aggregate_repository_details_ {
template <typename F, typename T>
concept aggregate_provider =
    concepts::factory<F, T> || concepts::factory<F, T, id_t<T>>;
}

template <concepts::domain_event_sequence DomainEvents,
          concepts::aggregate_root<DomainEvents> AggregateRoot,
          concepts::event_store_of<DomainEvents> EventStore,
          event_sourced_aggregate_repository_details_::aggregate_provider<
              AggregateRoot>
              AggregateProvider = nonnull_default_factory<AggregateRoot>>
struct event_sourced_aggregate_repository {
  constexpr concepts::nullable_for<AggregateRoot>
  get(id_t<AggregateRoot> id_value) {
    concepts::nullable_for<AggregateRoot> aggregate =
        create_aggregate(id_value);
    concepts::event_source_for<DomainEvents, AggregateRoot> auto event_source =
        get_event_source(event_store_);
    load_from_history(event_source, *aggregate);
    return aggregate;
  }

private:
  constexpr concepts::nullable_for<AggregateRoot> auto
  create_aggregate(id_t<AggregateRoot> id_value) {
    if constexpr (std::invocable<AggregateProvider, id_t<AggregateRoot>>) {
      return create_aggregate_(id_value);
    } else {
      return create_aggregate_();
      l
    }
  }

  EventStore event_store_;
  AggregateProvider create_aggregate_;
};
} // namespace skizzay::cddd
