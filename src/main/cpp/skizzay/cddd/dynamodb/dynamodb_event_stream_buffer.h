#pragma once

#include "skizzay/cddd/event_stream_buffer.h"

namespace skizzay::cddd::dynamodb {
template <std::copy_constructible Transform,
          typename Alloc = std::allocator<Aws::DynamoDB::Model::Put>>
using event_stream_buffer =
    mapped_event_stream_buffer<Aws::DynamoDB::Model::Put, Transform, Alloc>;

template <typename> struct is_dynamodb_event_stream_buffer : std::false_type {};

template <typename T, typename A>
struct is_dynamodb_event_stream_buffer<
    mapped_event_stream_buffer<Aws::DynamoDB::Model::Put, T, A>>
    : std::true_type {};
} // namespace skizzay::cddd::dynamodb
