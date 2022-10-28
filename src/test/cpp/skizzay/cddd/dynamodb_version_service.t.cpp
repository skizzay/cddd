#include <skizzay/cddd/dynamodb/dynamodb_version_service.h>

#include "skizzay/cddd//dynamodb/aws_sdk_raii.h"
#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_log_table.h"
#include "skizzay/cddd/timestamp.h"
#include "skizzay/cddd/version.h"
#include <aws/dynamodb/model/TransactWriteItemsRequest.h>
#include <catch.hpp>
#include <chrono>
#include <cstddef>
#include <optional>
#include <random>
#include <string>
#include <utility>

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

} // namespace

SCENARIO("Versions can be synced with DynamoDB",
         "[unit][dynamodb][event_store]") {
  using target_type = dynamodb::version_service<test_event<1>, test_event<2>>;
  std::string const target_id = "target_id_value";
  dynamodb::event_log_config const event_log_config{
      "hash_key",
      "sort_key",
      "timestamp",
      "type",
      "event_log",
      "ttl",
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::years{1})};

  GIVEN("A version service") {
    target_type target{target_id, event_log_config};

    WHEN("queried for version") {
      std::size_t const actual = version(target);

      THEN("the version is 0") { REQUIRE(0 == actual); }
    }

    WHEN("queried for the key") {
      auto const key_item = target.key();

      THEN("the hash key is the id") {
        auto hash_key_iterator = key_item.find(event_log_config.key_name());
        REQUIRE_FALSE(std::end(key_item) == hash_key_iterator);
        REQUIRE(Aws::DynamoDB::Model::ValueType::STRING ==
                hash_key_iterator->second.GetType());
        REQUIRE(target_id == hash_key_iterator->second.GetS());
      }
    }
  }
}

SCENARIO("Reading and writing versions",
         "[integration][dynamodb][event_store]") {
  using target_type = dynamodb::version_service<test_event<1>, test_event<2>>;
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

  dynamodb::event_log_table event_log_table{client, event_log_config};

  GIVEN("A version service") {
    target_type target{target_id, event_log_config};

    WHEN("the version is cached from source") {
      target.update_version(client);
      THEN("version is 0") { REQUIRE(0 == skizzay::cddd::version(target)); }

      AND_WHEN("a version record is committed") {
        std::size_t const num_items_in_commit =
            Catch::Generators::random(1ull, 50ull).get();
        auto const commit_outcome = client.TransactWriteItems(
            Aws::DynamoDB::Model::TransactWriteItemsRequest{}.AddTransactItems(
                target.version_record(num_items_in_commit)));
        if (commit_outcome.IsSuccess()) {
          AND_WHEN("the version is cached from source") {
            target.update_version(client);
            THEN("the version is the number of items committed") {
              REQUIRE(num_items_in_commit == skizzay::cddd::version(target));
            }
          }
          AND_WHEN("the service has success called back") {
            target.on_commit_success(num_items_in_commit);
            THEN("the version is the number of items committed") {
              REQUIRE(num_items_in_commit == skizzay::cddd::version(target));
            }
          }
        } else {
          FAIL("Commit did not succeed: " + commit_outcome.GetError().GetMessage());
        }
      }
    }
  }
}