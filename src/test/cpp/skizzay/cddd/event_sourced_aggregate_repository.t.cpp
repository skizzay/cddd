#include <skizzay/cddd/event_sourced_aggregate_repository.h>

#include "fakes.h"
#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/event_store.h"
#include "skizzay/cddd/in_memory/in_memory_event_store.h"
#include "skizzay/cddd/repository.h"

#include <catch.hpp>
#include <iostream>

using namespace skizzay::cddd;

namespace {
inline constexpr auto create_fake_aggregate = [](auto id_value, auto buffer) {
  return test::fake_aggregate{std::move(id_value), std::move(buffer)};
};
} // namespace

SCENARIO("Event-sourced aggregate repository", "[unit][repository]") {
  GIVEN("An event-sourced aggregate repository") {
    auto target = event_sourced_aggregate_repository{
        in_memory::event_store{test::fake_event_sequence<4>{},
                               test::fake_clock{}},
        create_fake_aggregate};

    WHEN("an aggregate is retrieved") {
      std::string id_value = "some_id";
      auto aggregate = skizzay::cddd::get(target, id_value);

      THEN("the aggregate didn't exist previously") {
        REQUIRE(0 == skizzay::cddd::version(aggregate));
      }

      THEN("the uncommitted event buffer is empty") {
        REQUIRE(std::ranges::empty(aggregate.uncommitted_events));
      }

      AND_WHEN("events are applied to the aggregate") {
        auto const apply_and_buffer = [&](auto &&event) {
          skizzay::cddd::apply_event(
              aggregate,
              static_cast<std::remove_reference_t<decltype(event)> const &>(
                  event));
          skizzay::cddd::add_event(aggregate.uncommitted_events,
                                   std::move(event));
        };
        apply_and_buffer(test::fake_event<1>{id_value, 1});
        apply_and_buffer(test::fake_event<2>{id_value, 2});
        apply_and_buffer(test::fake_event<3>{id_value, 3});

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
              REQUIRE(std::ranges::empty(aggregate.uncommitted_events));
            }
          }
        }
      }
    }
  }
}