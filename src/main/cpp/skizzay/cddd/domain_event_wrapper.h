#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/domain_event_visitor.h"
#include "skizzay/cddd/event_sourced.h"

#include <memory>

namespace skizzay::cddd {

namespace event_wrapper_details_ {
template <typename Derived, concepts::domain_event DomainEvent>
struct apply_event_visitor_impl : virtual event_visitor_interface<DomainEvent> {
  void visit(DomainEvent const &domain_event) override {
    cddd::apply_event(static_cast<Derived *>(this)->get_handler(),
                      domain_event);
  }
};

template <typename DomainEvents, typename Handler> struct apply_event_visitor;

template <concepts::domain_event... DomainEvents,
          concepts::handler_for<domain_event_sequence<DomainEvents...>> Handler>
struct apply_event_visitor<domain_event_sequence<DomainEvents...>, Handler>
    final
    : virtual event_visitor<domain_event_sequence<DomainEvents...>>,
      apply_event_visitor_impl<
          apply_event_visitor<domain_event_sequence<DomainEvents...>, Handler>,
          DomainEvents>... {
  explicit constexpr apply_event_visitor(Handler &handler) noexcept
      : handler_{handler} {}

  auto &get_handler() noexcept { return handler_; }

private:
  Handler &handler_;
};
} // namespace event_wrapper_details_

template <concepts::domain_event_sequence DomainEvents> struct event_wrapper {
  virtual ~event_wrapper() = default;

  using id_type = typename DomainEvents::id_reference_type;
  using version_type = version_t<DomainEvents>;
  using timestamp_type = timestamp_t<DomainEvents>;

  virtual id_type id() const noexcept = 0;
  virtual version_type version() const noexcept = 0;
  virtual timestamp_type timestamp() const noexcept = 0;
  virtual void set_id(id_type) = 0;
  virtual void set_version(version_type ) noexcept = 0;
  virtual void set_timestamp(timestamp_type ) noexcept = 0;

  event_visitor<DomainEvents> &&
  accept_event_visitor(event_visitor<DomainEvents> &&visitor) const {
    this->do_accept_event_visitor(
        static_cast<event_visitor<DomainEvents> &>(visitor));
    return std::move(visitor);
  }

  event_visitor<DomainEvents> &
  accept_event_visitor(event_visitor<DomainEvents> &visitor) const {
    this->do_accept_event_visitor(visitor);
    return visitor;
  }

  template <typename Handler>
  friend constexpr Handler &
  apply_event(Handler &handler, event_wrapper const &self) {
    event_wrapper_details_::apply_event_visitor<DomainEvents, Handler> visitor{
        handler};
    self.do_accept_event_visitor(visitor);
    return handler;
  }

  template <concepts::mutable_domain_event DomainEvent>
  requires(DomainEvents::template contains<DomainEvent>)
  static std::unique_ptr<event_wrapper> from_domain_event(
          DomainEvent &&domain_event) {
    struct impl final : event_wrapper {
      explicit impl(DomainEvent &&domain_event) noexcept(
          std::is_nothrow_move_constructible_v<DomainEvent>)
          : event{std::move(domain_event)} {}

      id_type
      id() const noexcept override {
        using cddd::id;
        return id(event);
      }

      version_type
      version() const noexcept override {
        using cddd::version;
        return version(event);
      }

      timestamp_type
      timestamp() const noexcept override {
        using cddd::timestamp;
        return timestamp(event);
      }

      void set_id(id_type id) override {
        using cddd::set_id;
        set_id(event, id);
      }

      void set_version(version_type const
                           version) noexcept override {
        using cddd::set_version;
        set_version(event, version);
      }

      void
      set_timestamp(timestamp_type const
                        timestamp) noexcept override {
        using cddd::set_timestamp;
        set_timestamp(event, timestamp);
      }

    private:
      void do_accept_event_visitor(
          event_visitor<DomainEvents> &visitor) const override {
        static_cast<
            event_visitor_interface<std::remove_cvref_t<DomainEvent>> &>(
            visitor)
            .visit(event);
      }

      std::remove_cvref_t<DomainEvent> event;
    };

    return std::make_unique<impl>(std::forward<DomainEvent>(domain_event));
  }

private:
  virtual void do_accept_event_visitor(event_visitor<DomainEvents> &) const = 0;
};

namespace domain_event_wrapper_details_ {
template <concepts::domain_event_sequence DomainEvents>
struct wrap_domain_events_fn final {
  template <concepts::domain_event_of<DomainEvents> DomainEvent>
  std::unique_ptr<event_wrapper<DomainEvents>>
  operator()(DomainEvent &&domain_event) const {
    return event_wrapper<DomainEvents>::from_domain_event(
        std::forward<DomainEvent>(domain_event));
  }
};
} // namespace domain_event_wrapper_details_

inline namespace domain_event_wrapper_fn_ {
template <concepts::domain_event_sequence DomainEvents>
inline constexpr domain_event_wrapper_details_::wrap_domain_events_fn<
    DomainEvents>
    wrap_domain_events = {};
}

} // namespace skizzay::cddd
