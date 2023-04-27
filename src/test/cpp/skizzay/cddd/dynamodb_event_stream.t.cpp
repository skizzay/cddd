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

template <std::size_t N>
dynamodb::record make_record(test::fake_event<N> const &event) {
  dynamodb::record record{};
  record["hk"] = dynamodb::attribute_value(event.id);
  record["sk"] = dynamodb::attribute_value(event.version);
  record["type"] = dynamodb::attribute_value(N);
  return record;
}

template <std::size_t N>
Aws::DynamoDB::Model::Put make_put(test::fake_event<N> const &event) {
  return Aws::DynamoDB::Model::Put{}.WithItem(make_record(event));
}

struct transformer {
  template <std::size_t N>
  Aws::DynamoDB::Model::Put operator()(test::fake_event<N> const &event) const {
    return make_put(event);
  }
};
using buffer_type = dynamodb::event_stream_buffer<transformer>;

inline constexpr auto create_event_stream_buffer = []() -> buffer_type {
  return buffer_type{};
};

struct fake_client {
  Aws::DynamoDB::Model::TransactWriteItemsOutcome
  commit(Aws::DynamoDB::Model::TransactWriteItemsRequest &&request) {
    event_requests.emplace_back(std::move(request));
    if (should_fail) {
      return Aws::DynamoDB::Model::TransactWriteItemsOutcome{};
    } else {
      return Aws::DynamoDB::Model::TransactWriteItemsResult{};
    }
  }

  bool should_fail = false;
  std::vector<Aws::DynamoDB::Model::TransactWriteItemsRequest> event_requests;
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

  dynamodb::event_log_config config_ =
      dynamodb::event_log_config{}
          .with_table(dynamodb::table_name{"some_table"})
          .with_ttl(week);
  fake_client client_;
  test::fake_clock clock_;
  std::string new_condition_expression_{"new_condition_expression"};
  std::string update_condition_expression_{"update_condition_expression"};
};
} // namespace

TEST_CASE("DynamoDB Event Stream") {
  Aws::Utils::Crypto::InitCrypto();
  std::string const id_value = "some_id";
  fake_store store;

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
          REQUIRE(store.client().event_requests.empty());
        }
      }
    }

    AND_GIVEN("a populated buffer") {
      buffer_type full_buffer;
      full_buffer.emplace_back(make_put(A{id_value, 1}));
      full_buffer.emplace_back(make_put(B{id_value, 2}));

      WHEN("the buffer is committed to the stream") {
        std::size_t const expected_version = 0;

        commit_events(target, id_value, expected_version,
                      std::move(full_buffer));

        THEN("the buffer was committed") {
          REQUIRE(1 == std::size(store.client().event_requests));
          // 1 Max Version Record, 2 Event Records
          REQUIRE(
              3 ==
              std::size(
                  store.client().event_requests.front().GetTransactItems()));

          AND_THEN("each put in the buffer is versioned") {
            auto const &version_key = store.config().sort_key();
            for (auto const &[index, items] :
                 views::enumerate(store.client()
                                      .event_requests.front()
                                      .GetTransactItems())) {
              REQUIRE(items.GetPut().GetItem().at(version_key.name).GetN() ==
                      std::to_string(index));
            }
          }

          AND_THEN("each put in the buffer is timestamped") {
            auto const &timestamp_field = store.config().timestamp_field();
            for (auto const &items :
                 store.client().event_requests.front().GetTransactItems()) {
              REQUIRE(
                  items.GetPut().GetItem().at(timestamp_field.name).GetN() ==
                  dynamodb::attribute_value(store.clock().result).GetN());
            }
          }

          AND_THEN("each put in the buffer has the same id") {
            auto const &id_key = store.config().hash_key();
            for (auto const &items :
                 store.client().event_requests.front().GetTransactItems()) {
              REQUIRE(items.GetPut().GetItem().at(id_key.name).GetS() ==
                      id_value);
            }
          }

          AND_THEN("each put in the buffer has the same time-to-live") {
            auto const &ttl_field = *store.config().ttl_field();
            for (auto const &items :
                 store.client().event_requests.front().GetTransactItems()) {
              REQUIRE(items.GetPut().GetItem().at(ttl_field.name).GetN() ==
                      dynamodb::attribute_value(store.clock().result + week).GetN());
            }
          }

          AND_THEN("each put in the buffer has the same table") {
            auto const &table = store.config().table().get();
            for (auto const &items :
                 store.client().event_requests.front().GetTransactItems()) {
              REQUIRE(items.GetPut().GetTableName() == table);
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