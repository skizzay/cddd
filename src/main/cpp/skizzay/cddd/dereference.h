#pragma once

#include <functional>
#include <iterator>
#include <type_traits>

namespace skizzay::cddd {
namespace dereference_details_ {
struct dereference_fn final {
  template <typename T> constexpr T &operator()(T &t) const noexcept {
    return t;
  }

  template <std::indirectly_readable T>
  constexpr std::add_lvalue_reference_t<
      typename std::indirectly_readable_traits<T>::value_type>
  operator()(T &t) const noexcept {
    return (*this)(*t);
  }

  template <typename T>
  constexpr T &operator()(std::reference_wrapper<T> t) const noexcept {
    return (*this)(t.get());
  }
};
} // namespace dereference_details_

inline namespace dereference_fn_ {
inline constexpr dereference_details_::dereference_fn dereference = {};
}

template <typename T>
using dereference_t = decltype(cddd::dereference(
    std::declval<std::add_lvalue_reference_t<T>>()));
} // namespace skizzay::cddd