#pragma once

#include "skizzay/cddd/dynamodb/dynamodb_event_log_config.h"
#include "skizzay/cddd/dynamodb/dynamodb_operation_failed_error.h"
#include <aws/dynamodb/model/CreateTableRequest.h>
#include <aws/dynamodb/model/DeleteTableRequest.h>
#include <aws/dynamodb/model/KeySchemaElement.h>
#include <exception>

namespace skizzay::cddd::dynamodb {
struct event_log_table {
  explicit event_log_table(Aws::DynamoDB::DynamoDBClient &client,
                           event_log_config const &config)
      : client_{client}, config_{config} {
    auto request = Aws::DynamoDB::Model::CreateTableRequest{}
                       .WithTableName(config_.table_name())
                       .WithAttributeDefinitions(key_definitions())
                       .WithKeySchema(key_schema())
                       .WithProvisionedThroughput(provisioned_throughput());
    auto outcome = client_.CreateTable(request);

    if (!outcome.IsSuccess()) {
      throw operation_failed_error{outcome.GetError()};
    }
  }

  ~event_log_table() noexcept(false) {
    auto request = Aws::DynamoDB::Model::DeleteTableRequest{}.WithTableName(
        config_.table_name());
    auto outcome = client_.DeleteTable(request);

    if (!outcome.IsSuccess() &&
        Aws::DynamoDB::DynamoDBErrors::TABLE_NOT_FOUND !=
            outcome.GetError().GetErrorType()) {
      throw operation_failed_error{outcome.GetError()};
    }
  }

private:
  Aws::Vector<Aws::DynamoDB::Model::AttributeDefinition>
  key_definitions() const {
    using Aws::DynamoDB::Model::AttributeDefinition;
    using Aws::DynamoDB::Model::ScalarAttributeType;
    return {AttributeDefinition{}
                .WithAttributeName(config_.key_name())
                .WithAttributeType(ScalarAttributeType::S),
            AttributeDefinition{}
                .WithAttributeName(config_.version_name())
                .WithAttributeType(ScalarAttributeType::N)};
  }

  Aws::Vector<Aws::DynamoDB::Model::KeySchemaElement> key_schema() const {
    using Aws::DynamoDB::Model::KeySchemaElement;
    using Aws::DynamoDB::Model::KeyType;
    return {KeySchemaElement{}
                .WithAttributeName(config_.key_name())
                .WithKeyType(KeyType::HASH),
            KeySchemaElement{}
                .WithAttributeName(config_.version_name())
                .WithKeyType(KeyType::RANGE)};
  }

  Aws::DynamoDB::Model::ProvisionedThroughput provisioned_throughput() const {
    return Aws::DynamoDB::Model::ProvisionedThroughput{}
        .WithReadCapacityUnits(10LL)
        .WithWriteCapacityUnits(10LL);
  }

  Aws::DynamoDB::DynamoDBClient &client_;
  event_log_config const &config_;
};
} // namespace skizzay::cddd::dynamodb
