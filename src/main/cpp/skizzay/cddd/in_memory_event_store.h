#pragma once

#include "skizzay/cddd/aggregate_root.h"
#include "skizzay/cddd/concurrent_repository.h"
#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/event_sourced.h"
#include "skizzay/cddd/event_stream.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/narrow_cast.h"
#include "skizzay/cddd/optimistic_concurrency_collision.h"
#include "skizzay/cddd/timestamp.h"
#include "skizzay/cddd/version.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <concepts>
#include <iterator>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <vector>

namespace skizzay::cddd {
namespace in_memory_event_store_details_ {

template <concepts::clock Clock, concepts::domain_event_sequence DomainEvents>
requires(!DomainEvents::empty) struct store_impl;

template <concepts::clock Clock, concepts::domain_event_sequence DomainEvents>
struct event_stream final
    : event_stream_base<event_stream<Clock, DomainEvents>, Clock,
                        std::unique_ptr<event_wrapper<DomainEvents>>,
                        DomainEvents> {
  using base_type =
      event_stream_base<event_stream<Clock, DomainEvents>, Clock,
                        std::unique_ptr<event_wrapper<DomainEvents>>,
                        DomainEvents>;
  using typename base_type::buffer_type;
  using typename base_type::element_type;
  using typename base_type::id_type;
  using typename base_type::timestamp_type;
  using typename base_type::version_type;

  explicit event_stream(Clock clock, store_impl<Clock, DomainEvents> &store)
      : base_type{std::move(clock)}, store_{store} {}

  void commit_buffered_events(buffer_type &&buffer,
                              std::remove_reference_t<id_type> const &id,
                              timestamp_type const,
                              version_type const expected_version) {
    store_.event_buffers_.get_or_put(id)->append(std::move(buffer),
                                                 expected_version);
  }

  element_type make_buffer_element(
      concepts::mutable_domain_event auto &&domain_event) const {
    return event_wrapper<DomainEvents>::from_domain_event(
        std::move(domain_event));
  }

  void populate_commit_info(std::remove_reference_t<id_type> const &,
                            timestamp_type const timestamp,
                            version_type const version, auto &event_ptr) {
    set_timestamp(*event_ptr, timestamp);
    set_version(*event_ptr, version);
  }

private:
  store_impl<Clock, DomainEvents> &store_;
};

template <typename Derived, concepts::domain_event DomainEvent>
struct aggregate_visitor_impl : virtual event_visitor_interface<DomainEvent> {
  void visit(DomainEvent const &domain_event) override {
    using skizzay::cddd::apply;
    apply(static_cast<Derived *>(this)->get_aggregate(), domain_event);
  }
};

template <concepts::clock Clock, concepts::domain_event... DomainEvents>
event_stream(auto &&, std::unsigned_integral auto const,
             store_impl<Clock, DomainEvents...> &)
    -> event_stream<Clock, DomainEvents...>;

template <typename, typename> struct aggregate_visitor;

template <
    concepts::domain_event... DomainEvents,
    concepts::aggregate_root<domain_event_sequence<DomainEvents...>> Aggregate>
struct aggregate_visitor<domain_event_sequence<DomainEvents...>, Aggregate>
    final
    : event_visitor<domain_event_sequence<DomainEvents...>>,
      aggregate_visitor_impl<
          aggregate_visitor<domain_event_sequence<DomainEvents...>, Aggregate>,
          DomainEvents>... {
  aggregate_visitor(Aggregate &aggregate) noexcept : aggregate_{aggregate} {}

  Aggregate &get_aggregate() noexcept { return aggregate_; }

private:
  Aggregate &aggregate_;
};

template <concepts::domain_event_sequence DomainEvents> struct buffer final {
  using id_type = id_t<DomainEvents>;
  using version_type = version_t<DomainEvents>;
  using timestamp_type = timestamp_t<DomainEvents>;
  using event_ptr = std::unique_ptr<event_wrapper<DomainEvents>>;
  using storage_type = std::vector<event_ptr>;

  typename storage_type::size_type version() const noexcept {
    std::shared_lock l_{m_};
    return std::size(storage_);
  }

  void append(storage_type &&events, version_type const expected_version) {
    using skizzay::cddd::version;

    std::lock_guard l_{m_};
    auto const actual_version = std::size(storage_);
    if (expected_version == actual_version) {
      storage_.insert(std::end(storage_),
                      std::move_iterator(std::ranges::begin(events)),
                      std::move_iterator(std::ranges::end(events)));
    } else {
      std::ostringstream message;
      message << "Saving events, expected version " << expected_version
              << ", but found " << actual_version;
      throw optimistic_concurrency_collision{message.str(), expected_version};
    }
  }

  concepts::domain_event_range auto
  get_events(version_type const begin_version,
             version_type const target_version) {
    std::shared_lock l_{m_};
    auto const begin_iterator = std::begin(storage_);
    return std::ranges::subrange(
        begin_iterator + begin_version - 1,
        begin_iterator + std::min(std::size(storage_), target_version));
  }

private:
  mutable std::shared_mutex m_;
  storage_type storage_;
};

template <concepts::clock Clock, concepts::domain_event_sequence DomainEvents>
struct event_source final {
  using event_ptr = std::unique_ptr<event_wrapper<DomainEvents>>;
  using version_type = version_t<DomainEvents>;

  explicit event_source(store_impl<Clock, DomainEvents> &store) noexcept
      : store_{store} {}

  template <concepts::aggregate_root<DomainEvents> Aggregate>
  void load_from_history(Aggregate &aggregate,
                         version_t<decltype(aggregate)> const target_version) {
    std::unsigned_integral auto const aggregate_version = version(aggregate);
    assert((aggregate_version < target_version) &&
           "Aggregate version cannot exceed target version");

    if (auto const buffer = store_.find_buffer(id(aggregate));
        nullptr != buffer) {
      std::ranges::for_each(
          buffer->get_events(aggregate_version + 1, target_version),
          [aggregate = aggregate_visitor<DomainEvents, Aggregate>{aggregate}](
              event_ptr const &event) mutable {
            event->accept_event_visitor(aggregate);
          });
    }
  }

private:
  store_impl<Clock, DomainEvents> &store_;
};

template <concepts::clock Clock, concepts::domain_event_sequence DomainEvents>
requires(!DomainEvents::empty) struct store_impl {
  friend event_stream<Clock, DomainEvents>;
  friend event_source<Clock, DomainEvents>;

  using id_type = id_t<DomainEvents>;
  using version_type = version_t<DomainEvents>;
  using timestamp_type = timestamp_t<DomainEvents>;
  using buffer_type = buffer<DomainEvents>;
  using event_ptr = std::unique_ptr<event_wrapper<DomainEvents>>;

  constexpr version_type
  version_head(std::remove_reference_t<id_type> const &id) const noexcept {
    auto const event_buffer = find_buffer(id);
    return nullptr == event_buffer ? 0
                                   : narrow_cast<version_type>(
                                         skizzay::cddd::version(*event_buffer));
  }

  event_stream<Clock, DomainEvents> get_event_stream() noexcept {
    return event_stream{clock_, *this};
  }

  event_source<Clock, DomainEvents> get_event_source() noexcept {
    return event_source{*this};
  }

  bool
  has_events_for(std::remove_reference_t<id_type> const &id) const noexcept {
    return nullptr != find_buffer(id);
  }

private:
  template <std::ranges::random_access_range Range>
  requires std::same_as<std::ranges::range_value_t<Range>, event_ptr>
  void commit(id_type id, Range &range) {
    if (!std::empty(range)) {
      event_buffers_.get_or_add(id)->append(current_timestamp(), range);
    }
  }

  std::shared_ptr<buffer_type>
  find_buffer(std::remove_reference_t<id_type> const &id) const noexcept {
    return event_buffers_.get(
        id, provide_null_value<std::shared_ptr<buffer_type>>{});
  }

  timestamp_type current_timestamp() {
    using skizzay::cddd::now;
    return std::chrono::time_point_cast<typename timestamp_type::duration>(
        now(clock_));
  }

  [[no_unique_address]] Clock clock_;
  concurrent_table<std::shared_ptr<buffer_type>, id_type> event_buffers_;
};
} // namespace in_memory_event_store_details_

template <concepts::clock Clock, concepts::domain_event_sequence DomainEvents>
using in_memory_event_store =
    in_memory_event_store_details_::store_impl<Clock, DomainEvents>;
} // namespace skizzay::cddd
