#pragma once

#include "skizzay/cddd/dynamodb/dynamodb_client.h"
#include "skizzay/cddd/dynamodb/dynamodb_clock.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_log_config.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_source.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_stream.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_stream_buffer.h"
#include "skizzay/cddd/event_stream.h"
#include "skizzay/cddd/factory.h"
#include "skizzay/cddd/timestamp.h"

#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/TransactWriteItemsRequest.h>
#include <chrono>
#include <concepts>
#include <ranges>

namespace skizzay::cddd::dynamodb {
namespace event_store_details_ {
template <typename>
inline constexpr bool is_dynamodb_event_stream_buffer_factory_v = false;

template <concepts::event_stream_buffer EventStreamBuffer>
requires std::same_as<std::ranges::range_value_t<EventStreamBuffer>,
                      Aws::DynamoDB::Model::Put>
inline constexpr bool
    is_dynamodb_event_stream_buffer_factory_v<EventStreamBuffer> = true;

template <typename EventDispatcher, std::invocable EventStreamBufferFactory,
          typename Client = basic_client,
          concepts::clock Clock = default_clock_t>
requires is_dynamodb_event_stream_buffer_factory_v<
    std::invoke_result_t<EventStreamBufferFactory>>
struct impl {
  constexpr impl(event_log_config config, EventDispatcher event_dispatcher,
                 EventStreamBufferFactory create_event_stream_buffer = {},
                 Client client = {}, Clock clock = {})
      : config_{std::move(config)}, client_{std::move_if_noexcept(client)},
        event_dispatcher_{std::move(event_dispatcher)},
        new_condition_expression_{"attribute_not_exists(" +
                                  config_.hash_key().name + ')'},
        update_condition_expression_{config_.sort_key().name_expression + '=' +
                                     config_.sort_key().value_expression},
        query_key_condition_expression_{
            '(' + config_.hash_key().name_expression + " = " +
            config_.hash_key().value_expression + ") AND (" +
            config_.sort_key().name_expression + " BETWEEN " +
            config_.sort_key().value_expression + "_min AND " +
            config_.sort_key().value_expression + "_max)"},
        create_event_stream_buffer_{
            std::move_if_noexcept(create_event_stream_buffer)},
        clock_{std::move_if_noexcept(clock)} {}

  constexpr concepts::event_stream auto get_event_stream() noexcept {
    return event_stream{*this};
  }

  constexpr concepts::event_source auto get_event_source() const noexcept {
    return event_source{*const_cast<impl *>(this)};
  }

  constexpr concepts::event_stream_buffer auto get_event_stream_buffer() const {
    return create_event_stream_buffer_();
  }

  constexpr Client &client() noexcept { return client_; }

  constexpr Clock &clock() noexcept { return clock_; }

  constexpr event_log_config const &config() const noexcept { return config_; }

  constexpr EventDispatcher &event_dispatcher() noexcept {
    return event_dispatcher_;
  }

  std::string const &new_condition_expression() const noexcept {
    return new_condition_expression_;
  }

  std::string const &update_condition_expression() const noexcept {
    return update_condition_expression_;
  }

  std::string const &query_key_condition_expression() const noexcept {
    return query_key_condition_expression_;
  }

private:
  event_log_config config_;
  Client client_;
  EventDispatcher event_dispatcher_;
  // These are optimizations. The idea is that we don't have to calculate these
  // expressions each time we instantiate an event stream/source.
  std::string new_condition_expression_;
  std::string update_condition_expression_;
  std::string query_key_condition_expression_;
  [[no_unique_address]] EventStreamBufferFactory create_event_stream_buffer_;
  [[no_unique_address]] Clock clock_;
};

template <typename T, typename U, typename V>
impl(event_log_config, T, U, V) -> impl<T, U, V, default_clock_t>;
} // namespace event_store_details_

template <typename EventDispatcher, std::invocable EventStreamBufferFactory,
          typename Client = basic_client,
          concepts::clock Clock = default_clock_t>
using event_store =
    event_store_details_::impl<EventDispatcher, EventStreamBufferFactory,
                               Client, Clock>;
} // namespace skizzay::cddd::dynamodb
