#include <skizzay/cddd/in_memory/in_memory_event_buffer.h>

#include "skizzay/cddd/concurrent_repository.h"
#include "skizzay/cddd/domain_event_wrapper.h"
#include "skizzay/cddd/event_sourced.h"
#include "skizzay/cddd/event_store.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/timestamp.h"

#include "fakes.h"
#include <catch.hpp>

#include <memory>
#include <tuple>
#include <vector>

using namespace skizzay::cddd;

SCENARIO("In-memory event store provides an event source",
         "[unit][in_memory][event_buffer]") {
  test::fake_clock clock;

  GIVEN("an event buffer") {
    auto target = in_memory::buffer<test::fake_event_sequence<3>>{};

    THEN("the version is 0") { REQUIRE(0 == skizzay::cddd::version(target)); }

    WHEN("events are appended to the buffer") {
      std::string const id_value = "some id value";
      auto append_to_buffer = [&](std::size_t expected_version,
                                  std::size_t target_version) {
        auto const ts = skizzay::cddd::now(clock);
        dereference(target).append(
            std::ranges::views::iota(expected_version + 1, target_version + 1) |
                std::ranges::views::transform([id_value, ts](auto const v) {
                  test::fake_event<1> event;
                  event.id = id_value;
                  event.version = v;
                  event.timestamp = ts;
                  return wrap_domain_events<test::fake_event_sequence<3>>(
                      std::move(event));
                }),
            expected_version);
      };
      std::size_t const target_version =
          test::random_number_generator.next() + 10;
      append_to_buffer(0, target_version);

      THEN("the version reflects the number of events appended") {
        REQUIRE(target_version == skizzay::cddd::version(target));
      }

      AND_WHEN("rolled back") {
        std::size_t const rollback_to_version = target_version / 2;
        dereference(target).rollback_to(rollback_to_version);

        THEN("the buffer has been rolled back") {
          REQUIRE(rollback_to_version == skizzay::cddd::version(target));
        }
      }

      AND_WHEN("events are requested") {
        std::size_t const begin_version = target_version / 2;
        std::ranges::sized_range auto const events =
            dereference(target).get_events(
                begin_version, std::numeric_limits<std::size_t>::max());

        THEN("all events in the range have been provided") {
          std::size_t expected_version = begin_version;
          REQUIRE_FALSE(std::ranges::empty(events));
          for (auto const &event : events) {
            REQUIRE(skizzay::cddd::version(event) == expected_version);
            ++expected_version;
          }
        }
      }

      AND_WHEN("events are appended to the buffer") {
        std::size_t const target_version2 =
            target_version + test::random_number_generator.next();
        append_to_buffer(target_version, target_version2);

        THEN("the version reflects the number of events appended") {
          REQUIRE(target_version2 == skizzay::cddd::version(target));
        }
      }
    }
  }
}