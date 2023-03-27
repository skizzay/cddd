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

// template <concepts::clock Clock, concepts::domain_event_sequence
// DomainEvents> requires(!DomainEvents::empty) struct store_impl;

// template <typename Store> struct event_stream final {
//   using id_type = id_t<Store>;
//   using buffer_type = mapped_event_stream_buffer<
//       typename Store::domain_event_sequence,
//       decltype(wrap_domain_events<typename Store::domain_event_sequence>)>;
//   using version_type = version_t<Store>;

//   explicit event_stream(Store &store) : store_{store} {}

//   constexpr void
//   commit_events(std::remove_reference_t<id_type> const &id, buffer_type
//   buffer,
//                 std::convertible_to<version_type> auto const
//                 expected_version) {
//     if (not std::empty(buffer)) {
//       timestamp_t<Store> const timestamp = now(store_.clock_);
//       for (auto &&[i, element] : views::enumerate(buffer)) {
//         set_timestamp(element, timestamp);
//         set_version(element,
//                     narrow_cast<version_type>(i) + expected_version + 1);
//       }
//       store_.event_buffers_.get_or_put(id)->append(std::move(buffer),
//                                                    expected_version);
//     }
//   }

//   constexpr buffer_type get_event_stream_buffer() const {
//     return buffer_type{
//         wrap_domain_events<typename Store::domain_event_sequence>};
//   }

//   constexpr void
//   rollback_to(std::remove_reference_t<id_type> const &id,
//               std::convertible_to<version_type> auto const expected_version)
//               {
//     if (auto buffer = store_.event_buffers_.get(id); buffer) {
//       buffer->rollback_to(expected_version);
//     }
//   }

// private:
//   Store &store_;
// };

// template <typename Derived, concepts::domain_event DomainEvent>
// struct aggregate_visitor_impl : virtual event_visitor_interface<DomainEvent>
// {
//   void visit(DomainEvent const &domain_event) override {
//     using skizzay::cddd::apply;
//     apply(static_cast<Derived *>(this)->get_aggregate(), domain_event);
//   }
// };

// template <typename, typename> struct aggregate_visitor;

// template <
//     concepts::domain_event... DomainEvents,
//     concepts::aggregate_root<domain_event_sequence<DomainEvents...>>
//     Aggregate>
// struct aggregate_visitor<domain_event_sequence<DomainEvents...>, Aggregate>
//     final
//     : event_visitor<domain_event_sequence<DomainEvents...>>,
//       aggregate_visitor_impl<
//           aggregate_visitor<domain_event_sequence<DomainEvents...>,
//           Aggregate>, DomainEvents>... {
//   aggregate_visitor(Aggregate &aggregate) noexcept : aggregate_{aggregate} {}

//   Aggregate &get_aggregate() noexcept { return aggregate_; }

// private:
//   Aggregate &aggregate_;
// };

// template <concepts::domain_event_sequence DomainEvents> struct buffer final {
//   using id_type = id_t<DomainEvents>;
//   using version_type = version_t<DomainEvents>;
//   using timestamp_type = timestamp_t<DomainEvents>;
//   using event_ptr = std::unique_ptr<event_wrapper<DomainEvents>>;
//   using storage_type = std::vector<event_ptr>;

//   typename storage_type::size_type version() const noexcept {
//     std::shared_lock l_{m_};
//     return std::size(storage_);
//   }

//   void append(std::ranges::sized_range auto &&events,
//               version_type const expected_version) {
//     using skizzay::cddd::version;

//     std::lock_guard l_{m_};
//     auto const actual_version = std::size(storage_);
//     if (expected_version == actual_version) {
//       storage_.insert(std::end(storage_),
//                       std::move_iterator(std::ranges::begin(events)),
//                       std::move_iterator(std::ranges::end(events)));
//     } else {
//       std::ostringstream message;
//       message << "Saving events, expected version " << expected_version
//               << ", but found " << actual_version;
//       throw optimistic_concurrency_collision{message.str(),
//       expected_version};
//     }
//   }

//   concepts::domain_event_range auto
//   get_events(version_type const begin_version,
//              version_type const target_version) {
//     std::shared_lock l_{m_};
//     auto const begin_iterator = std::begin(storage_);
//     return std::ranges::subrange(
//         begin_iterator + begin_version - 1,
//         begin_iterator + std::min(std::size(storage_), target_version));
//   }

// private:
//   mutable std::shared_mutex m_;
//   storage_type storage_;
// };

// template <concepts::clock Clock, concepts::domain_event_sequence
// DomainEvents> struct event_source final {
//   using event_ptr = std::unique_ptr<event_wrapper<DomainEvents>>;
//   using version_type = version_t<DomainEvents>;

//   explicit event_source(store_impl<Clock, DomainEvents> &store) noexcept
//       : store_{store} {}

//   template <concepts::aggregate_root<DomainEvents> Aggregate>
//   void load_from_history(Aggregate &aggregate,
//                          version_t<decltype(aggregate)> const target_version)
//                          {
//     std::unsigned_integral auto const aggregate_version = version(aggregate);
//     assert((aggregate_version < target_version) &&
//            "Aggregate version cannot exceed target version");

//     // if (auto const buffer = store_.find_buffer(id(aggregate));
//     //     nullptr != buffer) {
//     //   std::ranges::for_each(
//     //       buffer->get_events(aggregate_version + 1, target_version),
//     //       [aggregate = aggregate_visitor<DomainEvents,
//     //       Aggregate>{aggregate}](
//     //           event_ptr const &event) mutable {
//     //         event->accept_event_visitor(aggregate);
//     //       });
//     // }
//   }

// private:
//   store_impl<Clock, DomainEvents> &store_;
// };

// template <concepts::clock Clock> struct store_impl {
//   friend event_stream<store_impl<Clock>>;
//   friend event_source<Clock>;

//   // using id_type = id_t<DomainEvents>;
//   // using version_type = version_t<DomainEvents>;
//   // using timestamp_type = timestamp_t<DomainEvents>;
//   // using event_ptr = std::unique_ptr<event_wrapper<DomainEvents>>;
//   // using domain_event_sequence = DomainEvents;

//   // constexpr version_type
//   // version_head(std::remove_reference_t<id_type> const &id) const noexcept
//   {
//   //   auto const event_buffer = find_buffer(id);
//   //   return nullptr == event_buffer ? 0
//   //                                  : narrow_cast<version_type>(
//   // skizzay::cddd::version(*event_buffer));
//   // }

//   event_stream<store_impl<Clock>> get_event_stream() noexcept {
//     return event_stream{*this};
//   }

//   event_source<Clock, DomainEvents> get_event_source() noexcept {
//     return event_source{*this};
//   }

//   bool
//   has_events_for(std::remove_reference_t<id_type> const &id) const noexcept {
//     return nullptr != find_buffer(id);
//   }

// private:
//   template <std::ranges::random_access_range Range>
//   requires std::same_as<std::ranges::range_value_t<Range>, event_ptr>
//   void commit(id_type id, Range &range) {
//     if (!std::empty(range)) {
//       event_buffers_.get_or_add(id)->append(current_timestamp(), range);
//     }
//   }

//   std::shared_ptr<buffer<DomainEvents>>
//   find_buffer(std::remove_reference_t<id_type> const &id) const noexcept {
//     return event_buffers_.get(
//         id, provide_null_value<std::shared_ptr<buffer<DomainEvents>>>{});
//   }

//   timestamp_type current_timestamp() {
//     using skizzay::cddd::now;
//     return std::chrono::time_point_cast<typename timestamp_type::duration>(
//         now(clock_));
//   }

//   [[no_unique_address]] Clock clock_;
//   concurrent_table<std::shared_ptr<buffer<DomainEvents>>, id_type>
//       event_buffers_;
// };

template <concepts::domain_event_sequence DomainEvents, concepts::clock Clock>
requires(!DomainEvents::empty) struct impl {
  using event_stream_t = event_stream<impl<DomainEvents, Clock>>;
  using event_source_t = event_source<impl<DomainEvents, Clock>>;
  // friend event_stream_t;
  // friend event_stream<impl<DomainEvents, Clock>>;

  using domain_event_sequence = DomainEvents;
  using buffer_type = mapped_event_stream_buffer<
      std::unique_ptr<event_wrapper<DomainEvents>>,
      std::remove_const_t<decltype(wrap_domain_events<DomainEvents>)>>;

  constexpr impl(DomainEvents const = {}, Clock clock = {})
      : clock_{std::move_if_noexcept(clock)} {}

  constexpr event_stream_t get_event_stream() { return event_stream_t{*this}; }

  constexpr event_source_t get_event_source() { return event_source_t{*this}; }

  constexpr buffer_type get_event_stream_buffer() const {
    return buffer_type{wrap_domain_events<DomainEvents>};
  }

  constexpr concurrent_table<std::shared_ptr<buffer<DomainEvents>>,
                             std::remove_cvref_t<id_t<DomainEvents>>> &
  event_buffers() noexcept {
    return buffers_;
  }

  constexpr Clock &clock() noexcept { return clock_; }

private:
  concurrent_table<std::shared_ptr<buffer<DomainEvents>>,
                   std::remove_cvref_t<id_t<DomainEvents>>>
      buffers_;
  [[no_unique_address]] Clock clock_;
};
} // namespace event_store_details_

template <concepts::domain_event_sequence DomainEvents,
          concepts::clock Clock = std::chrono::system_clock>
using event_store = event_store_details_::impl<DomainEvents, Clock>;
} // namespace skizzay::cddd::in_memory
