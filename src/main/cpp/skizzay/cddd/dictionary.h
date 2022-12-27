#pragma once

#include <concepts>
#include <ranges>

namespace skizzay::cddd {
template <typename T> using key_t = typename T::key_type;
template <typename T> using mapped_t = typename T::mapped_type;

namespace concepts {
template <typename T>
concept lookup_dictionary = requires() {
  typename key_t<T>;
  typename mapped_t<T>;
  typename std::ranges::iterator_t<T>;
}
&&requires(T const &tc, key_t<T> const &key) {
  { tc.find(key) } -> std::same_as<std::ranges::iterator_t<T const>>;
};

template <typename T>
concept dictionary = lookup_dictionary<T> &&
    requires(T &t, key_t<T> const &key) {
  { t.find(key) } -> std::same_as<std::ranges::iterator_t<T>>;
  { t[key] } -> std::same_as<std::ranges::range_reference_t<T>>;
};
} // namespace concepts

} // namespace skizzay::cddd