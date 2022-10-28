#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/kafka/kafka_deser.h"
#include "skizzay/cddd/timestamp.h"
#include "skizzay/cddd/version.h"

#include <librdkafka/rdkafkacpp.h>

namespace skizzay::cddd::kafka {
namespace event_store_details_ {
template <concepts::domain_event... DomainEvents> struct event_source {
  using id_type = std::common_reference_t<id_t<DomainEvents>...>;
  using version_type = std::common_type_t<version_t<DomainEvents>...>;
  using timestamp_type = std::common_type_t<timestamp_t<DomainEvents>...>;

  template <concepts::versioned Aggregate>
  requires std::same_as<version_type, version_t<Aggregate>> &&
      (std::invocable<decltype(skizzay::cddd::apply),
                      std::add_lvalue_reference_t<Aggregate>,
                      std::remove_reference_t<DomainEvents> const &>
           &&...) void load_from_history(Aggregate &aggregate,
                                         version_t<decltype(aggregate)> const
                                             target_version) {
    std::unsigned_integral auto const aggregate_version = version(aggregate);
    assert((aggregate_version < target_version) &&
           "Aggregate version cannot exceed target version");
  }
};

template <concepts::domain_event... DomainEvents> struct event_stream {
  using id_type = std::common_reference_t<id_t<DomainEvents>...>;
  using version_type = std::common_type_t<version_t<DomainEvents>...>;
  using timestamp_type = std::common_type_t<timestamp_t<DomainEvents>...>;

  event_stream(std::remove_cvref_t<id_type> const &id) noexcept(
      std::is_nothrow_constructible_v<std::remove_cvref_t<id_type>>)
      : id_{id} {}

  std::remove_cvref_t<id_type> const &id() const noexcept { return id_; }

  template <concepts::domain_event DomainEvent>
  requires(std::same_as<std::remove_cvref_t<DomainEvent>,
                        std::remove_cvref_t<DomainEvents>> ||
           ...) void add_event(DomainEvent &&) {}

  constexpr void commit_events() {}

  constexpr void rollback() noexcept {}

private:
  std::remove_cvref_t<id_type> id_;
};

template <concepts::domain_event... DomainEvents> struct event_store {
  using id_type = std::common_reference_t<id_t<DomainEvents>...>;
  using version_type = std::common_type_t<version_t<DomainEvents>...>;
  using timestamp_type = std::common_type_t<timestamp_t<DomainEvents>...>;

  event_stream<DomainEvents...> get_event_stream(auto const &id) noexcept {
    return {id};
  }

  event_source<DomainEvents...> get_event_source(auto const &) noexcept {
    return {};
  }

  std::unique_ptr<serializer<DomainEvents...>> serializer_;
};
} // namespace event_store_details_

} // namespace skizzay::cddd::kafka
