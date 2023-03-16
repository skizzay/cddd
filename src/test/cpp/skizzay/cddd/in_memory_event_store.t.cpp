#include <skizzay/cddd/in_memory/in_memory_event_store.h>

#include "fakes.h"
#include "skizzay/cddd/event_sourced.h"
#include "skizzay/cddd/event_store.h"
#include <catch.hpp>

using namespace skizzay::cddd;

TEST_CASE("in-memory event store") {
  using domain_event_sequence_t = test::fake_event_sequence<2>;
  using clock_t = test::fake_clock;

  auto target = in_memory::event_store{domain_event_sequence_t{}, clock_t{}};
  REQUIRE(skizzay::cddd::concepts::event_store<decltype(target)>);

  SECTION("event stream buffer") {
    auto event_stream_buffer = get_event_stream_buffer(target);
    REQUIRE(skizzay::cddd::concepts::event_stream_buffer<
            decltype(event_stream_buffer)>);
    REQUIRE(std::empty(event_stream_buffer));

    SECTION("add events to the buffer") {
      add_event(event_stream_buffer, test::fake_event<2>{});
      add_event(event_stream_buffer, test::fake_event<1>{});

      REQUIRE(2 == std::size(event_stream_buffer));

      SECTION("clearing the buffer") {
        event_stream_buffer.clear();
        REQUIRE(std::empty(event_stream_buffer));
      }
    }
  }

  SECTION("event source") {
    auto event_source = get_event_source(target);
    REQUIRE(skizzay::cddd::concepts::event_source<decltype(event_source)>);

    SECTION("doesn't modify an aggregate if there aren't any events for it") {
      test::fake_aggregate aggregate{"test_id",
                                     get_event_stream_buffer(target)};
      REQUIRE(skizzay::cddd::concepts::aggregate_root<decltype(aggregate)>);
      load_from_history(event_source, aggregate);
      REQUIRE(0 == version(aggregate));
    }
  }

  SECTION("event stream") {
    auto event_stream = get_event_stream(target);
    REQUIRE(skizzay::cddd::concepts::event_stream<decltype(event_stream)>);

    SECTION("committing event buffers") {
      auto event_stream_buffer = get_event_stream_buffer(target);
      std::string id_value = "test_id";
      add_event(event_stream_buffer, test::fake_event<1>{});
      commit_events(event_stream, std::move(id_value), 0,
                    std::move(event_stream_buffer));
    }
  }
}