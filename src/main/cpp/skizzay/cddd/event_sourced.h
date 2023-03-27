#pragma once

#include "skizzay/cddd/dereference.h"
#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/version.h"
#include <functional>
#include <memory>
#include <variant>

namespace skizzay::cddd {
namespace cpo_details_ {

template <typename... Ts> void apply_event(Ts const &...) = delete;

struct apply_event_fn final {
  template <typename T, concepts::domain_event DomainEvent>
  requires requires(T &t, DomainEvent const &domain_event) {
    {dereference(t).apply_event(dereference(domain_event))};
  }
  constexpr void operator()(T &t, DomainEvent const &domain_event) const
      noexcept(
          noexcept(dereference(t).apply_event(dereference(domain_event)))) {
    dereference(t).apply_event(dereference(domain_event));
  }

  template <typename T, concepts::domain_event DomainEvent>
  requires requires(T &t, DomainEvent const &domain_event) {
    {apply_event(dereference(t), dereference(domain_event))};
  }
  constexpr void operator()(T &t, DomainEvent const &domain_event) const
      noexcept(noexcept(apply_event(dereference(t), domain_event))) {
    apply_event(dereference(t), dereference(domain_event));
  }

  // template <typename T, concepts::domain_event DomainEvent>
  // constexpr void
  // operator()(T &t, std::shared_ptr<DomainEvent> const &domain_event) const
  //     noexcept(std::is_nothrow_invocable_v<apply_fn const, T &,
  //                                          DomainEvent const &>) {
  //   (*this)()
  // }

  // template <typename T, concepts::domain_event DomainEvent>
  // constexpr void
  // operator()(T &t, std::shared_ptr<DomainEvent const> const &domain_event)
  // const
  //     noexcept(std::is_nothrow_invocable_v<apply_fn const, T &,
  //                                          DomainEvent const &>) {
  //   std::invoke(*this, t, *domain_event);
  // }

  // template <typename T, concepts::domain_event DomainEvent, typename Deleter>
  // constexpr void
  // operator()(T &t,
  //            std::unique_ptr<DomainEvent, Deleter> const &domain_event) const
  //     noexcept(std::is_nothrow_invocable_v<apply_fn const, T &,
  //                                          DomainEvent const &>) {
  //   std::invoke(*this, t, *domain_event);
  // }

  // template <typename T, concepts::domain_event DomainEvent, typename Deleter>
  // constexpr void operator()(
  //     T &t,
  //     std::unique_ptr<DomainEvent const, Deleter> const &domain_event) const
  //     noexcept(std::is_nothrow_invocable_v<apply_fn const, T &,
  //                                          DomainEvent const &>) {
  //   std::invoke(*this, t, *domain_event);
  // }

  // template <typename T, concepts::domain_event DomainEvent>
  // constexpr void
  // operator()(T &t, std::reference_wrapper<DomainEvent> const domain_event)
  // const
  //     noexcept(std::is_nothrow_invocable_v<apply_fn const, T &,
  //                                          DomainEvent const &>) {
  //   std::invoke(*this, t, domain_event.get());
  // }

  // template <typename T, concepts::domain_event DomainEvent>
  // constexpr void
  // operator()(T &t, std::reference_wrapper<DomainEvent const> const
  // domain_event)
  //     const noexcept {
  //   std::invoke(*this, t, domain_event.get());
  // }

  template <typename T, concepts::domain_event... DomainEvents>
  constexpr void
  operator()(T &t, std::variant<DomainEvents...> const &domain_event) const
      noexcept((std::is_nothrow_invocable_v<apply_event_fn const, T &,
                                            DomainEvents const &> &&
                ...)) {
    std::visit(
        [&](auto const &domain_event) noexcept {
          std::invoke(*this, t, domain_event);
        },
        domain_event);
  }
};

template <typename... Ts> void load_from_history(Ts const &...) = delete;

struct load_from_history_fn final {
  template <typename EventSource, concepts::versioned Aggregate>
  requires requires(EventSource &event_source, Aggregate &aggregate,
                    version_t<Aggregate> const target_version) {
    dereference(event_source)
        .load_from_history(dereference(aggregate), target_version);
  }
  constexpr void operator()(EventSource &event_source, Aggregate &aggregate,
                            version_t<Aggregate> const target_version) const
      noexcept(noexcept(dereference(event_source)
                            .load_from_history(dereference(aggregate),
                                               target_version))) {
    dereference(event_source)
        .load_from_history(dereference(aggregate), target_version);
  }

  template <typename EventSource, concepts::versioned Aggregate>
  requires requires(EventSource &event_source, Aggregate &aggregate,
                    version_t<Aggregate> const target_version) {
    load_from_history(dereference(event_source), dereference(aggregate),
                      target_version);
  }
  constexpr void operator()(EventSource &event_source, Aggregate &aggregate,
                            version_t<Aggregate> const target_version) const
      noexcept(noexcept(load_from_history(dereference(event_source),
                                          dereference(aggregate),
                                          target_version))) {
    load_from_history(dereference(event_source), dereference(aggregate),
                      target_version);
  }

  template <typename EventSource, concepts::identifiable AggregateRoot>
  requires concepts::versioned<AggregateRoot> && std::invocable<
      load_from_history_fn const, std::add_lvalue_reference_t<EventSource>,
      std::add_lvalue_reference_t<AggregateRoot>, version_t<AggregateRoot>>
  constexpr void operator()(EventSource &event_source,
                            AggregateRoot &aggregate_root) const
      noexcept(std::is_nothrow_invocable_v<
               load_from_history_fn const,
               std::add_lvalue_reference_t<EventSource>,
               std::add_lvalue_reference_t<AggregateRoot>,
               version_t<AggregateRoot>>) {
    (*this)(event_source, aggregate_root,
            std::numeric_limits<version_t<AggregateRoot>>::max());
  }
};

template <typename... Ts> void load_from_snapshot(Ts const &...) = delete;

struct load_from_snapshot_fn final {
  template <typename SnapshotSource, concepts::versioned Aggregate>
  requires requires(SnapshotSource &snapshot_source, Aggregate &aggregate,
                    version_t<Aggregate> const version) {
    snapshot_source.load_from_snapshot(aggregate, version);
  }
  constexpr void operator()(SnapshotSource &snapshot_source,
                            Aggregate &aggregate,
                            version_t<Aggregate> const version) const
      noexcept(noexcept(snapshot_source.load_from_snapshot(aggregate,
                                                           version))) {
    snapshot_source.load_from_snapshot(aggregate, version);
  }

  template <typename SnapshotSource, concepts::versioned Aggregate>
  requires requires(SnapshotSource &snapshot_source, Aggregate &aggregate,
                    version_t<Aggregate> const version) {
    load_from_snapshot(snapshot_source, aggregate, version);
  }
  constexpr void operator()(SnapshotSource &snapshot_source,
                            Aggregate &aggregate,
                            version_t<Aggregate> const version) const
      noexcept(noexcept(load_from_snapshot(snapshot_source, aggregate,
                                           version))) {
    load_from_snapshot(snapshot_source, aggregate, version);
  }

  template <typename SnapshotSource, concepts::versioned Aggregate>
  constexpr void operator()(SnapshotSource &snapshot_source,
                            Aggregate &aggregate) const
      noexcept(std::is_nothrow_invocable_v<load_from_snapshot_fn,
                                           SnapshotSource &, Aggregate &,
                                           version_t<Aggregate> const>) {
    std::invoke(*this, snapshot_source, aggregate,
                std::numeric_limits<version_t<Aggregate>>::max());
  }
};
} // namespace cpo_details_

inline namespace cpo_fn_ {
inline constexpr cpo_details_::apply_event_fn apply_event = {};
inline constexpr cpo_details_::load_from_history_fn load_from_history = {};
inline constexpr cpo_details_::load_from_snapshot_fn load_from_snapshot = {};
} // namespace cpo_fn_

template <typename T, concepts::domain_event DomainEvent>
struct can_apply_event
    : std::is_invocable<decltype(skizzay::cddd::apply_event),
                        std::add_lvalue_reference_t<T>, DomainEvent> {};

namespace handler_for_details_ {
template <typename, typename> struct impl;

template <typename T, concepts::domain_event DomainEvent>
struct impl<T, DomainEvent> : can_apply_event<T, DomainEvent> {};

template <typename T, concepts::domain_event... DomainEvents>
struct impl<T, domain_event_sequence<DomainEvents...>>
    : std::conjunction<can_apply_event<T, DomainEvents>...> {};
} // namespace handler_for_details_

namespace concepts {
template <typename T>
concept event_source = std::destructible<T>;

template <typename T, typename DomainEvents>
concept handler_for = handler_for_details_::impl<T, DomainEvents>::value;
} // namespace concepts

} // namespace skizzay::cddd
