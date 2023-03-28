#pragma once

#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/domain_event_wrapper.h"
#include "skizzay/cddd/narrow_cast.h"
#include "skizzay/cddd/nullable.h"
#include "skizzay/cddd/optimistic_concurrency_collision.h"

#include <cassert>
#include <shared_mutex>
#include <sstream>

namespace skizzay::cddd::in_memory {

namespace buffer_details_ {
inline constexpr auto copy_shared_lock =
    [](std::shared_lock<std::shared_mutex> const &l) noexcept
    -> std::shared_lock<std::shared_mutex> {
  return skizzay::cddd::is_null(l.mutex())
             ? std::shared_lock<std::shared_mutex>{}
             : std::shared_lock<std::shared_mutex>{*l.mutex(), std::adopt_lock};
};

template <concepts::domain_event DomainEvent>
struct locked_event_range
    : std::ranges::view_interface<locked_event_range<DomainEvent>> {
  struct iterator {
    friend locked_event_range<DomainEvent>;
    using base_iterator = typename std::vector<DomainEvent>::iterator;
    using difference_type = std::iter_difference_t<base_iterator>;
    using value_type = std::iter_value_t<base_iterator>;
    using pointer = typename std::iterator_traits<base_iterator>::pointer;
    using reference = std::iter_reference_t<base_iterator>;
    using iterator_category =
        typename std::iterator_traits<base_iterator>::iterator_category;

    constexpr iterator() noexcept = default;

    constexpr iterator(iterator const &other)
        : l_{copy_shared_lock(other.l_)}, iter_{other.iter_} {}

    constexpr iterator(iterator &&) noexcept = default;

    constexpr iterator &operator=(iterator &&) noexcept = default;

    constexpr iterator &operator=(iterator const &rhs) {
      if (this != &rhs) {
        *this = std::move(iterator{rhs});
      }
      return *this;
    }

    constexpr reference operator*() const noexcept { return *iter_; }

    constexpr pointer operator->() noexcept { return iter_.operator->(); }

    friend constexpr bool operator==(iterator const &l,
                                     iterator const &r) noexcept {
      return l.iter_ == r.iter_;
    }

    friend constexpr auto operator<=>(iterator const &l,
                                      iterator const &r) noexcept {
      return l.iter_ <=> r.iter_;
    }

    friend constexpr iterator operator+(iterator i, difference_type const n) {
      return i += n;
    }

    friend constexpr iterator operator-(iterator i, difference_type const n) {
      return i -= n;
    }

    friend constexpr iterator operator+(difference_type const n, iterator i) {
      return i += n;
    }

    friend constexpr iterator operator-(difference_type const n, iterator i) {
      return i -= n;
    }

    friend constexpr difference_type operator-(iterator l,
                                               iterator r) noexcept {
      return l.iter_ - r.iter_;
    }

    constexpr iterator &operator++() {
      ++iter_;
      return *this;
    }

    constexpr iterator operator++(int) {
      iterator result{*this};
      ++iter_;
      return result;
    }

    constexpr iterator &operator--() {
      --iter_;
      return *this;
    }

    constexpr iterator &operator+=(std::ptrdiff_t n) {
      iter_ += n;
      return *this;
    }

    constexpr iterator &operator-=(std::ptrdiff_t n) {
      iter_ -= n;
      return *this;
    }

    constexpr decltype(auto) operator[](auto index) { return iter_[index]; }

    constexpr decltype(auto) operator[](auto index) const {
      return iter_[index];
    }

  private:
    constexpr iterator(std::shared_lock<std::shared_mutex> l,
                       base_iterator iter)
        : l_{std::move(l)}, iter_{std::move(iter)} {}

    std::shared_lock<std::shared_mutex> l_;
    base_iterator iter_;
  };

  constexpr iterator begin() const noexcept {
    return {copy_shared_lock(l_), begin_};
  }
  constexpr iterator end() const noexcept {
    return {copy_shared_lock(l_), end_};
  }

  constexpr locked_event_range() noexcept = default;
  constexpr locked_event_range(
      std::shared_lock<std::shared_mutex> &&l,
      typename std::vector<DomainEvent>::iterator b,
      typename std::vector<DomainEvent>::iterator e) noexcept
      : l_{std::move(l)}, begin_{std::move(b)}, end_{std::move(e)} {}

  std::shared_lock<std::shared_mutex> l_;
  typename std::vector<DomainEvent>::iterator begin_;
  typename std::vector<DomainEvent>::iterator end_;
};
} // namespace buffer_details_

template <concepts::domain_event_sequence DomainEvents> struct buffer final {
  using event_t = std::unique_ptr<event_wrapper<DomainEvents>>;

  constexpr version_t<DomainEvents> version() const noexcept {
    std::shared_lock l_{m_};
    return narrow_cast<version_t<DomainEvents>>(std::size(events_),
                                                std::terminate);
  }

  template <std::ranges::sized_range EventBuffer>
  requires std::same_as<event_t, std::ranges::range_value_t<EventBuffer>>
  constexpr void append(EventBuffer &&events,
                        version_t<DomainEvents> const expected_version) {
    std::lock_guard l_{m_};
    concepts::version auto const actual_version = std::size(events_);
    if (expected_version == actual_version) {
      events_.insert(std::end(events_),
                     std::move_iterator(std::ranges::begin(events)),
                     std::move_iterator(std::ranges::end(events)));
    } else {
      std::ostringstream what;
      what << "Saving events, expected version " << expected_version
           << ", but found " << actual_version;
      throw optimistic_concurrency_collision{what.str(), expected_version};
    }
  }

  std::ranges::sized_range auto
  get_events(version_t<DomainEvents> const begin_version,
             version_t<DomainEvents> const target_version) {
    assert(begin_version <= target_version);
    std::shared_lock l_{m_};
    auto const begin_iterator = std::ranges::begin(events_);
    return buffer_details_::locked_event_range<event_t>{
        std::move(l_), begin_iterator + begin_version - 1,
        begin_iterator + std::min(std::size(events_), target_version)};
  }

  void rollback_to(concepts::version auto const target_version) {
    std::lock_guard l_{m_};
    if (target_version < std::size(events_)) {
      events_.erase(std::ranges::begin(events_) + target_version,
                    std::ranges::end(events_));
    }
  }

private:
  mutable std::shared_mutex m_;
  std::vector<event_t> events_;
};
} // namespace skizzay::cddd::in_memory
