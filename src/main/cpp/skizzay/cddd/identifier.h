#pragma once

#include <concepts>
#include <functional>
#include <type_traits>
#include <variant>

namespace skizzay::cddd {

namespace concepts {
template <typename T>
concept identifier = std::regular<std::remove_cvref_t<T>> &&
    std::is_nothrow_invocable_r_v<std::size_t,
                                  std::hash<std::remove_cvref_t<T>>,
                                  std::add_lvalue_reference_t<T const>> &&
    std::default_initializable<std::hash<std::remove_cvref_t<T>>>;
} // namespace concepts

namespace id_details_ {
template <typename T> void id(T const &...) = delete;

struct id_fn final {
  template <typename T>
  requires requires(T const &t) {
    { (t.id()) }
    noexcept->concepts::identifier;
  }
  constexpr decltype(auto) operator()(T const &t) const noexcept {
    return t.id();
  }

  template <typename T>
  requires concepts::identifier<decltype(T::id)>
  constexpr decltype(auto) operator()(T const &t) const noexcept {
    return (t.id);
  }

  template <typename T>
  requires requires(T const &t) {
    { id(t) }
    noexcept->concepts::identifier;
  }
  constexpr decltype(auto) operator()(T const &t) const noexcept {
    return id(t);
  }

  template <typename... Ts>
  constexpr decltype(auto)
  operator()(std::variant<Ts...> const &v) const noexcept {
    return std::visit(*this, v);
  }

  template <std::indirectly_readable Pointer>
  constexpr decltype(auto) operator()(Pointer const &pointer) const noexcept {
    return std::invoke(*this, *pointer);
  }
};

void set_id(auto &...) = delete;

struct set_id_fn final {
  template <typename T, typename Id>
  requires concepts::identifier<decltype(T::id)> &&
      std::assignable_from < std::remove_reference_t<decltype(T::id)>
  &, Id > constexpr T &operator()(T &t, Id &&id) const
         noexcept(noexcept(t.id = std::forward<Id>(id))) {
    t.id = std::forward<Id>(id);
    return t;
  }

  template <typename T, typename Id>
  requires requires(T &t, Id &&id) { {t.set_id(std::forward<Id>(id))}; }
  constexpr T &operator()(T &t, Id &&id) const
      noexcept(noexcept(t.set_id(std::forward<Id>(id)))) {
    t.set_id(std::forward<Id>(id));
    return t;
  }

  template <typename T, typename Id>
  requires requires(T &t, Id &&id) { {set_id(t, std::forward<Id>(id))}; }
  constexpr T &operator()(T &t, Id &&id) const
      noexcept(noexcept(set_id(t, std::forward<Id>(id)))) {
    set_id(t, std::forward<Id>(id));
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

} // namespace skizzay::cddd
