#pragma once

#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/version.h"

#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/QueryResult.h>
#include <aws/dynamodb/model/TransactWriteItem.h>
#include <aws/dynamodb/model/TransactWriteItemsResult.h>

namespace skizzay::cddd::dynamodb {
struct basic_client {
  Aws::DynamoDB::Model::TransactWriteItemsOutcome
  commit([[maybe_unused]] Aws::DynamoDB::Model::TransactWriteItemsRequest
             &&request) {
    return Aws::DynamoDB::Model::TransactWriteItemsResult{};
  }

  Aws::DynamoDB::Model::QueryOutcome
  get_events([[maybe_unused]] Aws::DynamoDB::Model::QueryRequest &&request) {
    return Aws::DynamoDB::Model::QueryResult{};
  }
};
} // namespace skizzay::cddd::dynamodb
