#pragma once

#include "skizzay/cddd/domain_event_sequence.h"

namespace skizzay::cddd {

template <concepts::domain_event DomainEvent> struct event_visitor_interface {
  virtual void visit(DomainEvent const &domain_event) = 0;
};

template <typename> struct event_visitor;

template <concepts::domain_event... DomainEvents>
struct event_visitor<domain_event_sequence<DomainEvents...>>
    : virtual event_visitor_interface<std::remove_cvref_t<DomainEvents>>... {};

template <typename> struct event_interface;

template <concepts::domain_event_sequence DomainEvents>
struct event_interface<DomainEvents> {
  using id_type = id_t<DomainEvents>;
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

  template <concepts::domain_event DomainEvent>
  requires(DomainEvents::template contains<DomainEvent>) static inline std::
      unique_ptr<event_interface<DomainEvents>> from_domain_event(
          DomainEvent &&domain_event) {
    struct event_holder final : event_interface {
      typename event_interface<DomainEvents>::id_type
      id() const noexcept override {
        using skizzay::cddd::id;
        return id(event);
      }

      typename event_interface<DomainEvents>::version_type
      version() const noexcept override {
        using skizzay::cddd::version;
        return version(event);
      }

      typename event_interface<DomainEvents>::timestamp_type
      timestamp() const noexcept override {
        using skizzay::cddd::timestamp;
        return timestamp(event);
      }

      void set_id(typename event_interface<DomainEvents>::id_type id) override {
        using skizzay::cddd::set_id;
        set_id(event, id);
      }

      void
      set_version(typename event_interface<DomainEvents>::version_type const
                      version) noexcept override {
        using skizzay::cddd::set_version;
        set_version(event, version);
      }

      void
      set_timestamp(typename event_interface<DomainEvents>::timestamp_type const
                        timestamp) noexcept override {
        using skizzay::cddd::set_timestamp;
        set_timestamp(event, timestamp);
      }

      std::remove_cvref_t<DomainEvent> event;

    private:
      void do_accept_event_visitor(
          event_visitor<DomainEvents> &visitor) const override {
        static_cast<
            event_visitor_interface<std::remove_cvref_t<DomainEvent>> &>(
            visitor)
            .visit(event);
      }
    };
    return std::make_unique<event_holder>(
        std::forward<DomainEvent>(domain_event));
  }

private:
  virtual void do_accept_event_visitor(event_visitor<DomainEvents> &) const = 0;
};
} // namespace skizzay::cddd
