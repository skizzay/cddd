#pragma once

#include "skizzay/cddd/boolean.h"
#include "skizzay/cddd/dereference.h"
#include "skizzay/cddd/factory.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/nullable.h"

namespace skizzay::cddd {

namespace repository_details_ {
bool contains(auto const &...) = delete;

struct contains_fn final {
  template <typename T, concepts::identifier Id>
  requires requires(T const &t, Id const &id_value) {
    { dereference(t).contains(dereference(id_value)) } -> concepts::boolean;
  }
  constexpr concepts::boolean auto operator()(T const &t,
                                              Id const &id_value) const
      noexcept(noexcept(dereference(t).contains(dereference(id_value)))) {
    return dereference(t).contains(dereference(id_value));
  }

  template <typename T, concepts::identifier Id>
  requires requires(T const &t, Id const &id_value) {
    { contains(dereference(t), dereference(id_value)) } -> concepts::boolean;
  }
  constexpr concepts::boolean auto operator()(T const &t,
                                              Id const &id_value) const
      noexcept(noexcept(contains(dereference(t), dereference(id_value)))) {
    return contains(dereference(t), dereference(id_value));
  }
};

void put(auto...) = delete;

struct put_fn final {
  template <typename T, concepts::identifiable Entity>
  requires requires(T &t, Entity &&entity) {
    {dereference(t).put(std::forward<Entity>(entity))};
  }
  constexpr decltype(auto) operator()(T &t, Entity &&entity) const
      noexcept(noexcept(dereference(t).put(std::forward<Entity>(entity)))) {
    dereference(t).put(std::forward<Entity>(entity));
  }

  template <typename T, concepts::identifiable Entity>
  requires requires(T &t, Entity &&entity) {
    {put(dereference(t), std::forward<Entity>(entity))};
  }
  constexpr void operator()(T &t, Entity &&entity) const
      noexcept(noexcept(put(dereference(t), std::forward<Entity>(entity)))) {
    put(dereference(t), std::forward<Entity>(entity));
  }
};

void add(auto...) = delete;

struct add_fn final {
  template <typename T, concepts::identifiable Entity>
  requires requires(T &t, Entity &&entity) {
    {dereference(t).add(std::forward<Entity>(entity))};
  }
  constexpr decltype(auto) operator()(T &t, Entity &&entity) const
      noexcept(noexcept(dereference(t).add(std::forward<Entity>(entity)))) {
    return dereference(t).add(std::forward<Entity>(entity));
  }

  template <typename T, concepts::identifiable Entity>
  requires requires(T &t, Entity &&entity) {
    {add(dereference(t), std::forward<Entity>(entity))};
  }
  constexpr decltype(auto) operator()(T &t, Entity &&entity) const
      noexcept(noexcept(add(dereference(t), std::forward<Entity>(entity)))) {
    return add(dereference(t), std::forward<Entity>(entity));
  }
};

void update(auto...) = delete;

struct update_fn final {
  template <typename T, concepts::identifiable Entity,
            typename OnEntityNotFound>
  requires requires(T &t, Entity entity, OnEntityNotFound on_entity_not_found) {
    {dereference(t).update(std::move(entity), std::move(on_entity_not_found))};
  }
  constexpr decltype(auto)
  operator()(T &t, Entity entity,
             OnEntityNotFound on_entity_not_found = {}) const {
    return dereference(t).update(std::move(entity),
                                 std::move(on_entity_not_found));
  }

  template <typename T, concepts::identifiable Entity,
            std::invocable<id_t<Entity>> OnEntityNotFound>
  requires requires(T &t, Entity entity,
                    OnEntityNotFound on_entity_not_found = {}) {
    {on_entity_not_found(dereference(t), std::move(entity),
                         std::move(on_entity_not_found))};
  }
  constexpr decltype(auto)
  operator()(T &t, Entity entity,
             OnEntityNotFound on_entity_not_found = {}) const {
    return on_entity_not_found(dereference(t), std::move(entity),
                               std::move(on_entity_not_found));
  }
};

void get(auto...) = delete;

template <typename T>
concept nonvoid = (not std::is_void_v<T>);

struct get_fn final {
  template <typename T, concepts::identifier Id>
  requires requires(T &t, Id id) {
    { dereference(t).get(id) } -> nonvoid;
  }
  [[nodiscard]] constexpr nonvoid auto operator()(T &t, Id &&id) const
      noexcept(noexcept(dereference(t).get(std::forward<Id>(id)))) {
    return dereference(t).get(std::forward<Id>(id));
  }

  template <typename T, concepts::identifier Id>
  requires requires(T &t, Id &&id) {
    { get(dereference(t), std::forward<Id>(id)) } -> nonvoid;
  }
  [[nodiscard]] constexpr nonvoid auto operator()(T &t, Id &&id) const
      noexcept(noexcept(get(dereference(t), std::forward<Id>(id)))) {
    return get(dereference(t), std::forward<Id>(id));
  }
};

void remove(auto...) = delete;

struct remove_fn final {
  template <typename T, concepts::identifier Id>
  requires requires(T &t, Id id) { {dereference(t).remove(id)}; }
  constexpr decltype(auto) operator()(T &t, Id &&id) const
      noexcept(noexcept(dereference(t).remove(std::forward<Id>(id)))) {
    return dereference(t).remove(std::forward<Id>(id));
  }

  template <typename T, concepts::identifier Id>
  requires requires(T &t, Id &&id) {
    {remove(dereference(t), std::forward<Id>(id))};
  }
  constexpr decltype(auto) operator()(T &t, Id &&id) const
      noexcept(noexcept(remove(dereference(t), std::forward<Id>(id)))) {
    return remove(dereference(t), std::forward<Id>(id));
  }
};
} // namespace repository_details_

inline namespace repository_fn_ {
inline constexpr repository_details_::put_fn put = {};
inline constexpr repository_details_::add_fn add = {};
inline constexpr repository_details_::update_fn update = {};
inline constexpr repository_details_::get_fn get = {};
inline constexpr repository_details_::contains_fn contains = {};
inline constexpr repository_details_::remove_fn remove = {};
} // namespace repository_fn_

namespace concepts {
template <typename T, typename Entity>
concept repository_for_entity = identifiable<Entity> && std::invocable <
                                decltype(skizzay::cddd::get) const &,
        T const &, std::remove_reference_t<id_t<Entity>>
const & > && std::invocable<decltype(skizzay::cddd::put) const &, T &, Entity> &&
std::invocable<decltype(skizzay::cddd::contains), T &, id_t<Entity>>;
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

template <typename T, concepts::identifier Id>
using repository_value_t = std::remove_cvref_t<std::invoke_result_t<
    decltype(skizzay::cddd::get), std::add_lvalue_reference_t<T>, Id>>;

template <typename T, concepts::identifier Id>
using repository_null_value_t =
    std::invoke_result_t<decltype(skizzay::cddd::remove), Id>;

inline namespace repository_fn_ {
inline constexpr repository_details_::get_or_put_fn get_or_put{};
}
} // namespace skizzay::cddd
