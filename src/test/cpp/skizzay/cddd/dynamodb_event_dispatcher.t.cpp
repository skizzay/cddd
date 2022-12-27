#include <skizzay/cddd/dynamodb/dynamodb_event_dispatcher.h>

#include "skizzay/cddd/aggregate_root.h"
#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/dynamodb/dynamodb_attribute_value.h"
#include "skizzay/cddd/dynamodb/dynamodb_deser.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/timestamp.h"
#include "skizzay/cddd/version.h"
#include <catch.hpp>
#include <chrono>

using namespace skizzay::cddd;

namespace {
struct fake_clock {
  std::chrono::system_clock::time_point now() noexcept {
    return result = skizzay::cddd::now(system_clock);
  }

  [[no_unique_address]] std::chrono::system_clock system_clock;
  std::chrono::system_clock::time_point result =
      skizzay::cddd::now(system_clock);
};

template <std::size_t N>
struct test_event : basic_domain_event<test_event<N>, std::string, std::size_t,
                                       timestamp_t<fake_clock>> {
  static test_event<N> from_item(
      Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> const &item) {
    test_event<N> result;
    set_id(result, dynamodb::get_value_from_item<std::string>(item, "hk"));
    set_timestamp(
        result,
        dynamodb::get_value_from_item<timestamp_t<fake_clock>>(item, "ts"));
    set_version(result, dynamodb::get_value_from_item<std::size_t>(item, "sk"));
    return result;
  }
};

struct fake_aggregate final {
  std::string const &id() const noexcept { return id_; }
  std::size_t version() const noexcept { return version_; }
  timestamp_t<fake_clock> timestamp() const noexcept { return timestamp_; }

  template <std::size_t N> void apply(test_event<N> const &event) {
    this->id_ = skizzay::cddd::id(event);
    this->version_ = skizzay::cddd::version(event);
    this->timestamp_ = skizzay::cddd::timestamp(event);
  }

  std::string id_;
  std::size_t version_ = 0;
  timestamp_t<fake_clock> timestamp_;
};

inline auto random_number_generator =
    Catch::Generators::random(std::size_t{1}, std::size_t{50});

} // namespace

SCENARIO("Events from DynamoDB can be dispatched to handlers",
         "[unit][dynamodb][event_store]") {
  using item_type = Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>;
  std::string const target_id = "target_id_value";
  dynamodb::event_log_config const event_log_config{
      "hk",
      "sk",
      "ts",
      "type",
      "TestEventLog",
      "ttl",
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::years{1})};
  fake_clock clock;

  GIVEN("An event dispatcher") {
    using target_type =
        dynamodb::event_dispatcher<test_event<1>, test_event<2>>;

    target_type target{event_log_config};
    fake_aggregate aggregate;
    aggregate_visitor<fake_aggregate, test_event<1>, test_event<2>> visitor{
        aggregate};

    WHEN("a translator is registered for test event 1") {
      std::string const type = "test event 1";
      target.register_translator(type, test_event<1>::from_item);

      AND_WHEN("a translator is registered for test event 1") {
        bool exception_caught = false;
        try {
          target.register_translator(type, test_event<1>::from_item);
        } catch (std::logic_error const &) {
          exception_caught = true;
        }

        THEN("an invalid argument exception has been thrown") {
          CHECK(exception_caught);
        }
      }

      AND_WHEN("an item has been received for test event 1") {
        item_type item;
        dynamodb::set_item_value(item, event_log_config.key_name(), "abc");
        dynamodb::set_item_value(item, event_log_config.version_name(), 1);
        dynamodb::set_item_value(item, event_log_config.timestamp_name(),
                                 now(clock));
        dynamodb::set_item_value(item, event_log_config.type_name(),
                                 "test event 1");
        target.dispatch(item, visitor);

        THEN("it was dispatched") {
          CHECK(version(aggregate) == 1);
          CHECK(id(aggregate) == "abc");
          CHECK(timestamp(aggregate).time_since_epoch().count() ==
                clock.result.time_since_epoch().count());
        }
      }
    }

    WHEN("a message is received that doesn't have a handler registered") {
      bool exception_caught = false;
      item_type item;
      item.emplace("type", dynamodb::attribute_value("test event 1"));
      try {
        target.dispatch(item, visitor);
      } catch (history_load_failed const &) {
        exception_caught = true;
      }

      THEN("history_load_failed exception was thrown") {
        CHECK(exception_caught);
      }
    }
  }
}