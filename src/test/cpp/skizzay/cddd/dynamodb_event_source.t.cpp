#include <skizzay/cddd/dynamodb/dynamodb_event_source.h>

#include "skizzay/cddd/dynamodb/aws_sdk_raii.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_log_table.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_stream.h"

#include <catch.hpp>

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
  explicit fake_aggregate(std::string id) : id_{std::move(id)} {}

  std::string const &id() const noexcept { return id_; }
  std::size_t version() const noexcept { return version_; }
  timestamp_t<fake_clock> timestamp() const noexcept { return timestamp_; }

  template <std::size_t N> void apply(test_event<N> const &event) {
    CHECK(skizzay::cddd::id(event) == skizzay::cddd::id(*this));
    this->version_ = skizzay::cddd::version(event);
    this->timestamp_ = skizzay::cddd::timestamp(event);
    ++number_of_events_seen;
  }

  std::string id_;
  std::size_t version_ = 0;
  timestamp_t<fake_clock> timestamp_;
  std::size_t number_of_events_seen = 0;
};

struct fake_serializer : dynamodb::serializer<test_event<1>, test_event<2>> {
  Aws::DynamoDB::Model::Put serialize(test_event<1> &&) const override {
    return {};
  }
  std::string_view
  message_type(event_type<test_event<1>> const) const noexcept override {
    return "test event 1";
  }

  Aws::DynamoDB::Model::Put serialize(test_event<2> &&) const override {
    return {};
  }
  std::string_view
  message_type(event_type<test_event<2>> const) const noexcept override {
    return "test event 2";
  }
};

inline auto random_number_generator =
    Catch::Generators::random(std::size_t{1}, std::size_t{50});

} // namespace

SCENARIO("Aggregates can be loaded from a DynamoDB event source",
         "[unit][dynamodb][event_store]") {
  dynamodb::event_log_config const event_log_config{
      "hk",
      "sk",
      "ts",
      "type",
      "TestEventLog",
      "ttl",
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::years{1})};
  Aws::SDKOptions options;
  dynamodb::aws_sdk_raii aws_sdk{options};
  Aws::Client::ClientConfiguration client_configuration("default");
  client_configuration.endpointOverride = "http://localhost:4566";
  Aws::DynamoDB::DynamoDBClient client{client_configuration};
  dynamodb::event_log_table event_log_table{client, event_log_config};
  dynamodb::event_dispatcher<test_event<1>, test_event<2>> event_dispatcher{
      event_log_config};
  fake_clock clock;
  std::string aggregate_id = "abcd";
  fake_aggregate aggregate{aggregate_id};

  event_dispatcher.register_translator("test event 1",
                                       test_event<1>::from_item);
  event_dispatcher.register_translator("test event 2",
                                       test_event<2>::from_item);
  random_number_generator.next();

  GIVEN("a DynamoDB event source") {
    dynamodb::event_source target{event_dispatcher, event_log_config, client};

    WHEN("an aggregate is loaded from history") {
      skizzay::cddd::load_from_history(target, aggregate);

      THEN("no events have been applied to the aggregate") {
        CHECK(0 == aggregate.number_of_events_seen);
      }
    }

    AND_GIVEN("there are events on the stream") {
      fake_serializer serializer;
      dynamodb::event_stream<fake_clock, test_event<1>, test_event<2>>
          event_stream{aggregate_id, serializer, event_log_config, client,
                       clock};
      std::size_t const num_events_to_add = random_number_generator.get();
      int one_or_two = 1;
      for (std::size_t i = 0; i != num_events_to_add; ++i) {
        if (1 == one_or_two) {
          skizzay::cddd::add_event(event_stream, test_event<1>{});
          one_or_two = 2;
        } else {
          skizzay::cddd::add_event(event_stream, test_event<2>{});
          one_or_two = 1;
        }
      }
      skizzay::cddd::commit_events(event_stream, std::size_t{0});

      WHEN("an aggregate is loaded from history") {
        skizzay::cddd::load_from_history(target, aggregate);

        THEN("the events have been applied to the aggregate") {
          CHECK(num_events_to_add == aggregate.number_of_events_seen);
          CHECK(num_events_to_add == skizzay::cddd::version(aggregate));
        }
      }
    }
  }
}