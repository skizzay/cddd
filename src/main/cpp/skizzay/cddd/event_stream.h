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
  requires requires(T &t, Id id_value, Version const expected_version,
                    EventStreamBuffer buffer) {
    {commit_events(t, std::move(id_value), expected_version,
                   std::move(buffer))};
  }
  constexpr void operator()(T &t, Id id_value, Version const expected_version,
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
  requires std::invocable<commit_events_fn const, T &, Id,
                          std::make_unsigned_t<Version>, EventStreamBuffer>
  constexpr void operator()(T t, Id id_value, Version const expected_version,
                            EventStreamBuffer buffer) const {
    // noexcept(std::is_nothrow_invocable_v<
    //          commit_events_fn const, std::add_lvalue_reference_t<T>,
    //          std::remove_cvref_t<Id>, std::make_unsigned_t<Version>,
    //          EventStreamBuffer>) {
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

template <typename T>
using event_stream_buffer_t =
    std::invoke_result_t<decltype(skizzay::cddd::get_event_stream_buffer), T>;

namespace concepts {
template <typename T>
concept event_stream = std::destructible<T>;

template <typename T, typename DomainEvents>
concept event_stream_of =
    event_stream<T> && domain_event_sequence<DomainEvents> &&
    std::invocable < decltype(skizzay::cddd::commit_events),
        std::add_lvalue_reference_t<T>,
        std::remove_reference_t<id_t<DomainEvents>>
const &,
    version_t<DomainEvents> >
        &&event_stream_buffer_of<event_stream_buffer_t<T>, DomainEvents> &&
            std::invocable<decltype(skizzay::cddd::rollback_to),
                           std::add_lvalue_reference_t<T>, id_t<DomainEvents>,
                           std::ranges::range_size_t<event_stream_buffer_t<T>>>;
} // namespace concepts

namespace event_stream_details_ {
template <concepts::domain_event DomainEvent> struct add_event_interace {
  virtual void add_event(DomainEvent &&domain_event) = 0;
};

template <typename Derived, concepts::domain_event DomainEvent>
struct add_event_impl : virtual add_event_interace<DomainEvent> {
  void add_event(DomainEvent &&domain_event) override {
    using skizzay::cddd::add_event;
    add_event(static_cast<Derived *>(this)->get_impl(),
              std::move(domain_event));
  }
};
} // namespace event_stream_details_

// template <typename> struct event_stream_interface;

// template <concepts::domain_event... DomainEvents>
// struct event_stream_interface<domain_event_sequence<DomainEvents...>>
//     : virtual event_stream_details_::add_event_interace<DomainEvents>... {
//   using id_type = id_t<DomainEvents...>;
//   using version_type = version_t<DomainEvents...>;

//   virtual void commit_events(id_type id_value,
//                              version_type const expected_version,
//                              EventStreamBuffer buffer) = 0;
//   virtual void rollback() = 0;

//   static inline std::unique_ptr<
//       event_stream_interface<domain_event_sequence<DomainEvents...>>>
//   type_erase(
//       concepts::event_stream_of<domain_event_sequence<DomainEvents...>> auto
//           event_stream) {
//     struct impl final
//         : event_stream_interface,
//           event_stream_details_::add_event_impl<impl, DomainEvents>... {

//       void commit_events(version_t<DomainEvents...> expected_version)
//       noexcept(
//           noexcept(skizzay::cddd::commit_events(this->impl_,
//                                                 expected_version))) override
//                                                 {
//         return skizzay::cddd::commit_events(impl_, expected_version);
//       }

//       void rollback() noexcept(
//           noexcept(skizzay::cddd::rollback(this->impl_))) override {
//         return skizzay::cddd::rollback(impl_);
//       }

//       std::remove_cvref_t<decltype(event_stream)> impl_;
//     };

//     return std::make_unique<impl>(std::move(event_stream));
//   }
// };

// template <typename Derived, concepts::clock Clock, std::copyable Transform,
//           concepts::domain_event_sequence DomainEvents,
//           typename Alloc = event_stream_buffer_details_::default_alloc_t<
//               DomainEvents, Transform>>
// requires std::same_as<Derived, std::remove_cvref_t<Derived>>
// struct event_stream_base {
//   using id_type = id_t<DomainEvents>;
//   using buffer_type =
//       mapped_event_stream_buffer<DomainEvents, Transform, Alloc>;
//   using element_type = std::ranges::range_value_t<buffer_type>;
//   using version_type = version_t<DomainEvents>;
//   using timestamp_type = timestamp_t<DomainEvents>;

//   constexpr void
//   commit_events(std::remove_reference_t<id_type> const &id, buffer_type buffer,
//                 std::convertible_to<version_type> auto const expected_version) {
//     if (not std::empty(buffer)) {
//       timestamp_t<DomainEvents> const timestamp = now(clock_);
//       for (auto &&[i, element] : views::enumerate(buffer)) {
//         version_type const event_version =
//             narrow_cast<version_type>(i) + expected_version + 1;
//         derived().populate_commit_info(id, timestamp, event_version, element);
//       }
//       derived().commit_buffered_events(
//           std::move(buffer), id, timestamp,
//           narrow_cast<version_type>(expected_version));
//     }
//   }

//   constexpr buffer_type get_event_stream_buffer() const {
//     return buffer_type{transform_};
//   }

// protected:
//   explicit event_stream_base(Clock clock, Transform transform)
//       : clock_{std::move_if_noexcept(clock)}, transform_{std::move_if_noexcept(
//                                                   transform)} {}

// private:
//   constexpr Derived &derived() noexcept {
//     return *static_cast<Derived *>(this);
//   }

//   [[no_unique_address]] Clock clock_;
//   [[no_unique_address]] Transform transform_;
// };

} // namespace skizzay::cddd
