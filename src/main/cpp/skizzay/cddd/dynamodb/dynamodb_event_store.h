#pragma once

#include "skizzay/cddd/dynamodb/dynamodb_event_source.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_stream.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_stream_buffer.h"
#include "skizzay/cddd/event_stream.h"
#include "skizzay/cddd/timestamp.h"

#include <aws/dynamodb/model/Put.h>
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

template <typename> inline constexpr bool is_complete_type_v = false;

template <typename T>
requires(0 < sizeof(T)) inline constexpr bool is_complete_type_v<T> = true;

using default_clock_t =
    std::conditional_t<is_complete_type_v<std::chrono::utc_clock>,
                       std::chrono::utc_clock, std::chrono::system_clock>;

template <std::invocable EventStreamBufferFactory,
          concepts::clock Clock = default_clock_t>
requires is_dynamodb_event_stream_buffer_factory_v<
    std::invoke_result_t<EventStreamBufferFactory>>
struct impl {
  constexpr impl(EventStreamBufferFactory create_event_stream_buffer = {},
                 [[maybe_unused]] Clock clock = {})
      : create_event_stream_buffer_{
            std::move_if_noexcept(create_event_stream_buffer)} {};

  constexpr concepts::event_stream auto get_event_stream() {
    return event_stream{*this};
  }

  constexpr concepts::event_source auto get_event_source() const {
    return event_source{*this};
  }

  constexpr concepts::event_stream_buffer auto get_event_stream_buffer() const {
    return create_event_stream_buffer_();
  }

private:
  EventStreamBufferFactory create_event_stream_buffer_;
};

template <typename T> impl(T) -> impl<T, default_clock_t>;
} // namespace event_store_details_

template <typename EventStreamBufferFactory,
          concepts::clock Clock = event_store_details_::default_clock_t>
using event_store = event_store_details_::impl<EventStreamBufferFactory, Clock>;
} // namespace skizzay::cddd::dynamodb
