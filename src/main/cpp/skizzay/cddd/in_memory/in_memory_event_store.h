#pragma once

#include "skizzay/cddd/concurrent_repository.h"
#include "skizzay/cddd/in_memory/in_memory_event_buffer.h"
#include "skizzay/cddd/in_memory/in_memory_event_source.h"
#include "skizzay/cddd/in_memory/in_memory_event_stream.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <concepts>
#include <iterator>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <vector>

namespace skizzay::cddd::in_memory {
namespace event_store_details_ {

template <std::invocable EventStreamBufferFactory, typename BufferStore,
          concepts::clock Clock = std::chrono::system_clock>
requires concepts::event_stream_buffer<
    std::invoke_result_t<EventStreamBufferFactory>>
struct impl {
  using event_stream_buffer_t = std::invoke_result_t<EventStreamBufferFactory>;
  using event_stream_t =
      event_stream<impl<EventStreamBufferFactory, BufferStore, Clock>>;
  using event_source_t =
      event_source<impl<EventStreamBufferFactory, BufferStore, Clock> const>;

  using buffer_store = BufferStore;

  constexpr impl(EventStreamBufferFactory event_stream_buffer_factory = {},
                 BufferStore buffers = {}, Clock clock = {})
      : buffers_{std::move_if_noexcept(buffers)},
        event_stream_buffer_factory_{
            std::move_if_noexcept(event_stream_buffer_factory)},
        clock_{std::move_if_noexcept(clock)} {}

  constexpr event_stream_t get_event_stream() { return event_stream_t{*this}; }

  constexpr event_source_t get_event_source() const {
    return event_source_t{*this};
  }

  constexpr event_stream_buffer_t get_event_stream_buffer() const {
    return event_stream_buffer_factory_();
  }

  constexpr buffer_store &event_buffers() noexcept { return buffers_; }

  constexpr buffer_store const &event_buffers() const noexcept {
    return buffers_;
  }

  constexpr Clock &clock() noexcept { return clock_; }

private:
  buffer_store buffers_;
  [[no_unique_address]] EventStreamBufferFactory event_stream_buffer_factory_;
  [[no_unique_address]] Clock clock_;
};
} // namespace event_store_details_

template <concepts::domain_event DomainEvent,
          typename Alloc = std::allocator<
              std::pair<std::remove_reference_t<id_t<DomainEvent>> const,
                        std::shared_ptr<buffer<DomainEvent>>>>>
using concurrent_buffer_store =
    concurrent_table<std::shared_ptr<buffer<DomainEvent>>,
                     std::remove_cvref_t<id_t<DomainEvent>>, Alloc>;
                     
template <concepts::domain_event DomainEvent,
          typename Alloc = std::allocator<
              std::pair<std::remove_reference_t<id_t<DomainEvent>> const,
                        std::shared_ptr<buffer<DomainEvent>>>>>
using serial_buffer_store =
    std::unordered_map<std::remove_cvref_t<id_t<DomainEvent>>,
                       std::shared_ptr<buffer<DomainEvent>>,
                       std::hash<std::remove_cvref_t<id_t<DomainEvent>>>,
                       std::equal_to<std::remove_cvref_t<id_t<DomainEvent>>>,
                       Alloc>;

template <std::invocable EventStreamBufferFactory, typename BufferStore,
          concepts::clock Clock = std::chrono::system_clock>
using event_store =
    event_store_details_::impl<EventStreamBufferFactory, BufferStore, Clock>;
} // namespace skizzay::cddd::in_memory
