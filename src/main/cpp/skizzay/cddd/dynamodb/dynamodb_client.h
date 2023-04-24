#pragma once

#include "skizzay/cddd/dynamodb/dynamodb_event_log_config.h"
#include "skizzay/cddd/dynamodb/dynamodb_operation_failed_error.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/version.h"

#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/CreateTableRequest.h>
#include <aws/dynamodb/model/DeleteTableRequest.h>
#include <aws/dynamodb/model/GetItemResult.h>
#include <aws/dynamodb/model/QueryResult.h>
#include <aws/dynamodb/model/TransactWriteItem.h>
#include <aws/dynamodb/model/TransactWriteItemsResult.h>

namespace skizzay::cddd::dynamodb {
using create_table_error =
    operation_failed_error<std::runtime_error, Aws::DynamoDB::DynamoDBError>;
using delete_table_error =
    operation_failed_error<std::runtime_error, Aws::DynamoDB::DynamoDBError>;

struct basic_client {
  template <typename... Args>
  requires std::constructible_from<Aws::DynamoDB::DynamoDBClient, Args...>
  explicit basic_client(Args &&...args)
      : client_{std::make_unique<Aws::DynamoDB::DynamoDBClient>(
            std::forward<Args>(args)...)} {}

  basic_client(basic_client const &) = delete;
  basic_client(basic_client &&) = default;

  Aws::DynamoDB::Model::TransactWriteItemsOutcome
  commit(Aws::DynamoDB::Model::TransactWriteItemsRequest &&request) {
    return client_->TransactWriteItems(request);
  }

  Aws::DynamoDB::Model::QueryOutcome
  get_items(Aws::DynamoDB::Model::QueryRequest &&request) {
    return client_->Query(request);
  }

  Aws::DynamoDB::Model::GetItemOutcome
  get_item(Aws::DynamoDB::Model::GetItemRequest &&request) {
    return client_->GetItem(request);
  }

  void create_table(event_log_config const &config) {
    using namespace Aws::DynamoDB::Model;
    auto request =
        CreateTableRequest{}
            .WithTableName(config.table().get())
            .WithAttributeDefinitions(Aws::Vector<AttributeDefinition>{
                AttributeDefinition{}
                    .WithAttributeName(config.hash_key().name)
                    .WithAttributeType(ScalarAttributeType::S),
                AttributeDefinition{}
                    .WithAttributeName(config.sort_key().name)
                    .WithAttributeType(ScalarAttributeType::N)})
            .WithKeySchema(Aws::Vector<KeySchemaElement>{
                KeySchemaElement{}
                    .WithAttributeName(config.hash_key().name)
                    .WithKeyType(KeyType::HASH),
                KeySchemaElement{}
                    .WithAttributeName(config.sort_key().name)
                    .WithKeyType(KeyType::RANGE)})
            .WithProvisionedThroughput(ProvisionedThroughput{}
                                           .WithReadCapacityUnits(1LL)
                                           .WithWriteCapacityUnits(1LL));
    auto outcome = client_->CreateTable(request);

    if (!outcome.IsSuccess()) {
      throw create_table_error{outcome.GetError()};
    }
  }

  void delete_table(event_log_config const &config) {
    using namespace Aws::DynamoDB::Model;
    auto request = DeleteTableRequest{}.WithTableName(config.table().get());
    auto outcome = client_->DeleteTable(request);

    if (!outcome.IsSuccess() &&
        Aws::DynamoDB::DynamoDBErrors::TABLE_NOT_FOUND !=
            outcome.GetError().GetErrorType()) {
      throw skizzay::cddd::dynamodb::delete_table_error{outcome.GetError()};
    }
  }

private:
  std::unique_ptr<Aws::DynamoDB::DynamoDBClient> client_;
};
} // namespace skizzay::cddd::dynamodb
