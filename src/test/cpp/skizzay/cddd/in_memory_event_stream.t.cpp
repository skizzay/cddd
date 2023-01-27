#include <skizzay/cddd/in_memory_event_store.h>

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/event_store.h"
#include "skizzay/cddd/event_stream.h"
#include "skizzay/cddd/timestamp.h"
#include "skizzay/cddd/version.h"

#include <catch.hpp>

#include <memory>

using namespace skizzay::cddd;

namespace {
struct fake_clock {
  std::chrono::system_clock::time_point now() noexcept {
    return std::exchange(result, skizzay::cddd::now(system_clock));
  }

  [[no_unique_address]] std::chrono::system_clock system_clock;
  std::chrono::system_clock::time_point result =
      skizzay::cddd::now(system_clock);
};

template <std::size_t N>
struct test_event : basic_domain_event<test_event<N>, std::string, std::size_t,
                                       timestamp_t<fake_clock>> {};

struct fake_aggregate {
  using event_type = std::variant<test_event<1>, test_event<2>>;
  using event_storage_type = std::vector<event_type>;

  template <std::size_t N>
  requires(1 == N) || (2 == N) void apply(test_event<N> const &event) {
    version = skizzay::cddd::version(event);
    events.push_back(event);
  }

  std::string id;
  std::size_t version = {};
  event_storage_type events = {};
};

using version_type = version_t<test_event<0>, test_event<1>, test_event<2>>;
} // namespace

SCENARIO("In-memory event store provides an event stream",
         "[unit][in_memory][event_store][event_stream]") {
  GIVEN("An in-memory event store") {
    using events_list = domain_event_sequence<test_event<1>, test_event<2>>;
    in_memory_event_store<fake_clock, events_list> target;

    using event_ptr = std::unique_ptr<event_wrapper<events_list>>;
    using event_interface = std::iter_value_t<event_ptr>;
    REQUIRE(std::is_class_v<event_ptr>);
    REQUIRE(std::indirectly_readable<event_ptr>);
    REQUIRE(
        std::invocable<decltype(skizzay::cddd::id), event_interface const &>);
    REQUIRE(std::invocable<decltype(skizzay::cddd::id), event_ptr const &>);
    // REQUIRE(concepts::identifiable<event_ptr>);
    // REQUIRE(concepts::versioned<event_ptr>);
    // REQUIRE(concepts::domain_event<event_ptr>);

    THEN("it is an event store") {
      // REQUIRE(concepts::event_store<decltype(target)>);
    }

    THEN("the event store has no events for any id") {
      std::string id = "abc";
      REQUIRE_FALSE(target.has_events_for(id));
    }

    WHEN("an event stream is requested from the store using an unused id") {
      std::string id = "abc";
      auto event_stream = get_event_stream(target, std::as_const(id));

      THEN("the event stream is empty") { REQUIRE(0 == version(event_stream)); }

      AND_WHEN("the event stream is committed") {
        commit_events(event_stream, version_type{0});

        THEN("the event store has no events for the id") {
          REQUIRE_FALSE(target.has_events_for(id));
        }
      }

      AND_WHEN("events are added to the stream") {
        add_event(event_stream, test_event<1>{});
        add_event(event_stream, test_event<2>{});

        AND_WHEN("the event stream is committed") {
          commit_events(event_stream, 0);

          THEN("the stream has events") { REQUIRE(0 < version(event_stream)); }

          AND_THEN("the event store has events for the committed id") {
            REQUIRE(target.has_events_for(id));
          }

          AND_WHEN("an event stream is requested from the store using the id") {
            auto other_event_stream =
                get_event_stream(target, std::as_const(id));

            THEN("the event stream is not empty") {
              REQUIRE(0 < version(other_event_stream));
            }

            AND_WHEN("events are added to the other stream") {
              add_event(other_event_stream, test_event<1>{});

              AND_WHEN("the other stream is committed") {
                commit_events(other_event_stream, 2);

                AND_WHEN("events are added to the stream") {
                  add_event(event_stream, test_event<1>{});
                  add_event(event_stream, test_event<2>{});

                  AND_WHEN("the event stream is committed") {
                    bool found_exception = false;
                    try {
                      commit_events(event_stream, 5);
                    } catch (optimistic_concurrency_collision const &e) {
                      found_exception = true;
                      REQUIRE(e.version_expected() == 5);
                    }

                    THEN(
                        "An optimistic concurrency collision was encountered") {
                      REQUIRE(found_exception);
                    }

                    AND_WHEN("the event stream is rolled back") {
                      rollback(event_stream);

                      THEN("Both event streams are synced to same version") {
                        REQUIRE(version(event_stream) ==
                                version(other_event_stream));
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

SCENARIO("In-memory event store provides an event source",
         "[unit][in_memory][event_store][event_source]") {
  GIVEN("An in-memory event store") {
    using events_list = domain_event_sequence<test_event<1>, test_event<2>>;
    in_memory_event_store<fake_clock, events_list> target;

    AND_GIVEN("an event source for id=\"abc\"") {
      std::string id = "abc";
      auto event_source = get_event_source(target, id);

      AND_GIVEN("an aggregate that can consume events from the event source") {
        fake_aggregate aggregate{id};

        WHEN("replaying the aggregate from source") {
          REQUIRE(std::invocable<decltype(skizzay::cddd::apply),
                                 fake_aggregate &, test_event<1> const &>);
          REQUIRE(std::invocable<decltype(skizzay::cddd::apply),
                                 fake_aggregate &, test_event<2> const &>);
          load_from_history(event_source, aggregate);

          THEN("no events were loaded") {
            REQUIRE(std::empty(aggregate.events));
            REQUIRE(0 == version(aggregate));
          }
        }
      }
    }

    AND_GIVEN("events have been streamed for id=\"abc\"") {
      std::string id = "abc";
      {
        auto event_stream = get_event_stream(target, std::as_const(id));
        add_event(event_stream, test_event<1>{id});
        add_event(event_stream, test_event<2>{id});
        commit_events(event_stream, 0);
      }
      AND_GIVEN("an event source for id=\"abc\"") {
        auto event_source = get_event_source(target, id);

        AND_GIVEN(
            "an aggregate that can consume events from the event source") {
          fake_aggregate aggregate{id};

          WHEN("replaying the aggregate from source") {
            load_from_history(event_source, aggregate);

            THEN("the aggregate loaded the events") {
              REQUIRE_FALSE(std::empty(aggregate.events));
              AND_THEN(
                  "the aggregate's version reflects last event's version") {
                REQUIRE(version(aggregate.events.back()) == version(aggregate));
              }
            }
          }
        }
      }
    }
  }
}