#include <skizzay/cddd/event_sourced_aggregate_repository.h>

#include "fakes.h"
#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/event_store.h"
#include "skizzay/cddd/in_memory/in_memory_event_store.h"
#include "skizzay/cddd/repository.h"

#include <catch2/catch_test_macros.hpp>
#include <iostream>

using namespace skizzay::cddd;

namespace {
using domain_events = test::fake_event_sequence<4>;
using event_t = std::shared_ptr<event_wrapper<domain_events>>;
using buffer_store_t = in_memory::serial_buffer_store<event_t>;

inline constexpr auto create_fake_aggregate = [](auto id_value, auto buffer) {
  return test::fake_aggregate{std::move(id_value), std::move(buffer)};
};

inline constexpr auto create_buffer = []() {
  return mapped_event_stream_buffer<
      event_t,
      std::remove_cvref_t<decltype(wrap_domain_events<domain_events>)>>{
      wrap_domain_events<domain_events>};
};
} // namespace

SCENARIO("Event-sourced aggregate repository", "[unit][repository]") {
  GIVEN("An event-sourced aggregate repository") {
    event_sourced_aggregate_repository target{
        in_memory::event_store{create_buffer, buffer_store_t{},
                               test::fake_clock{}},
        create_fake_aggregate};

    WHEN("an aggregate is retrieved") {
      std::string id_value = "some_id";
      auto aggregate = skizzay::cddd::get(target, id_value);

      THEN("the aggregate didn't exist previously") {
        REQUIRE(0 == skizzay::cddd::version(aggregate));
      }

      THEN("the uncommitted event buffer is empty") {
        REQUIRE(std::ranges::empty(aggregate.uncommitted_events()));
      }

      AND_WHEN("events are applied to the aggregate") {
        aggregate.apply_and_add_event(test::fake_event<1>{id_value, 1});
        aggregate.apply_and_add_event(test::fake_event<2>{id_value, 2});
        aggregate.apply_and_add_event(test::fake_event<3>{id_value, 3});

        THEN("the aggregate was updated") {
          REQUIRE(3 == skizzay::cddd::version(aggregate));
        }

        THEN("the uncommitted event buffer is not empty") {
          REQUIRE(!std::ranges::empty(aggregate.uncommitted_events()));
        }

        AND_WHEN("the aggregate is put into the repository") {
          skizzay::cddd::put(target, std::move(aggregate));

          THEN("the repository contains the aggregate") {
            REQUIRE(skizzay::cddd::contains(target, id_value));
          }

          AND_WHEN("the aggregate is retrieved") {
            aggregate = skizzay::cddd::get(target, id_value);

            THEN("the aggregate was saved in the repository") {
              REQUIRE(3 == skizzay::cddd::version(aggregate));
            }

            THEN("the uncommitted event buffer is empty") {
              REQUIRE(std::ranges::empty(aggregate.uncommitted_events()));
            }
          }
        }
      }
    }
  }
}