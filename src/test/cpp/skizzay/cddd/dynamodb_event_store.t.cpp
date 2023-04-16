#include <skizzay/cddd/dynamodb/dynamodb_event_store.h>

#include "skizzay/cddd/dynamodb/dynamodb_event_stream_buffer.h"
#include "skizzay/cddd/event_store.h"
#include "skizzay/cddd/event_stream.h"
#include "skizzay/cddd/event_stream_buffer.h"

#include "fakes.h"

using namespace skizzay::cddd;

namespace {
using A = test::fake_event<1>;
using B = test::fake_event<2>;
using C = test::fake_event<3>;

struct transformer {
  template <std::size_t N>
  Aws::DynamoDB::Model::Put operator()(test::fake_event<N> const &) const {
    return {};
  }
};

inline constexpr auto create_event_stream_buffer = []() {
  return dynamodb::event_stream_buffer<transformer>{};
};
} // namespace

TEST_CASE("DynamoDB event store") {
  dynamodb::event_store target{create_event_stream_buffer};

  SECTION("event stream buffer") {
    auto event_stream_buffer = skizzay::cddd::get_event_stream_buffer(target);
    REQUIRE(skizzay::cddd::concepts::event_stream_buffer<
            decltype(event_stream_buffer)>);
    REQUIRE(std::empty(event_stream_buffer));

    SECTION("add events to the buffer") {
      skizzay::cddd::add_event(event_stream_buffer, test::fake_event<2>{});
      skizzay::cddd::add_event(event_stream_buffer, test::fake_event<1>{});

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
      test::fake_aggregate aggregate{
          "test_id", skizzay::cddd::get_event_stream_buffer(target)};
      REQUIRE(skizzay::cddd::concepts::aggregate_root<decltype(aggregate)>);
      skizzay::cddd::load_from_history(event_source, aggregate);
      REQUIRE(0 == version(aggregate));
    }
  }

  SECTION("event stream") {
    auto event_stream = get_event_stream(target);
    REQUIRE(skizzay::cddd::concepts::event_stream<decltype(event_stream)>);

    SECTION("committing event buffers") {
      auto event_stream_buffer = get_event_stream_buffer(target);
      std::string id_value = "test_id";
      skizzay::cddd::add_event(event_stream_buffer, test::fake_event<1>{});
      skizzay::cddd::commit_events(event_stream, std::move(id_value), 0,
                                   std::move(event_stream_buffer));
    }
  }
}