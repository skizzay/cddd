#include <skizzay/cddd/event_sourced_aggregate_repository.h>

#include "fakes.h"
#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/event_store.h"
#include "skizzay/cddd/in_memory/in_memory_event_store.h"
#include "skizzay/cddd/repository.h"

#include <catch.hpp>
#include <iostream>

using namespace skizzay::cddd;

SCENARIO("Event-sourced aggregate repository", "[unit][repository]") {
  using domain_events_t = test::fake_event_sequence<2>;
  using event_ptr = std::unique_ptr<event_wrapper<domain_events_t>>;
  using event_stream_buffer_t = mapped_event_stream_buffer<
      event_ptr,
      std::remove_const_t<decltype(wrap_domain_events<domain_events_t>)>>;
  using aggregate_root_t = test::fake_aggregate<event_stream_buffer_t>;

  GIVEN("an event-sourced aggregate repository") {
    [[maybe_unused]] auto target = event_sourced_aggregate_repository{
        in_memory::event_store{domain_events_t{}, test::fake_clock{}},
        default_factory<aggregate_root_t>{}};

    AND_GIVEN("an entity id") {
      std::string entity_id = "entity id";

      WHEN("an aggregate is retrieved from the repository") {
        [[maybe_unused]] aggregate_root_t aggregate_root =
            get(target, entity_id);

        THEN("the provided aggregate has the entity id") {
          REQUIRE(id(aggregate_root) == entity_id);
        }
      }
    }
  }
}