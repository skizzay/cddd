#pragma once

#include "skizzay/cddd/factory.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/nullable.h"

namespace skizzay::cddd {

namespace repository_details_ {

struct put_fn final {
  template <typename T, concepts::identifiable Entity>
  requires requires(T &t, Entity &&entity) {
    { t.put(std::forward<Entity>(entity)); };
  }
  constexpr void operator()(T &t, Entity &&entity) const
      noexcept(noexcept(t.put(std::forward<Entity>(entity)))) {
    t.put(std::forward<Entity>(entity));
  }

  template <typename T, concepts::identifiable Entity>
  requires requires(T &t, Entity &&entity) {
    { put(t, std::forward<Entity>(entity)); };
  }
  constexpr void operator()(T &t, Entity &&entity) const
      noexcept(noexcept(put(t, std::forward<Entity>(entity)))) {
    put(t, std::forward<Entity>(entity));
  }

  template <std::indirectly_readable T, concepts::identifiable Entity>
  constexpr void operator()(T &pointer, Entity &&entity) const
      noexcept(std::is_nothrow_invocable_v<
               put_fn const &,
               typename std::indirectly_readable_traits<T>::value_type &,
               Entity &&>) {
    std::invoke(*this, *pointer, std::forward<Entity>(entity));
  }
};

struct get_fn final {
  template <typename T, concepts::identifier Id>
  requires requires(T &t, Id &&id) {
    { t.get(std::forward<Id>(id)) } -> concepts::nullable;
  }
  constexpr concepts::nullable operator()(T &t, Id &&id) const
      noexcept(noexcept(t.get(std::forward<Id>(id)))) {
    return t.get(std::forward<Id>(id));
  }

  template <typename T, concepts::identifier Id>
  requires requires(T &t, Id &&id) {
    { get(t, std::forward<Id>(id)) } -> concepts::nullable;
  }
  constexpr concepts::nullable operator()(T &t, Id &&id) const
      noexcept(noexcept(get(t, std::forward<Id>(id)))) {
    return get(t, std::forward<Id>(id));
  }

  template <std::indirectly_readable T, concepts::identifier Id>
  constexpr concepts::nullable operator()(T &t, Id &&id) const noexcept(
      std::is_nothrow_invocable_v<
          get_fn const &,
          typename std::indirectly_readable_traits<T>::value_type &, Id &&>) {
    std::invoke(*this, *t, std::forward<Id>(id));
  }
};
} // namespace repository_details_

inline namespace repository_fn_ {
inline constexpr repository_details_::put_fn put = {};
inline constexpr repository_details_::get_fn get = {};
} // namespace repository_fn_

namespace concepts {
template <typename T, typename Entity>
concept repository = identifiable<Entity> &&
    std::invocable<decltype(skizzay::cddd::put) const &, T &, Entity> &&
    std::invocable<decltype(skizzay::cddd::get) const &, T const &,
                   std::remove_reference_t<id_t<Entity>>
const & > ;
} // namespace concepts
} // namespace skizzay::cddd
