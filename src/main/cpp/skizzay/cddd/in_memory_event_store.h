#pragma once

#include "skizzay/cddd/aggregate_root.h"
#include "skizzay/cddd/domain_event.h"
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

template <concepts::clock Clock, concepts::domain_event... DomainEvents>
requires(0 < sizeof...(DomainEvents)) struct store_impl;

template <concepts::clock Clock, concepts::domain_event... DomainEvents>
struct event_stream final {
  using id_type = id_t<DomainEvents...>;
  using event_ptr = std::unique_ptr<event_interface<DomainEvents...>>;
  using buffer_type = std::vector<event_ptr>;
  using version_type = version_t<DomainEvents...>;
  using timestamp_type = timestamp_t<DomainEvents...>;

  explicit event_stream(auto &&id, version_type const starting_version,
                        store_impl<Clock, DomainEvents...> &store)
      : id_{std::forward<decltype(id)>(id)},
        starting_version_{starting_version}, store_{store} {}

  constexpr std::remove_cvref_t<id_type> const &id() const noexcept {
    return id_;
  }

  template <concepts::mutable_domain_event DomainEvent>
  requires(std::same_as<std::remove_cvref_t<DomainEvent>,
                        std::remove_cvref_t<DomainEvents>> ||
           ...) void add_event(DomainEvent &&domain_event) {
    update_event_stream_info(domain_event);
    pending_events_.emplace_back(
        std::make_unique<event_holder_impl<std::remove_cvref_t<DomainEvent>,
                                           DomainEvents...>>(
            std::move(domain_event)));
  }

  constexpr void commit_events() {
    version_type const next_starting_version = version();
    try {
      store_.commit(id(), pending_events_);
      pending_events_.clear();
      starting_version_ = next_starting_version;
    } catch (optimistic_concurrency_collision const &e) {
      starting_version_ = e.version_found();
      throw;
    }
  }

  constexpr void rollback() noexcept { pending_events_.clear(); }

  constexpr version_type version() const noexcept(
      is_nothrow_narrow_cast_v<version_type, typename buffer_type::size_type>) {
    return starting_version_ +
           narrow_cast<version_type>(std::size(pending_events_));
  }

  constexpr version_type next_version() const noexcept { return version() + 1; }

private:
  void
  update_event_stream_info(concepts::domain_event auto &domain_event) const {
    set_id(domain_event, id_);
    set_version(domain_event, next_version());
  }

  std::remove_cvref_t<id_type> id_;
  version_type starting_version_;
  store_impl<Clock, DomainEvents...> &store_;
  buffer_type pending_events_;
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

template <typename Aggregate, concepts::domain_event... DomainEvents>
requires(std::invocable<decltype(skizzay::cddd::apply),
                        std::add_lvalue_reference_t<Aggregate>,
                        std::remove_cvref_t<DomainEvents> const &>
             &&...) struct aggregate_visitor final
    : event_visitor<DomainEvents...>,
      aggregate_visitor_impl<aggregate_visitor<Aggregate, DomainEvents...>,
                             std::remove_cvref_t<DomainEvents>>... {
  aggregate_visitor(Aggregate &aggregate) noexcept : aggregate_{aggregate} {}

  Aggregate &get_aggregate() noexcept { return aggregate_; }

private:
  Aggregate &aggregate_;
};

template <concepts::domain_event... DomainEvents> struct buffer final {
  using id_type = id_t<DomainEvents...>;
  using version_type = version_t<DomainEvents...>;
  using timestamp_type = timestamp_t<DomainEvents...>;
  using event_ptr = std::unique_ptr<event_interface<DomainEvents...>>;
  using storage_type = std::vector<event_ptr>;

  typename storage_type::size_type version() const noexcept {
    std::shared_lock l_{m_};
    return std::size(storage_);
  }

  template <std::ranges::random_access_range Range>
  requires std::same_as<std::ranges::range_value_t<Range>, event_ptr>
  void append(timestamp_type const timestamp, Range &range) {
    using skizzay::cddd::version;

    auto const expected_version = version(std::ranges::begin(range)) - 1;
    auto const actual_version = std::size(storage_);
    std::lock_guard l_{m_};
    if (expected_version == actual_version) {
      storage_.reserve(actual_version + std::ranges::size(range));
      std::ranges::transform(std::make_move_iterator(std::ranges::begin(range)),
                             std::make_move_iterator(std::ranges::end(range)),
                             std::back_inserter(storage_),
                             [timestamp](event_ptr event) -> event_ptr {
                               event->set_timestamp(timestamp);
                               return event;
                             });
    } else {
      std::ostringstream message;
      message << "Saving events, expected version " << expected_version
              << ", but found " << actual_version;
      throw optimistic_concurrency_collision{message.str(), actual_version,
                                             expected_version};
    }
  }

  concepts::domain_event_range auto
  get_events(version_type const begin_version,
             version_type const target_version) {
    auto const begin_iterator = std::begin(storage_);
    return std::ranges::subrange(
        begin_iterator + begin_version - 1,
        begin_iterator + std::min(std::size(storage_), target_version));
  }

private:
  mutable std::shared_mutex m_;
  storage_type storage_;
};

template <concepts::domain_event... DomainEvents> struct event_source final {
  using event_ptr = std::unique_ptr<event_interface<DomainEvents...>>;
  using version_type = version_t<DomainEvents...>;

  explicit event_source(
      std::shared_ptr<buffer<DomainEvents...>> buffer_ptr) noexcept
      : buffer_{std::move(buffer_ptr)} {}

  template <concepts::aggregate_root<DomainEvents...> Aggregate>
  void load_from_history(Aggregate &aggregate,
                         version_t<decltype(aggregate)> const target_version) {
    std::unsigned_integral auto const aggregate_version = version(aggregate);
    assert((aggregate_version < target_version) &&
           "Aggregate version cannot exceed target version");

    if (nullptr != buffer_) {
      std::ranges::for_each(
          buffer_->get_events(aggregate_version + 1, target_version),
          [aggregate = aggregate_visitor<Aggregate, DomainEvents...>{
               aggregate}](event_ptr const &event) mutable {
            event->accept_event_visitor(aggregate);
          });
    }
  }

private:
  std::shared_ptr<buffer<DomainEvents...>> buffer_;
};

template <concepts::domain_event... DomainEvents>
event_source(std::shared_ptr<buffer<DomainEvents...>>)
    -> event_source<DomainEvents...>;

template <concepts::clock Clock, concepts::domain_event... DomainEvents>
requires(0 < sizeof...(DomainEvents)) struct store_impl {
  friend event_stream<Clock, DomainEvents...>;

  using id_type = id_t<DomainEvents...>;
  using version_type = version_t<DomainEvents...>;
  using timestamp_type = timestamp_t<DomainEvents...>;
  using buffer_type = buffer<DomainEvents...>;
  using event_ptr = std::unique_ptr<event_interface<DomainEvents...>>;

  event_stream<Clock, DomainEvents...>
  get_event_stream(auto const &id) noexcept {
    using skizzay::cddd::version;

    std::shared_ptr<buffer_type> const buffer_ptr = find_buffer(id);
    return event_stream{std::forward<decltype(id)>(id),
                        (nullptr == buffer_ptr ? 0 : version(*buffer_ptr)),
                        *this};
  }

  event_source<DomainEvents...> get_event_source(auto const &id) noexcept {
    return event_source{find_buffer(id)};
  }

  bool has_events_for(auto const &id) const noexcept {
    return nullptr != find_buffer(id);
  }

private:
  template <std::ranges::random_access_range Range>
  requires std::same_as<std::ranges::range_value_t<Range>, event_ptr>
  void commit(id_type id, Range &range) {
    using skizzay::cddd::now;
    if (!std::empty(range)) {
      std::shared_ptr<buffer_type> buffer_ptr = find_buffer(id);
      if (nullptr == buffer_ptr) {
        buffer_ptr = std::make_shared<buffer_type>();
        buffer_ptr->append(current_timestamp(), range);
        std::lock_guard l_{m_};
        event_buffers_.emplace(id, std::move(buffer_ptr));
      } else {
        buffer_ptr->append(current_timestamp(), range);
      }
    }
  }

  std::shared_ptr<buffer_type> find_buffer(id_type id) const noexcept {
    std::shared_lock l_{m_};
    if (auto const buffer_iter = event_buffers_.find(id);
        std::end(event_buffers_) != buffer_iter) {
      return buffer_iter->second;
    } else {
      return nullptr;
    }
  }

  timestamp_type current_timestamp() {
    using skizzay::cddd::now;
    return std::chrono::time_point_cast<typename timestamp_type::duration>(
        now(clock_));
  }

  mutable std::shared_mutex m_;
  [[no_unique_address]] Clock clock_;
  std::unordered_map<std::remove_cvref_t<id_type>, std::shared_ptr<buffer_type>>
      event_buffers_;
};
} // namespace in_memory_event_store_details_

template <concepts::clock Clock, concepts::domain_event... DomainEvents>
using in_memory_event_store =
    in_memory_event_store_details_::store_impl<Clock, DomainEvents...>;
} // namespace skizzay::cddd
