#pragma once

#include "skizzay/cddd/domain_event.h"
#include <cstddef>
#include <span>
#include <type_traits>
#include <vector>

namespace skizzay::cddd::kafka {
namespace derser_details_ {
template <concepts::domain_event DomainEvent> struct serializer_interface {
  virtual std::pmr::vector<std::byte> serialize(DomainEvent const &) const = 0;
};
} // namespace derser_details_

template <concepts::domain_event... DomainEvents>
struct serializer : virtual derser_details_::serializer_interface<
                        std::remove_cvref_t<DomainEvents>>... {};

} // namespace skizzay::cddd::kafka
