#pragma once

#include "skizzay/cddd/dynamodb/dynamodb_attribute_value.h"
#include "skizzay/cddd/dynamodb/dynamodb_operation_failed_error.h"
#include "skizzay/cddd/history_load_failed.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/optimistic_concurrency_collision.h"
#include "skizzay/cddd/timestamp.h"

#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/dynamodb/model/QueryRequest.h>
#include <aws/dynamodb/model/TransactWriteItemsRequest.h>
#include <concepts>
#include <type_traits>
#include <vector>

namespace skizzay::cddd::dynamodb {
template <typename E>
using history_load_error = operation_failed_error<history_load_failed, E>;

namespace client_details_ {

template <concepts::identifier Id, std::unsigned_integral Version,
          concepts::timestamp Timestamp>
struct client {
  virtual Aws::DynamoDB::DynamoDBClient &handle() noexcept = 0;
  virtual Aws::Vector<
      Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>>
  get_events(Id const &id, Version min_version, Version max_version) = 0;
  virtual void
  commit(std::vector<Aws::DynamoDB::Model::TransactWriteItem> &&buffer) = 0;
  virtual Aws::DynamoDB::Model::AttributeValue get_version(Id const &id) = 0;
  virtual Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>
  key(Id const &id, Version version) = 0;
  virtual void initialize(Aws::DynamoDB::Model::Put &put, std::string_view type,
                          Id const &id) = 0;
};

template <concepts::identifier Id, std::unsigned_integral Version,
          concepts::timestamp Timestamp>
struct basic_client : client<Id, Version, Timestamp> {
  basic_client(
      Aws::String table_name, Aws::String key_name, Aws::String version_name,
      Aws::String max_version_name,
      Aws::Client::ClientConfiguration const &client_configuration = {})
      : client_{client_configuration}, table_name_{std::move(table_name)},
        key_name_{std::move(key_name)}, version_name_{std::move(version_name)},
        max_version_name_{std::move(max_version_name)} {}

  basic_client(
      Aws::String table_name, Aws::String key_name, Aws::String version_name,
      Aws::String max_version_name, Aws::Auth::AWSCredentials &credentials,
      Aws::Client::ClientConfiguration const &client_configuration = {})
      : client_{credentials, client_configuration},
        table_name_{std::move(table_name)}, key_name_{std::move(key_name)},
        version_name_{std::move(version_name)}, max_version_name_{std::move(
                                                    max_version_name)} {}

  basic_client(
      Aws::String table_name, Aws::String key_name, Aws::String version_name,
      Aws::String max_version_name,
      std::shared_ptr<Aws::Auth::AWSCredentialsProvider> const
          &credentials_provider,
      Aws::Client::ClientConfiguration const &client_configuration = {})
      : client_{credentials_provider, client_configuration},
        table_name_{std::move(table_name)}, key_name_{std::move(key_name)},
        version_name_{std::move(version_name)}, max_version_name_{std::move(
                                                    max_version_name)} {}

  Aws::DynamoDB::DynamoDBClient &handle() noexcept final { return client_; }

  Aws::Vector<Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>>
  get_events(Id const &id, Version min_version, Version max_version) final {
    Aws::DynamoDB::Model::QueryRequest request =
        get_query_request(id, min_version, max_version);
    auto response = get_event_batch(request);
    if (std::empty(response.GetLastEvaluatedKey())) {
      return response.GetItems();
    } else {
      Aws::Vector<Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>>
          result;
      auto copy_items_to_result = [&result](auto const &items) {
        result.reserve(std::size(result) + std::size(items));
        std::copy(std::move_iterator(std::begin(items)),
                  std::move_iterator(std::end(items)),
                  std::back_inserter(result));
      };
      copy_items_to_result(response.GetItems());
      while (not std::empty(response.GetLastEvaluatedKey())) {
        response = get_event_batch(
            request.WithExclusiveStartKey(response.GetLastEvaluatedKey()));
        copy_items_to_result(response.GetItems());
      }
      return result;
    }
  }

  void
  commit(Aws::Vector<Aws::DynamoDB::Model::TransactWriteItem> items) final {
    auto const response =
        get_commit_response(get_commit_request(std::move(items)));
    if (!response.IsSuccess()) {
      throw commit_error{response.GetError()};
    }
  }

  Aws::DynamoDB::Model::AttributeValue get_version(Id const &id) final {
    auto extract_version_from_outcome = [this](auto &version_record) {
      auto const max_version_iterator = version_record.find(max_version_name_);
      return (std::end(version_record) == max_version_iterator)
                 ? Aws::DynamoDB::Model::AttributeValue{}
                 : max_version_iterator->second;
    };
    auto outcome = get_version_response(get_version_request(id));
    if (outcome.IsSuccess()) {
      return extract_version_from_outcome(outcome.GetResult().GetItem());
    } else {
      throw operation_failed_error<std::runtime_error,
                                   Aws::DynamoDB::DynamoDBError>{
          outcome.GetError()};
    }
  }

  Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>
  key(Id const &id, Version version) final {
    return {{key_name_, attribute_value(id)},
            {version_name_, attribute_value(version)}};
  }

  void initialize(Aws::DynamoDB::Model::Put &put, std::string_view type,
                  Id const &id) override {
    put.WithTableName(table_name_)
        .AddItem(key_name_, attribute_value(id))
        .AddItem(type_name_, attribute_value(type))
        .WithConditionExpression("attribute_not_exists(" + key_name_ + ")");
  }

private:
  virtual Aws::DynamoDB::Model::TransactWriteItemsRequest get_commit_request(
      Aws::Vector<Aws::DynamoDB::Model::TransactWriteItem> &&items) {
    return Aws::DynamoDB::Model::TransactWriteItemsRequest{}.WithTransactItems(
        std::move(items));
  }

  virtual Aws::DynamoDB::Model::TransactWriteItemsOutcome get_commit_response(
      Aws::DynamoDB::Model::TransactWriteItemsRequest const &request) {
    return client_.TransactWriteItems(request);
  }

  virtual Aws::DynamoDB::Model::GetItemRequest
  get_version_request(Id const &id) {
    return Aws::DynamoDB::Model::GetItemRequest{}
        .WithTableName(table_name_)
        .WithKey(key(id, Version{0}))
        .WithProjectionExpression(max_version_name_)
        .WithConsistentRead(true);
  }

  virtual Aws::DynamoDB::Model::GetItemOutcome
  get_version_response(Aws::DynamoDB::Model::GetItemRequest const &request) {
    return client_.GetItem(request);
  }

  inline Aws::DynamoDB::Model::QueryResult
  get_event_batch(Aws::DynamoDB::Model::QueryRequest const &request) {
    auto outcome = get_query_response(request);
    if (outcome.IsSuccess()) {
      return outcome.GetResultWithOwnership();
    } else {
      throw operation_failed_error{outcome.GetError()};
    }
  }

  virtual Aws::DynamoDB::Model::QueryRequest
  get_query_request(Id const &id, Version min_version, Version max_version) {
    return Aws::DynamoDB::Model::QueryRequest{}
        .WithTableName(table_name_)
        .WithConsistentRead(true)
        .WithKeyConditionExpression(
            "(#pk = :pk) AND (#sk BETWEEN :sk_min AND :sk_max)")
        .WithExpressionAttributeNames(make_expression_attribute_names())
        .WithExpressionAttributeValues(
            make_expression_attribute_values(id, min_version, max_version));
  }

  virtual Aws::DynamoDB::Model::QueryOutcome
  get_query_response(Aws::DynamoDB::Model::QueryRequest const &request) {
    return client_.Query(request);
  }

  inline Aws::Map<Aws::String, Aws::String>
  make_expression_attribute_names() const {
    return {{"#pk", key_name_}, {"#sk", version_name_}};
  }

  inline Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>
  make_expression_attribute_values(Id const &id, Version const min_version,
                                   Version const max_version) const {
    return {{":pk", attribute_value(id)},
            {":sk_min", attribute_value(min_version)},
            {":sk_max", attribute_value(max_version)}};
  }

  Aws::DynamoDB::DynamoDBClient client_;
  Aws::String table_name_;
  Aws::String key_name_;
  Aws::String version_name_;
  Aws::String max_version_name_;
  Aws::String type_name_;
};
} // namespace client_details_

template <typename Id, typename Version, typename Timestamp>
using client = client_details_::client<std::remove_cvref_t<Id>,
                                       std::remove_cvref_t<Version>,
                                       std::remove_cvref_t<Timestamp>>;

template <typename Id, typename Version, typename Timestamp>
using basic_client =
    client_details_::basic_client<std::remove_cvref_t<Id>,
                                  std::remove_cvref_t<Version>,
                                  std::remove_cvref_t<Timestamp>>;

} // namespace skizzay::cddd::dynamodb
