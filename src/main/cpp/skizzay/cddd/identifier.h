#pragma once

#include "skizzay/cddd/dereference.h"

#include <concepts>
#include <functional>
#include <type_traits>
#include <variant>

namespace skizzay::cddd {

namespace concepts {
template <typename T>
concept identifier = std::regular<std::remove_cvref_t<dereference_t<T>>> &&
    std::is_nothrow_invocable_r_v<
        std::size_t, std::hash<std::remove_cvref_t<dereference_t<T>>>,
        std::add_const_t<dereference_t<T>>> &&
    std::default_initializable<
        std::hash<std::remove_cvref_t<dereference_t<T>>>>;
} // namespace concepts

namespace id_details_ {
template <typename T> void id(T const &...) = delete;

struct id_fn final {
  template <typename T>
  requires requires(T const &t) {
    { (dereference(t).id()) }
    noexcept->concepts::identifier;
  }
  constexpr decltype(auto) operator()(T const &t) const noexcept {
    return dereference(t).id();
  }

  template <typename T>
  requires concepts::identifier<
      decltype(std::remove_cvref_t<dereference_t<T>>::id)>
  constexpr decltype(auto) operator()(T const &t) const noexcept {
    return (dereference(t).id);
  }

  template <typename T>
  requires requires(T const &t) {
    { id(dereference(t)) }
    noexcept->concepts::identifier;
  }
  constexpr decltype(auto) operator()(T const &t) const noexcept {
    return id(dereference(t));
  }

  template <typename... Ts>
  requires(std::invocable<id_fn const, std::add_const_t<Ts>>
               &&...) constexpr decltype(auto)
  operator()(std::variant<Ts...> const &v) const noexcept {
    return std::visit(*this, v);
  }
};

void set_id(auto &...) = delete;

struct set_id_fn final {
  template <typename T, concepts::identifier Id>
  requires requires(T &t, Id &&id_value) {
    {dereference(t).id = std::forward<Id>(id_value)};
  }
  constexpr T &operator()(T &t, Id &&id_value) const
      noexcept(noexcept(dereference(t).id = std::forward<Id>(id_value))) {
    dereference(t).id = std::forward<Id>(id_value);
    return t;
  }

  template <typename T, concepts::identifier Id>
  requires requires(T &t, Id &&id) {
    {dereference(t).set_id(std::forward<Id>(id))};
  }
  constexpr T &operator()(T &t, Id &&id) const
      noexcept(noexcept(dereference(t).set_id(std::forward<Id>(id)))) {
    dereference(t).set_id(std::forward<Id>(id));
    return t;
  }

  template <typename T, concepts::identifier Id>
  requires requires(T &t, Id &&id) {
    {set_id(dereference(t), std::forward<Id>(id))};
  }
  constexpr T &operator()(T &t, Id &&id) const
      noexcept(noexcept(set_id(dereference(t), std::forward<Id>(id)))) {
    set_id(dereference(t), std::forward<Id>(id));
    return t;
  }

  template <typename... Ts, concepts::identifier Id>
  requires(
      std::invocable<set_id_fn const, Ts &, Id> &&...) constexpr decltype(auto)
  operator()(std::variant<Ts...> &t, Id &&id) const
      noexcept((std::is_nothrow_invocable_v<set_id_fn const, Ts &, Id> &&
                ...)) {
    std::visit([this, &id](auto &t) { (*this)(t, std::forward<Id>(id)); }, t);
    return t;
  }
};
} // namespace id_details_

inline namespace id_fn_ {
constexpr inline id_details_::id_fn id = {};
constexpr inline id_details_::set_id_fn set_id = {};
} // namespace id_fn_

namespace concepts {
template <typename T>
concept identifiable = std::invocable < decltype(skizzay::cddd::id),
        std::remove_reference_t<T>
const & >
    &&identifier<std::invoke_result_t<decltype(skizzay::cddd::id),
                                      std::remove_reference_t<T> const &>>;
} // namespace concepts

namespace id_details_ {
template <typename> struct impl;

template <concepts::identifiable Identifiable> struct impl<Identifiable> {
  using type =
      std::invoke_result_t<decltype(skizzay::cddd::id),
                           std::remove_reference_t<Identifiable> const &>;
};

template <typename T>
requires(!concepts::identifiable<T>) && requires { typename T::id_type; }
&&concepts::identifier<typename T::id_type> struct impl<T> {
  using type = typename T::id_type;
};
} // namespace id_details_

template <typename... Ts>
using id_t = std::common_reference_t<typename id_details_::impl<Ts>::type...>;

template <typename... Ts> using id_value_t = std::remove_cvref_t<id_t<Ts...>>;

namespace concepts {
template <typename T, typename Id>
concept identifiable_by = identifiable<T> && identifier<Id> &&
    std::equality_comparable_with<id_t<T>, Id>;
} // namespace concepts

} // namespace skizzay::cddd
