#pragma once

#include <concepts>
#include <type_traits>

namespace skizzay::cddd {
template <typename> struct is_boolean : std::false_type {};

namespace boolean_details_ {
template <typename T>
concept boolean_testable = std::convertible_to<T, bool>;
}

template <typename T>
requires boolean_details_::boolean_testable<T> && requires(T &&t) {
  { !static_cast<T &&>(t) } -> boolean_details_::boolean_testable;
}
struct is_boolean<T> : std::true_type {};

namespace concepts {
template <typename T>
concept boolean = is_boolean<T>::value;
} // namespace concepts
} // namespace skizzay::cddd