#pragma once

#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/event_sourced.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/version.h"

namespace skizzay::cddd {
namespace concepts {
template <typename T, typename... DomainEvents>
concept aggregate_root =
    versioned<T> && identifiable<T> &&(0 != sizeof...(DomainEvents)) &&
    (domain_event<DomainEvents> && ...) &&
    std::same_as<std::remove_cvref_t<id_t<T>>,
                 std::remove_cvref_t<id_t<DomainEvents...>>>
        &&std::same_as<version_t<T>, version_t<DomainEvents...>> &&
    (std::invocable<decltype(skizzay::cddd::apply),
                    std::add_lvalue_reference_t<T>,
                    std::remove_reference_t<DomainEvents> const &> &&
     ...);
} // namespace concepts

} // namespace skizzay::cddd
