#include <skizzay/cddd/dynamodb/dynamodb_event_stream.h>

#include "skizzay/cddd/dynamodb/aws_sdk_raii.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_log_table.h"
#include "skizzay/cddd/event_stream.h"
#include "skizzay/cddd/timestamp.h"
#include "skizzay/cddd/version.h"
#include <aws/dynamodb/model/TransactWriteItemsRequest.h>
#include <catch.hpp>

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

SCENARIO("Events can be streamed to DynamoDB",
         "[unit][dynamodb][event_store]") {
  Aws::SDKOptions options;
  dynamodb::aws_sdk_raii aws_sdk{options};
  Aws::Client::ClientConfiguration client_configuration("default");
  client_configuration.endpointOverride = "http://localhost:4566";
  Aws::DynamoDB::DynamoDBClient client{client_configuration};
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

  dynamodb::event_log_table event_log_table{client, event_log_config};
  fake_serializer serializer;

  random_number_generator.next();
  GIVEN("a DynamoDB event stream") {
    using target_type =
        dynamodb::event_stream<fake_clock, test_event<1>, test_event<2>>;
    target_type target{target_id, serializer, event_log_config, client, clock};

    AND_GIVEN("events have been added") {
      int one_or_two = 1;
      for (std::size_t i = 0, num_events_to_add = random_number_generator.get();
           i != num_events_to_add; ++i) {
        if (1 == one_or_two) {
          skizzay::cddd::add_event(target, test_event<1>{});
          one_or_two = 2;
        } else {
          skizzay::cddd::add_event(target, test_event<2>{});
          one_or_two = 1;
        }
      }

      WHEN("events are committed") {
        skizzay::cddd::commit_events(target, std::size_t{0});
      }
    }
  }
}