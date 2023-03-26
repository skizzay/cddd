#include <skizzay/cddd/in_memory/in_memory_event_stream.h>

#include "skizzay/cddd/concurrent_repository.h"
#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/domain_event_wrapper.h"
#include "skizzay/cddd/event_store.h"
#include "skizzay/cddd/event_stream.h"
#include "skizzay/cddd/timestamp.h"
#include "skizzay/cddd/version.h"

#include "fakes.h"
#include <catch.hpp>

#include <memory>
#include <tuple>
#include <vector>

using namespace skizzay::cddd;

namespace {
using buffer_type =
    std::vector<std::unique_ptr<event_wrapper<test::fake_event_sequence<2>>>>;

struct fake_buffer {
  void append(buffer_type &&events, std::size_t const expected_version) {
    appended.emplace_back(std::move(events), expected_version);
  }

  void rollback_to(std::size_t const expected_version) {
    rollbacks.emplace_back(expected_version);
  }

  std::vector<std::tuple<buffer_type, std::size_t>> appended;
  std::vector<std::size_t> rollbacks;
};

struct fake_store {
  using buffer_type = ::buffer_type;

  test::fake_clock &clock() noexcept { return clock_; }

  in_memory::event_stream<fake_store> get_event_stream() noexcept {
    return {*this};
  }

  concurrent_table<std::shared_ptr<fake_buffer>, std::string> &
  event_buffers() noexcept {
    return event_buffers_;
  }

  concurrent_table<std::shared_ptr<fake_buffer>, std::string> event_buffers_;
  test::fake_clock clock_;
};
} // namespace

SCENARIO("In-memory event store provides an event stream",
         "[unit][in_memory][event_store][event_stream]") {
  fake_store store;

  GIVEN("an event stream provided by its store") {
    [[maybe_unused]] skizzay::cddd::concepts::event_stream auto target =
        get_event_stream(store);

    AND_GIVEN("an empty buffer") {
      buffer_type empty_buffer;
      WHEN("the buffer is committed to the stream") {
        std::string const id_value = "some_id";
        std::size_t const expected_version = 0;

        commit_events(target, id_value, expected_version,
                      std::move(empty_buffer));

        THEN("the buffer was not committed") {
          REQUIRE_FALSE(store.event_buffers().contains(id_value));
        }
      }
    }

    AND_GIVEN("a populated buffer") {
      buffer_type full_buffer;
      add_event(full_buffer, wrap_domain_events<test::fake_event_sequence<2>>(
                                 test::fake_event<1>{}));
      add_event(full_buffer, wrap_domain_events<test::fake_event_sequence<2>>(
                                 test::fake_event<2>{}));

      WHEN("the buffer is committed to the stream") {
        std::string const id_value = "some_id";
        std::size_t const expected_version = 0;

        commit_events(target, id_value, expected_version,
                      std::move(full_buffer));

        THEN("the buffer was committed") {
          REQUIRE(store.event_buffers().contains(id_value));

          AND_THEN("each event in the buffer is versioned") {
            auto const &[appended_buffer, appended_expected_version] =
                store.event_buffers().get(id_value)->appended.front();
            REQUIRE(expected_version == appended_expected_version);
            std::size_t expected_event_version = 1;
            for (auto const &buffered_event : appended_buffer) {
              REQUIRE(version(buffered_event) ==
                      (expected_event_version + expected_version));
              ++expected_event_version;
            }
          }

          AND_THEN("each event in the buffer is timestamped") {
            auto const &[appended_buffer, _] =
                store.event_buffers().get(id_value)->appended.front();
            for (auto const &buffered_event : appended_buffer) {
              REQUIRE(timestamp(buffered_event) == store.clock().result);
            }
          }

          AND_THEN("each event in the buffer has the same id") {
            auto const &[appended_buffer, _] =
                store.event_buffers().get(id_value)->appended.front();
            for (auto const &buffered_event : appended_buffer) {
              REQUIRE(id(buffered_event) == id_value);
            }
          }
        }

        AND_WHEN("the buffer is rolled back to 0") {
          rollback_to(target, id_value, expected_version);

          THEN("the stream is not found") {
            REQUIRE_FALSE(contains(store.event_buffers(), id_value));
          }
        }
      }
    }
  }
}