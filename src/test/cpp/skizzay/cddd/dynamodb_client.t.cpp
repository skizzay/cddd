#include <skizzay/cddd/dynamodb/dynamodb_client.h>

#include "skizzay/cddd/dynamodb/dynamodb_attribute_value.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_log_config.h"

#include "fakes.h"
#include <aws/core/Aws.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/dynamodb/model/QueryRequest.h>
#include <aws/dynamodb/model/TransactWriteItemsRequest.h>

#include <utility>

using namespace skizzay::cddd;
using namespace Aws::DynamoDB::Model;

namespace {
struct aws_sdk_raii final {
  explicit aws_sdk_raii(Aws::SDKOptions options) : options_{std::move(options)} {
    Aws::InitAPI(options_);
  }

  ~aws_sdk_raii() { Aws::ShutdownAPI(options_); }

private:
  Aws::SDKOptions const options_;
};

struct event_log_table final {
  explicit event_log_table(dynamodb::basic_client &client,
                           dynamodb::event_log_config const &config)
      : client_{client}, config_{config} {
    try {
      client_.create_table(config_);
    } catch (dynamodb::create_table_error const &e) {
      // Just in case a previous test run failed to clean up after itself.
      if (e.error() == Aws::DynamoDB::DynamoDBErrors::RESOURCE_IN_USE) {
        client_.delete_table(config_);
        client_.create_table(config_);
      }
    }
  }

  ~event_log_table() noexcept(false) { client_.delete_table(config_); }

private:
  dynamodb::basic_client &client_;
  dynamodb::event_log_config const &config_;
};
} // namespace

TEST_CASE("DynamoDB client") {
  Aws::SDKOptions options;
  aws_sdk_raii aws_sdk{options};
  Aws::Client::ClientConfiguration client_configuration{"default"};
  client_configuration.endpointOverride = "http://localhost:4566";
  auto config = dynamodb::event_log_config{}.with_table(
      dynamodb::table_name{"test_table"});

  GIVEN("A DynamoDB client") {
    std::string const id_value = "test_id";
    dynamodb::basic_client target{client_configuration};
    event_log_table table{target, config};

    WHEN("items are requested") {
      auto query_request =
          QueryRequest{}
              .WithTableName(config.table().get())
              .WithKeyConditionExpression(
                  "(#hk = :hk) AND (#sk BETWEEN :sk_min AND :sk_max)")
              .WithExpressionAttributeNames({
                  {"#hk", config.hash_key().name},
                  {"#sk", config.sort_key().name},
              })
              .WithExpressionAttributeValues(
                  {{":hk", dynamodb::attribute_value(id_value)},
                   {":sk_min", dynamodb::attribute_value(1u)},
                   {":sk_max", dynamodb::attribute_value(
                                   std::numeric_limits<unsigned>::max())}});
      auto query_result = target.get_items(std::move(query_request));
      THEN("the result is successful") { REQUIRE(query_result.IsSuccess()); }
      THEN("the result contains no items") {
        REQUIRE(query_result.GetResult().GetItems().empty());
      }
    }

    WHEN("an item is requested") {
      auto get_item_request =
          GetItemRequest{}
              .WithTableName(config.table().get())
              .WithKey(
                  {{config.hash_key().name,
                    dynamodb::attribute_value(id_value)},
                   {config.sort_key().name, dynamodb::attribute_value(0u)}});

      auto get_item_result = target.get_item(std::move(get_item_request));
      THEN("the result is successful") { REQUIRE(get_item_result.IsSuccess()); }
      THEN("the result does not contain the item") {
        REQUIRE(std::empty(get_item_result.GetResult().GetItem()));
      }
    }

    WHEN("items have been populated") {
      test::fake_clock clock;
      concepts::timestamp auto const timestamp = skizzay::cddd::now(clock);
      auto transact_write_items_request =
          TransactWriteItemsRequest{}.WithTransactItems(
              {TransactWriteItem{}.WithPut(
                   Put{}
                       .WithItem({{config.hash_key().name,
                                   dynamodb::attribute_value(id_value)},
                                  {config.sort_key().name,
                                   dynamodb::attribute_value(0u)},
                                  {"t", dynamodb::attribute_value(timestamp)},
                                  {"v", dynamodb::attribute_value(2u)}})
                       .WithTableName(config.table().get())),
               TransactWriteItem{}.WithPut(
                   Put{}
                       .WithItem({{config.hash_key().name,
                                   dynamodb::attribute_value(id_value)},
                                  {config.sort_key().name,
                                   dynamodb::attribute_value(1u)},
                                  {"t", dynamodb::attribute_value(timestamp)}})
                       .WithTableName(config.table().get())),
               TransactWriteItem{}.WithPut(
                   Put{}
                       .WithItem({{config.hash_key().name,
                                   dynamodb::attribute_value(id_value)},
                                  {config.sort_key().name,
                                   dynamodb::attribute_value(2u)},
                                  {"t", dynamodb::attribute_value(timestamp)}})
                       .WithTableName(config.table().get()))});

      auto transact_write_items_result =
          target.commit(std::move(transact_write_items_request));
      THEN("the result is successful") {
        REQUIRE(transact_write_items_result.IsSuccess());
      }

      AND_WHEN("items are requested") {
        auto query_request =
            QueryRequest{}
                .WithTableName(config.table().get())
                .WithKeyConditionExpression(
                    "(#hk = :hk) AND (#sk BETWEEN :sk_min AND :sk_max)")
                .WithExpressionAttributeNames({
                    {"#hk", config.hash_key().name},
                    {"#sk", config.sort_key().name},
                })
                .WithExpressionAttributeValues(
                    {{":hk", dynamodb::attribute_value(id_value)},
                     {":sk_min", dynamodb::attribute_value(1u)},
                     {":sk_max", dynamodb::attribute_value(
                                     std::numeric_limits<unsigned>::max())}});
        auto query_result = target.get_items(std::move(query_request));
        THEN("the result is successful") { REQUIRE(query_result.IsSuccess()); }
        THEN("the result contains requested items") {
          REQUIRE(2 == query_result.GetResult().GetCount());
        }
      }

      AND_WHEN("an item is requested") {
        auto get_item_request =
            GetItemRequest{}
                .WithTableName(config.table().get())
                .WithKey(
                    {{config.hash_key().name,
                      dynamodb::attribute_value(id_value)},
                     {config.sort_key().name, dynamodb::attribute_value(0u)}});

        auto get_item_result = target.get_item(std::move(get_item_request));
        THEN("the result is successful") {
          REQUIRE(get_item_result.IsSuccess());
        }
        THEN("the result contains the item") {
          REQUIRE(get_item_result.GetResult().GetItem().at("t") ==
                  dynamodb::attribute_value(clock.result));
        }
      }
    }
  }
}