#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/dynamodb/dynamodb_deser.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_dispatcher.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_log_config.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_source.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_stream.h"
#include "skizzay/cddd/factory.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/timestamp.h"

#include <memory>

namespace skizzay::cddd::dynamodb {

template <concepts::clock Clock, concepts::domain_event... DomainEvents>
struct event_store {
  using id_type = id_t<DomainEvents...>;

  template <typename QueryRequestFactory =
                default_factory<Aws::DynamoDB::Model::QueryRequest>,
            typename CommitRequestFactory = default_factory<
                Aws::DynamoDB::Model::TransactWriteItemsRequest>>
  explicit event_store(
      Clock clock, event_log_config config,
      std::unique_ptr<serializer<DomainEvents...>> event_serializer,
      event_dispatcher<DomainEvents...> dispatcher,
      QueryRequestFactory get_query_request = {},
      CommitRequestFactory get_commit_request = {})
      : clock_{std::move(clock)}, config_{std::move(config)},
        serializer_{std::move(serializer)}, dispatcher_{std::move(dispatcher)},
        get_query_request_{std::move(get_query_request)},
        get_commit_request_{std::move(get_commit_request)} {}

  event_stream<Clock, DomainEvents...> get_event_stream(id_type id) {
    return {id, serializer_, config_, client_, clock_, get_commit_request_};
  }

  event_source<DomainEvents...>
  get_event_source(std::remove_cvref_t<id_type> id) {
    return {std::move(id), dispatcher_, config_, client_, get_query_request_};
  }

private:
  Clock clock_;
  event_log_config config_;
  std::unique_ptr<serializer<DomainEvents...>> serializer_;
  event_dispatcher<DomainEvents...> dispatcher_;
  std::function<Aws::DynamoDB::Model::QueryRequest()> get_query_request_;
  std::function<Aws::DynamoDB::Model::TransactWriteItemsRequest()>
      get_commit_request_;
};
} // namespace skizzay::cddd::dynamodb