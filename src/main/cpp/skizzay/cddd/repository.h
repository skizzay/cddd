#pragma once

#include "skizzay/cddd/boolean.h"
#include "skizzay/cddd/dereference.h"
#include "skizzay/cddd/factory.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/nonvoid.h"

#include <map>
#include <unordered_map>

namespace skizzay::cddd {

namespace repository_details_ {
template <typename> struct is_std_map : std::false_type {};
template <typename K, typename V, typename C, typename A>
struct is_std_map<std::map<K, V, C, A>> : std::true_type {};
template <typename K, typename V, typename H, typename E, typename A>
struct is_std_map<std::unordered_map<K, V, H, E, A>> : std::true_type {};

template <typename T>
concept std_map = is_std_map<std::remove_cvref_t<dereference_t<T>>>::value;

template <std_map M>
using map_key_t = typename std::remove_cvref_t<dereference_t<M>>::key_type;

template <std_map M>
using map_value_t = typename std::remove_cvref_t<dereference_t<M>>::mapped_type;

bool contains(auto const &...) = delete;

struct contains_fn final {
  template <typename T, concepts::identifier Id>
  requires requires(T const &t, Id const &id_value) {
    { dereference(t).contains(id_value) } -> concepts::boolean;
  }
  constexpr concepts::boolean auto operator()(T const &t,
                                              Id const &id_value) const
      noexcept(noexcept(dereference(t).contains(id_value))) {
    return dereference(t).contains(id_value);
  }

  template <typename T, concepts::identifier Id>
  requires requires(T const &t, Id const &id_value) {
    { contains(dereference(t), id_value) } -> concepts::boolean;
  }
  constexpr concepts::boolean auto operator()(T const &t,
                                              Id const &id_value) const
      noexcept(noexcept(contains(dereference(t), id_value))) {
    return contains(dereference(t), id_value);
  }
};

void get(auto...) = delete;

struct get_fn final {
  template <typename T, concepts::identifier Id>
  requires requires(T const &t, Id &&id) {
    { dereference(t).get(std::forward<Id>(id)) } -> concepts::nonvoid;
  }
  [[nodiscard]] constexpr concepts::nonvoid auto operator()(T const &t,
                                                            Id &&id) const
      noexcept(noexcept(dereference(t).get(std::forward<Id>(id)))) {
    return dereference(t).get(std::forward<Id>(id));
  }

  template <typename T, concepts::identifier Id>
  requires requires(T const &t, Id &&id) {
    { get(dereference(t), std::forward<Id>(id)) } -> concepts::nonvoid;
  }
  [[nodiscard]] constexpr concepts::nonvoid auto operator()(T const &t,
                                                            Id &&id) const
      noexcept(noexcept(get(dereference(t), std::forward<Id>(id)))) {
    return get(dereference(t), std::forward<Id>(id));
  }

  template <typename T, concepts::identifier Id, typename OnKeyNotFound>
  requires requires(T const &t, Id &&id, OnKeyNotFound on_key_not_found) {
    {
      dereference(t).get(std::forward<Id>(id), on_key_not_found)
      } -> concepts::nonvoid;
  }
  [[nodiscard]] constexpr concepts::nonvoid auto
  operator()(T const &t, Id &&id, OnKeyNotFound on_key_not_found) const
      noexcept(noexcept(dereference(t).get(std::forward<Id>(id),
                                           on_key_not_found))) {
    return dereference(t).get(std::forward<Id>(id), on_key_not_found);
  }

  template <typename T, concepts::identifier Id, typename OnKeyNotFound>
  requires requires(T const &t, Id &&id, OnKeyNotFound on_key_not_found) {
    {
      get(dereference(t), std::forward<Id>(id), on_key_not_found)
      } -> concepts::nonvoid;
  }
  [[nodiscard]] constexpr concepts::nonvoid auto
  operator()(T const &t, Id &&id, OnKeyNotFound on_key_not_found) const
      noexcept(noexcept(get(dereference(t), std::forward<Id>(id),
                            on_key_not_found))) {
    return get(dereference(t), std::forward<Id>(id), on_key_not_found);
  }

  template <std_map Map,
            typename OnKeyNotFound = nonnull_default_factory<map_key_t<Map>>>
  [[nodiscard]] constexpr map_value_t<Map>
  operator()(Map const &map, map_key_t<Map> const &key,
             OnKeyNotFound &&create = {}) const {
    if (auto const entry = dereference(map).find(key);
        std::end(dereference(map)) == entry) {
      return std::forward<OnKeyNotFound>(create)(key);
    } else {
      return entry->second;
    }
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

  template <std_map Map>
  requires concepts::identifiable<map_value_t<Map>>
  constexpr void operator()(Map &map, map_value_t<Map> entity) const {
    auto id_value = skizzay::cddd::id(entity);
    auto const [entry, created] =
        dereference(map).emplace(std::move(id_value), std::move(entity));
    if (!created) {
      entry->second = std::move(entity);
    }
  }

  template <std_map Map>
  requires concepts::identifier<map_key_t<Map>>
  constexpr void operator()(Map &map, map_key_t<Map> key,
                            map_value_t<Map> value) const {
    auto [entry, created] =
        dereference(map).emplace(std::move(key), std::move(value));
    if (not created) {
      entry->second = std::move(value);
    }
  }
};

void add(auto...) = delete;

struct add_fn final {
  template <typename T, concepts::identifiable Entity>
  requires requires(T &t, Entity &&entity) {
    { dereference(t).add(std::forward<Entity>(entity)) } -> concepts::boolean;
  }
  constexpr concepts::boolean auto operator()(T &t, Entity &&entity) const
      noexcept(noexcept(dereference(t).add(std::forward<Entity>(entity)))) {
    return dereference(t).add(std::forward<Entity>(entity));
  }

  template <typename T, concepts::identifiable Entity>
  requires requires(T &t, Entity &&entity) {
    { add(dereference(t), std::forward<Entity>(entity)) } -> concepts::boolean;
  }
  constexpr concepts::boolean auto operator()(T &t, Entity &&entity) const
      noexcept(noexcept(add(dereference(t), std::forward<Entity>(entity)))) {
    return add(dereference(t), std::forward<Entity>(entity));
  }

  template <std_map Map>
  requires concepts::identifiable<typename Map::value_type>
  constexpr bool operator()(Map &map, typename Map::value_type value) const {
    auto id_value = skizzay::cddd::id(value);
    return dereference(map)
        .emplace(std::move(id_value), std::move(value))
        .second;
  }

  template <std_map Map>
  requires concepts::identifier<map_key_t<Map>>
  constexpr bool operator()(Map &map, map_key_t<Map> key,
                            map_value_t<Map> value) const {
    return dereference(map).emplace(std::move(key), std::move(value)).second;
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

  template <std_map Map>
  requires concepts::identifiable<map_value_t<Map>>
  constexpr bool operator()(Map &map, map_value_t<Map> entity) const {
    if (auto const iter = dereference(map).find(skizzay::cddd::id(entity));
        std::end(dereference(map)) != iter) {
      iter->second = std::move(entity);
      return true;
    } else {
      return false;
    }
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

  template <std_map Map>
  constexpr bool operator()(Map &map, map_key_t<Map> const &key) const {
    if (auto const iter = dereference(map).find(key);
        std::end(dereference(map)) != iter) {
      dereference(map).erase(iter);
      return true;
    } else {
      return false;
    }
  }
};

struct get_or_add_fn final {
  template <typename T, concepts::identifier Id,
            typename Factory = nonnull_default_factory<
                std::remove_cvref_t<std::invoke_result_t<
                    get_fn const, std::add_lvalue_reference_t<T>, Id>>>>
  requires requires(T &t, Id const &id_value, Factory factory) {
    { dereference(t).get_or_add(id_value, factory) } -> concepts::nonvoid;
  }
  [[nodiscard]] constexpr concepts::nonvoid auto
  operator()(T &t, Id const &id_value, Factory factory = {}) const {
    return dereference(t).get_or_add(id_value, std::move(factory));
  }

  template <typename T, concepts::identifier Id,
            typename Factory = nonnull_default_factory<
                std::remove_cvref_t<std::invoke_result_t<
                    get_fn const, std::add_lvalue_reference_t<T>, Id>>>>
  requires requires(T &t, Id const &id_value, Factory factory) {
    { get_or_add(dereference(t), id_value, factory) } -> concepts::nonvoid;
  }
  [[nodiscard]] constexpr concepts::nonvoid auto
  operator()(T &t, Id const &id_value, Factory factory = {}) const {
    return get_or_add(dereference(t), id_value, std::move(factory));
  }

  template <std_map Map,
            typename Factory = nonnull_default_factory<map_value_t<Map>>>
  [[nodiscard]] constexpr map_value_t<Map>
  operator()(Map &map, map_key_t<Map> const &key, Factory &&create = {}) const {
    if (auto const iter = dereference(map).find(key);
        std::end(dereference(map)) != iter) {
      return iter->second;
    } else {
      return map.emplace(key, std::forward<Factory>(create)(key)).first->second;
    }
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
inline constexpr repository_details_::get_or_add_fn get_or_add = {};
} // namespace repository_fn_

namespace concepts {
template <typename T, typename Entity>
concept repository_for_entity = identifiable<Entity> && std::invocable <
                                decltype(skizzay::cddd::get) const &,
        T const &, std::remove_reference_t<id_t<Entity>>
const & >
    &&std::invocable<decltype(skizzay::cddd::put) const &, T &, Entity>
        &&std::invocable<decltype(skizzay::cddd::contains), T &, id_t<Entity>>;
} // namespace concepts

namespace repository_details_ {} // namespace repository_details_

template <typename T, concepts::identifier Id>
using repository_value_t = std::remove_cvref_t<std::invoke_result_t<
    decltype(skizzay::cddd::get), std::add_lvalue_reference_t<T>, Id>>;

template <typename T, concepts::identifier Id>
using repository_null_value_t =
    std::invoke_result_t<decltype(skizzay::cddd::remove), Id>;
} // namespace skizzay::cddd
