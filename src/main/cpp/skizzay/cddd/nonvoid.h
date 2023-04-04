#pragma once

#include <concepts>
#include <type_traits>

namespace skizzay::cddd::concepts {
template <typename T>
concept nonvoid = (!std::is_void_v<T>)&&std::destructible<T>;
} // namespace skizzay::cddd::concepts
