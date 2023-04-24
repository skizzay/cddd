#include <skizzay/cddd/dynamodb/dynamodb_event_source.h>

#include "skizzay/cddd/dynamodb/dynamodb_event_log_config.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_stream_buffer.h"
#include "skizzay/cddd/event_sourced.h"

#include "fakes.h"
#include <aws/dynamodb/DynamoDBClient.h>

using namespace skizzay::cddd;

namespace {
using A = test::fake_event<1>;
using B = test::fake_event<2>;
using C = test::fake_event<3>;

template <std::size_t N>
dynamodb::record make_record(test::fake_event<N> const &event) {
  dynamodb::record record{};
  record["hk"] = dynamodb::attribute_value(event.id);
  record["sk"] = dynamodb::attribute_value(event.version);
  record["t"] = dynamodb::attribute_value(event.timestamp);
  record["type"] = dynamodb::attribute_value(N);
  return record;
}

struct transformer {
  template <std::size_t N>
  Aws::DynamoDB::Model::Put operator()(test::fake_event<N> const &event) const {
    return Aws::DynamoDB::Model::Put{}.WithItem(make_record(event));
  }
};
using buffer_type = dynamodb::event_stream_buffer<transformer>;

inline constexpr auto create_event_stream_buffer = []() -> buffer_type {
  return buffer_type{};
};

struct fake_dispatcher {
  void dispatch(dynamodb::record const &encoded_event,
                skizzay::cddd::concepts::aggregate_root_of<A> auto
                    &aggregate_root) const {
    A a;
    dynamodb::get_item_or_default_value(encoded_event, "hk", a.id);
    dynamodb::get_item_or_default_value(encoded_event, "sk", a.version);
    dynamodb::get_item_or_default_value(encoded_event, "t", a.timestamp);
    skizzay::cddd::apply_event(aggregate_root, a);
  }
};

struct fake_client {
  Aws::DynamoDB::Model::QueryOutcome
  get_items(Aws::DynamoDB::Model::QueryRequest &&request) {
    event_requests.emplace_back(std::move(request));
    if (should_fail) {
      return Aws::DynamoDB::Model::QueryOutcome{};
    } else {
      return expected_event_result;
    }
  }

  Aws::DynamoDB::Model::GetItemOutcome
  get_item(Aws::DynamoDB::Model::GetItemRequest &&request) {
    item_requests.emplace_back(std::move(request));
    if (should_fail) {
      return Aws::DynamoDB::Model::GetItemOutcome{};
    } else {
      return expected_item_result;
    }
  }

  std::vector<Aws::DynamoDB::Model::QueryRequest> event_requests;
  Aws::DynamoDB::Model::QueryResult expected_event_result;
  std::vector<Aws::DynamoDB::Model::GetItemRequest> item_requests;
  Aws::DynamoDB::Model::GetItemResult expected_item_result;
  bool should_fail = false;
};

struct fake_store {
  dynamodb::event_source<fake_store> get_event_source() const noexcept {
    return dynamodb::event_source<fake_store>{*const_cast<fake_store *>(this)};
  }
  dynamodb::event_log_config const &config() const noexcept { return config_; }
  fake_client &client() noexcept { return client_; }
  test::fake_clock &clock() noexcept { return clock_; }
  fake_dispatcher &event_dispatcher() noexcept { return dispatcher_; }
  std::string const &query_key_condition_expression() const noexcept {
    return query_key_condition_expression_;
  }

  void add_event_to_expected_result(test::fake_event<1> const &event) {
    client_.expected_event_result.AddItems(make_record(event));
    dynamodb::record max_version_record{};
    max_version_record["hk"] = dynamodb::attribute_value(event.id);
    max_version_record["sk"] = dynamodb::attribute_value(0);
    max_version_record["t"] = dynamodb::attribute_value(event.timestamp);
    max_version_record[config().max_version_field().name] =
        dynamodb::attribute_value(event.version);
    client_.expected_item_result.SetItem(std::move(max_version_record));
  }

  dynamodb::event_log_config config_ = dynamodb::event_log_config{}.with_table(
      dynamodb::table_name{"some_table"});
  fake_client client_;
  test::fake_clock clock_;
  fake_dispatcher dispatcher_;
  std::string query_key_condition_expression_{"query key condition expression"};
};
} // namespace

TEST_CASE("DynamoDB Event Source") {
  fake_store store;
  test::fake_aggregate<buffer_type> aggregate_root;

  GIVEN("an event source provided by its store") {
    auto target = store.get_event_source();

    REQUIRE(skizzay::cddd::concepts::event_source<decltype(target)>);

    THEN("the source does not contain any events") {
      REQUIRE_FALSE(target.contains("some id"));
    }

    WHEN("loading an aggregate from history") {
      skizzay::cddd::load_from_history(target, aggregate_root);

      THEN("nothing was loaded into the aggregate") {
        REQUIRE(0 == skizzay::cddd::version(aggregate_root));
      }
    }

    WHEN("loading an aggregate from history") {
      skizzay::cddd::load_from_history(target, aggregate_root);

      THEN("nothing was loaded into the aggregate") {
        REQUIRE(0 == skizzay::cddd::version(aggregate_root));
      }
    }

    AND_GIVEN("events in the store") {
      std::string const id_value = "some id";
      std::size_t const starting_version = test::random_number_generator.next();
      std::size_t const num_events_to_load =
          test::random_number_generator.next();
      std::size_t const target_version = starting_version + num_events_to_load;
      aggregate_root.id = id_value;
      aggregate_root.version = starting_version;
      concepts::timestamp auto const timestamp =
          skizzay::cddd::now(store.clock());
      aggregate_root.id = id_value;
      aggregate_root.version = starting_version;
      std::ranges::for_each(
          std::ranges::views::iota(starting_version, target_version + 1),
          [&](std::size_t const event_version) {
            test::fake_event<1> event;
            event.id = id_value;
            event.version = event_version;
            event.timestamp = timestamp;
            store.add_event_to_expected_result(event);
          });

      THEN("the source does contains events") {
        REQUIRE(target.contains(id_value));
      }

      WHEN("loading an aggregate from history") {
        skizzay::cddd::load_from_history(target, aggregate_root,
                                         target_version);
        THEN("events were loaded starting where the aggregate root left off") {
          REQUIRE(skizzay::cddd::version(aggregate_root) == target_version);
        }
      }

      WHEN("querying for head version") {
        concepts::version auto const actual_head_version =
            target.head<version_t<decltype(aggregate_root)>>(aggregate_root.id);

        THEN("the head version is returned") {
          REQUIRE(actual_head_version == target_version);
        }
      }
    }

    AND_GIVEN("DynamoDB is in a bad state") {
      store.client().should_fail = true;

      WHEN("loading an aggregate from history") {
        bool caught_expection = false;
        try {
          skizzay::cddd::load_from_history(target, aggregate_root);
        } catch (
            dynamodb::history_load_error<Aws::DynamoDB::DynamoDBError> const
                &) {
          caught_expection = true;
        } catch (...) {
        }

        THEN("a history_load_error exception is thrown") {
          REQUIRE(caught_expection);
        }
      }

      WHEN("querying for head version") {
        bool caught_expection = false;
        try {
          [[maybe_unused]] concepts::version auto head_version =
              target.head(aggregate_root.id);
        } catch (
            dynamodb::history_load_error<Aws::DynamoDB::DynamoDBError> const
                &) {
          caught_expection = true;
        } catch (...) {
        }

        THEN("a history_load_error exception is thrown") {
          REQUIRE(caught_expection);
        }
      }
    }
  }
}