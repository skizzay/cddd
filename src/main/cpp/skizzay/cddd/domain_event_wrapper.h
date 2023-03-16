#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/domain_event_sequence.h"
#include "skizzay/cddd/domain_event_visitor.h"
#include "skizzay/cddd/event_sourced.h"

namespace skizzay::cddd {

template <concepts::domain_event_sequence DomainEvents> struct event_wrapper {
  using id_type = typename DomainEvents::id_reference_type;
  using version_type = version_t<DomainEvents>;
  using timestamp_type = timestamp_t<DomainEvents>;

  virtual id_type id() const noexcept = 0;
  virtual version_type version() const noexcept = 0;
  virtual timestamp_type timestamp() const noexcept = 0;
  virtual void set_id(id_type) = 0;
  virtual void set_version(version_type const) noexcept = 0;
  virtual void set_timestamp(timestamp_type const) noexcept = 0;
  inline event_visitor<DomainEvents> &
  accept_event_visitor(event_visitor<DomainEvents> &visitor) const {
    this->do_accept_event_visitor(visitor);
    return visitor;
  }

  template <concepts::mutable_domain_event DomainEvent>
  requires(DomainEvents::template contains<DomainEvent>) static inline std::
      unique_ptr<event_wrapper<DomainEvents>> from_domain_event(
          DomainEvent &&domain_event) {
    struct impl final : public event_wrapper {
      explicit inline impl(DomainEvent &&domain_event) noexcept(
          std::is_nothrow_move_constructible_v<DomainEvent>)
          : event{std::move(domain_event)} {}

      typename event_wrapper<DomainEvents>::id_type
      id() const noexcept override {
        using skizzay::cddd::id;
        return id(event);
      }

      typename event_wrapper<DomainEvents>::version_type
      version() const noexcept override {
        using skizzay::cddd::version;
        return version(event);
      }

      typename event_wrapper<DomainEvents>::timestamp_type
      timestamp() const noexcept override {
        using skizzay::cddd::timestamp;
        return timestamp(event);
      }

      void set_id(typename event_wrapper<DomainEvents>::id_type id) override {
        using skizzay::cddd::set_id;
        set_id(event, id);
      }

      void set_version(typename event_wrapper<DomainEvents>::version_type const
                           version) noexcept override {
        using skizzay::cddd::set_version;
        set_version(event, version);
      }

      void
      set_timestamp(typename event_wrapper<DomainEvents>::timestamp_type const
                        timestamp) noexcept override {
        using skizzay::cddd::set_timestamp;
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

    return std::make_unique<impl>(std::move(domain_event));
  }

private:
  virtual void do_accept_event_visitor(event_visitor<DomainEvents> &) const = 0;
};

namespace domain_event_wrapper_details_ {
template <concepts::domain_event_sequence DomainEvents>
struct wrap_domain_events_fn final {
  template <concepts::domain_event_of<DomainEvents> DomainEvent>
  inline std::unique_ptr<event_wrapper<DomainEvents>>
  operator()(DomainEvent &&domain_event) const {
    return event_wrapper<DomainEvents>::from_domain_event(
        std::move(domain_event));
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
