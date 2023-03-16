#pragma once

#include "skizzay/cddd/dereference.h"
#include "skizzay/cddd/factory.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/nullable.h"

namespace skizzay::cddd {

namespace repository_details_ {

struct put_fn final {
  template <typename T, concepts::identifiable Entity>
  requires requires(T &t, Entity &&entity) {
    {t.put(std::forward<Entity>(entity))};
  }
  constexpr void operator()(T &t, Entity &&entity) const
      noexcept(noexcept(t.put(std::forward<Entity>(entity)))) {
    t.put(std::forward<Entity>(entity));
  }

  template <typename T, concepts::identifiable Entity>
  requires requires(T &t, Entity &&entity) {
    {put(t, std::forward<Entity>(entity))};
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
  requires requires(T &t, Id id) {
    { dereference(t).get(id) } -> concepts::identifiable_by<Id>;
  }
  constexpr concepts::identifiable_by<Id> auto operator()(T &t, Id &&id) const
      noexcept(noexcept(t.get(std::forward<Id>(id)))) {
    return dereference(t).get(std::forward<Id>(id));
  }

  template <typename T, concepts::identifier Id>
  requires requires(T &t, Id &&id) {
    {
      get(dereference(t), std::forward<Id>(id))
      } -> concepts::identifiable_by<Id>;
  }
  constexpr concepts::identifiable_by<Id> auto operator()(T &t, Id &&id) const
      noexcept(noexcept(get(t, std::forward<Id>(id)))) {
    return get(dereference(t), std::forward<Id>(id));
  }
};
} // namespace repository_details_

inline namespace repository_fn_ {
inline constexpr repository_details_::put_fn put = {};
inline constexpr repository_details_::get_fn get = {};
} // namespace repository_fn_

namespace concepts {
template <typename T, typename Entity>
concept repository =
    identifiable<Entity> &&
    std::invocable<decltype(skizzay::cddd::put) const &, T &, Entity> &&
    std::invocable < decltype(skizzay::cddd::get) const &,
        T const &, std::remove_reference_t<id_t<Entity>>
const & > ;
} // namespace concepts

namespace repository_details_ {
struct get_or_put_fn final {
  template <typename T, concepts::identifier Id,
            typename Factory = nonnull_default_factory<T>>
  requires requires(T &t, Id const &id_value, Factory factory) {
    { t.get_or_put(id_value, factory) } -> concepts::identifiable_by<Id>;
  }
  constexpr concepts::identifiable_by<Id> auto
  operator()(T &t, Id const &id_value, Factory factory = {}) const {
    return t.get_or_put(id, std::move(factory));
  }
};
} // namespace repository_details_

inline namespace repository_fn_ {
inline constexpr repository_details_::get_or_put_fn get_or_put{};
}
} // namespace skizzay::cddd
