#include <skizzay/cddd/dynamodb/dynamodb_event_stream.h>

#include "skizzay/cddd/dynamodb/dynamodb_event_log_config.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_stream_buffer.h"
#include "skizzay/cddd/event_stream.h"

#include "fakes.h"
#include <aws/core/utils/crypto/Factories.h>
#include <aws/dynamodb/DynamoDBClient.h>

using namespace skizzay::cddd;

namespace {
using A = test::fake_event<1>;
using B = test::fake_event<2>;
using C = test::fake_event<3>;

inline constexpr std::chrono::days week{7};

struct transformer {
  template <std::size_t N>
  Aws::DynamoDB::Model::Put operator()(test::fake_event<N> const &) const {
    return {};
  }
};
using buffer_type = dynamodb::event_stream_buffer<transformer>;

inline constexpr auto create_event_stream_buffer = []() -> buffer_type {
  return buffer_type{};
};

struct fake_client {
  Aws::DynamoDB::Model::TransactWriteItemsOutcome
  commit(Aws::DynamoDB::Model::TransactWriteItemsRequest &&request) {
    requests.emplace_back(std::move(request));
    if (should_fail) {
      return Aws::DynamoDB::Model::TransactWriteItemsOutcome{};
    } else {
      return Aws::DynamoDB::Model::TransactWriteItemsResult{};
    }
  }

  bool should_fail = false;
  std::vector<Aws::DynamoDB::Model::TransactWriteItemsRequest> requests;
};

struct fake_store {
  dynamodb::event_stream<fake_store> get_event_stream() const noexcept {
    return dynamodb::event_stream<fake_store>{*const_cast<fake_store *>(this)};
  }
  dynamodb::event_log_config const &config() const noexcept { return config_; }
  fake_client &client() noexcept { return client_; }
  test::fake_clock &clock() noexcept { return clock_; }

  std::string const &new_condition_expression() const noexcept {
    return new_condition_expression_;
  }
  std::string const &update_condition_expression() const noexcept {
    return update_condition_expression_;
  }

  dynamodb::event_log_config config_;
  fake_client client_;
  test::fake_clock clock_;
  std::string new_condition_expression_{"new_condition_expression"};
  std::string update_condition_expression_{"update_condition_expression"};
};
} // namespace

TEST_CASE("DynamoDB Event Stream") {
  Aws::Utils::Crypto::InitCrypto();
  fake_store store;
  store.config_.with_ttl(week);

  GIVEN("an event stream provided by its store") {
    skizzay::cddd::concepts::event_stream auto target =
        store.get_event_stream();

    AND_GIVEN("an empty buffer") {
      buffer_type empty_buffer;
      WHEN("the buffer is committed to the stream") {
        std::string const id_value = "some_id";
        std::size_t const expected_version = 0;

        commit_events(target, id_value, expected_version,
                      std::move(empty_buffer));

        THEN("the buffer was not committed") {
          REQUIRE(store.client().requests.empty());
        }
      }
    }

    AND_GIVEN("a populated buffer") {
      buffer_type full_buffer;
      add_event(full_buffer, test::fake_event<1>{});
      add_event(full_buffer, test::fake_event<2>{});

      WHEN("the buffer is committed to the stream") {
        std::string const id_value = "some_id";
        std::size_t const expected_version = 0;

        commit_events(target, id_value, expected_version,
                      std::move(full_buffer));

        THEN("the buffer was committed") {
          REQUIRE(1 == std::size(store.client().requests));
          // 1 ConditionCheck, 2 Put
          REQUIRE(3 == std::size(
                           store.client().requests.front().GetTransactItems()));

          AND_THEN("each event in the buffer is versioned") {
            auto const &version_key = store.config().sort_key();
            for (auto const &[index, items] : views::enumerate(
                     store.client().requests.front().GetTransactItems())) {
              REQUIRE(items.GetPut().GetItem().at(version_key.name).GetN() ==
                      std::to_string(index));
            }
          }

          AND_THEN("each event in the buffer is timestamped") {
            auto const &timestamp_field = store.config().timestamp_field();
            for (auto const &items :
                 store.client().requests.front().GetTransactItems()) {
              REQUIRE(items.GetPut().GetItem().at(timestamp_field.name) ==
                      dynamodb::attribute_value(store.clock().result));
            }
          }

          AND_THEN("each event in the buffer has the same id") {
            auto const &id_key = store.config().hash_key();
            for (auto const &items :
                 store.client().requests.front().GetTransactItems()) {
              REQUIRE(items.GetPut().GetItem().at(id_key.name) ==
                      dynamodb::attribute_value(id_value));
            }
          }

          // AND_WHEN("the buffer is rolled back to 0") {
          //   rollback_to(target, id_value, expected_version);

          //   THEN("the stream is not found") {
          //     REQUIRE_FALSE(contains(store.event_buffers(), id_value));
          //   }
          // }
        }
      }

      AND_GIVEN("DynamoDB is in a bad state") {
        store.client().should_fail = true;

        WHEN("the buffer is committed to the stream") {
          std::string const id_value = "some_id";
          std::size_t const expected_version = 0;

          REQUIRE_THROWS_AS(commit_events(target, id_value, expected_version,
                                          std::move(full_buffer)),
                            commit_failed);
        }
      }
    }
  }

  Aws::Utils::Crypto::CleanupCrypto();
}

// SCENARIO("Events can be streamed to DynamoDB",
//          "[unit][dynamodb][event_store]") {
//   Aws::SDKOptions options;
//   dynamodb::aws_sdk_raii aws_sdk{options};
//   Aws::Client::ClientConfiguration client_configuration("default");
//   client_configuration.endpointOverride = "http://localhost:4566";
//   Aws::DynamoDB::DynamoDBClient client{client_configuration};
//   std::string const target_id = "target_id_value";
//   dynamodb::event_log_config const event_log_config{
//       "hk",
//       "sk",
//       "ts",
//       "type",
//       "TestEventLog",
//       "ttl",
//       std::chrono::duration_cast<std::chrono::seconds>(std::chrono::years{1})};
//   fake_clock clock;

//   dynamodb::event_log_table event_log_table{client, event_log_config};
//   fake_serializer serializer;

//   random_number_generator.next();
//   GIVEN("a DynamoDB event stream") {
//     using target_type =
//         dynamodb::event_stream<fake_clock, test_event<1>, test_event<2>>;
//     target_type target{target_id, serializer, event_log_config, client,
//     clock};

//     AND_GIVEN("events have been added") {
//       int one_or_two = 1;
//       for (std::size_t i = 0, num_events_to_add =
//       random_number_generator.get();
//            i != num_events_to_add; ++i) {
//         if (1 == one_or_two) {
//           skizzay::cddd::add_event(target, test_event<1>{});
//           one_or_two = 2;
//         } else {
//           skizzay::cddd::add_event(target, test_event<2>{});
//           one_or_two = 1;
//         }
//       }

//       WHEN("events are committed") {
//         skizzay::cddd::commit_events(target, std::size_t{0});
//       }
//     }
//   }
// }