#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/event_stream_buffer.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/views.h"

#include <algorithm>
#include <concepts>
#include <type_traits>
#include <utility>

namespace skizzay::cddd {
namespace event_stream_details_ {

template <typename... Ts> void commit_events(Ts const &...) = delete;

struct commit_events_fn final {
  template <typename T, concepts::identifier Id, concepts::version Version,
            concepts::event_stream_buffer EventStreamBuffer>
  requires requires(T t, Id id_value, Version const expected_version,
                    EventStreamBuffer buffer) {
    {commit_events(t, std::move(id_value), expected_version,
                   std::move(buffer))};
  }
  constexpr void operator()(T t, Id id_value, Version const expected_version,
                            EventStreamBuffer buffer) const
      noexcept(noexcept(commit_events(t, std::move(id_value), expected_version,
                                      std::move(buffer)))) {
    commit_events(t, std::move(id_value), expected_version, std::move(buffer));
  }

  template <typename T, concepts::identifier Id, concepts::version Version,
            concepts::event_stream_buffer EventStreamBuffer>
  requires requires(T t, Id id_value, Version const expected_version,
                    EventStreamBuffer buffer) {
    {t.commit_events(std::move(id_value), expected_version, std::move(buffer))};
  }
  constexpr void operator()(T t, Id const &id_value,
                            Version const expected_version,
                            EventStreamBuffer buffer) const
      noexcept(noexcept(t.commit_events(std::move(id_value), expected_version,
                                        std::move(buffer)))) {
    t.commit_events(std::move(id_value), expected_version, std::move(buffer));
  }

  template <typename T, concepts::identifier Id, std::signed_integral Version,
            concepts::event_stream_buffer EventStreamBuffer>
  constexpr void operator()(T t, Id id_value, Version const expected_version,
                            EventStreamBuffer buffer) const {
    (*this)(std::move_if_noexcept(t), std::move(id_value),
            narrow_cast<std::make_unsigned_t<Version>>(expected_version),
            std::move(buffer));
  }
};

struct rollback_to_fn final {
  template <typename T, concepts::identifier Id, concepts::version Version>
  requires requires(T t, Id const &id_value, Version const target_version) {
    {t.rollback_to(dereference(id_value), target_version)};
  }
  constexpr void operator()(T t, Id const &id_value,
                            Version const target_version) const
      noexcept(noexcept(t.rollback_to(dereference(id_value), target_version))) {
    t.rollback_to(dereference(id_value), target_version);
  }

  template <typename T, concepts::identifier Id, concepts::version Version>
  requires requires(T &&t, Id const &id_value, Version const target_version) {
    {rollback_to(t, dereference(id_value), target_version)};
  }
  constexpr void operator()(T &&t, Id const &id_value,
                            Version const target_version) const
      noexcept(noexcept(rollback_to(t, dereference(id_value),
                                    target_version))) {
    rollback_to(t, dereference(id_value), target_version);
  }
};
} // namespace event_stream_details_

inline namespace event_stream_fn_ {
inline constexpr event_stream_details_::commit_events_fn commit_events = {};
inline constexpr event_stream_details_::rollback_to_fn rollback_to = {};
} // namespace event_stream_fn_

namespace concepts {
template <typename T>
concept event_stream = std::copyable<T>;

template <typename T, typename DomainEvents>
concept event_stream_of =
    event_stream<T> && domain_event_sequence<DomainEvents> &&
    std::invocable < decltype(commit_events),
        std::add_lvalue_reference_t<T>,
        std::remove_reference_t<id_t<DomainEvents>>
const &,
    version_t<DomainEvents> >
        &&event_stream_buffer_of<event_stream_buffer_t<T>, DomainEvents> &&
            std::invocable<decltype(rollback_to),
                           std::add_lvalue_reference_t<T>, id_t<DomainEvents>,
                           std::ranges::range_size_t<event_stream_buffer_t<T>>>;
} // namespace concepts

namespace event_stream_details_ {
template <concepts::domain_event DomainEvent> struct add_event_interace {
  virtual ~add_event_interace() = default;

  virtual void add_event(DomainEvent &&domain_event) = 0;
};

template <typename Derived, concepts::domain_event DomainEvent>
struct add_event_impl : virtual add_event_interace<DomainEvent> {
  void add_event(DomainEvent &&domain_event) override {
    using cddd::add_event;
    add_event(static_cast<Derived *>(this)->get_impl(),
              std::move(domain_event));
  }
};
} // namespace event_stream_details_

} // namespace skizzay::cddd
