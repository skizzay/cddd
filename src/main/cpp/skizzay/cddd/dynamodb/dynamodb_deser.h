#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/dynamodb/dynamodb_attribute_value.h"
#include <aws/dynamodb/model/AttributeValue.h>
#include <aws/dynamodb/model/Put.h>
#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace skizzay::cddd::dynamodb {

namespace deser_details_ {
template <concepts::domain_event DomainEvent> struct serializer_interface {
  virtual Aws::DynamoDB::Model::Put serialize(DomainEvent &&) const = 0;
  virtual std::string_view
  message_type(event_type<DomainEvent> const) const noexcept = 0;
};

template <concepts::domain_event DomainEvent> struct deserializer_interface {
  virtual void deserialize(
      Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> const &,
      event_visitor_interface<DomainEvent> &visitor) = 0;
};
} // namespace deser_details_

template <concepts::domain_event... DomainEvents>
struct serializer
    : virtual deser_details_::serializer_interface<DomainEvents>... {};

template <concepts::domain_event... DomainEvents>
struct deserializer
    : virtual deser_details_::deserializer_interface<DomainEvents>... {};

inline auto const set_item_value = [](auto &item, std::string_view key,
                                      auto &&value) {
  auto [iterator, is_new_entry] =
      item.emplace(key, attribute_value(std::forward<decltype(value)>(value)));
  if (not is_new_entry) {
    iterator->second = attribute_value(std::forward<decltype(value)>(value));
  }
};

} // namespace skizzay::cddd::dynamodb
