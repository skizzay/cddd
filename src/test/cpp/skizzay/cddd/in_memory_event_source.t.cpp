#include <skizzay/cddd/in_memory/in_memory_event_source.h>

#include "skizzay/cddd/concurrent_repository.h"
#include "skizzay/cddd/domain_event_wrapper.h"
#include "skizzay/cddd/event_sourced.h"
#include "skizzay/cddd/event_store.h"
#include "skizzay/cddd/identifier.h"

#include "fakes.h"
#include <catch.hpp>

#include <memory>
#include <tuple>
#include <vector>

using namespace skizzay::cddd;

namespace {
using buffer_type =
    std::vector<std::unique_ptr<event_wrapper<test::fake_event_sequence<2>>>>;
using domain_event_sequence_t = test::fake_event_sequence<2>;

struct fake_buffer {
  std::ranges::sized_range auto get_events(std::size_t begin_version,
                                           std::size_t target_version) {
    get_events_requests.emplace_back(begin_version, target_version);
    return std::ranges::views::iota(begin_version, target_version + 1) |
           std::ranges::views::transform([this](
                                             std::size_t const event_version) {
             test::fake_event<1> event;
             event.id = id;
             event.version = event_version;
             event.timestamp = timestamp;
             return skizzay::cddd::wrap_domain_events<domain_event_sequence_t>(
                 std::move(event));
           });
  }

  std::string id{};
  std::chrono::system_clock::time_point timestamp{};
  mutable std::vector<std::pair<std::size_t, std::size_t>>
      get_events_requests{};
};

struct fake_store {
  using domain_event_sequence = domain_event_sequence_t;

  test::fake_clock &clock() noexcept { return clock_; }

  in_memory::event_source<fake_store> get_event_source() noexcept {
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

SCENARIO("In-memory event store provides an event source",
         "[unit][in_memory][event_store][event_stream]") {
  fake_store store;
  test::fake_aggregate<buffer_type> aggregate_root;
  skizzay::cddd::now(store.clock());

  GIVEN("an event source provided by its store") {
    auto target = skizzay::cddd::get_event_source(store);

    REQUIRE(skizzay::cddd::concepts::event_source<decltype(target)>);

    WHEN("loading an unknown aggregate from history") {
      skizzay::cddd::load_from_history(target, aggregate_root);

      THEN("nothing was loaded into the aggregate") {
        REQUIRE(0 == skizzay::cddd::version(aggregate_root));
      }
    }

    WHEN("loading a known aggregate from history") {
      std::string const id_value = "some id";
      std::size_t const starting_version = test::random_number_generator.next();
      std::size_t const num_events_to_load =
          test::random_number_generator.next();
      std::size_t const target_version = starting_version + num_events_to_load;
      aggregate_root.id = id_value;
      aggregate_root.version = starting_version;
      store.event_buffers().put(
          id_value, std::make_shared<fake_buffer>(
                        id_value, skizzay::cddd::now(store.clock())));

      skizzay::cddd::load_from_history(target, aggregate_root, target_version);
      THEN("events were loaded starting where the aggregate root left off") {
        REQUIRE(skizzay::cddd::version(aggregate_root) == target_version);
      }
    }
  }
}