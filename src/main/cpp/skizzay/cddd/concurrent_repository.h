#pragma once

#include "skizzay/cddd/factory.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/nullable.h"

#include <concepts>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace skizzay::cddd {

namespace concurrent_table_details_ {

template <typename T, concepts::identifier Id> struct impl {
  using key_type = std::remove_cvref_t<Id>;

  constexpr nullable_t<T> get(key_type const &key) const
      noexcept(std::is_nothrow_copy_constructible_v<T>) {
    std::shared_lock l_{m_};
    if (auto const entry = unguarded_find(key); std::end(entries_) != entry) {
      return entry->second;
    } else {
      return null_value<T>;
    }
  }

  template <typename Factory = nonnull_default_factory<T>>
  requires concepts::factory<Factory, T, key_type const &> ||
      concepts::factory<Factory, T>
  constexpr T
  get_or_put(key_type const &key,
             Factory create = {}) requires std::default_initializable<T> {
    if (auto const result = get(key); null_value<T> != result) {
      return result;
    } else if constexpr (concepts::factory<Factory, T, key_type const &>) {
      return add(key, create(key));
    } else {
      return add(key, create());
    }
  }

  constexpr bool contains(key_type const &key) const noexcept {
    std::shared_lock l_{m_};
    return unguarded_find(key) == std::end(entries_);
  }

  constexpr T put(key_type key, T &&t) {
    std::lock_guard l_{m_};
    return unguarded_put(std::move(key), std::forward<T>(t));
  }

  constexpr T add(key_type key, T &&t) {
    std::lock_guard l_{m_};
    if (auto entry = unguarded_find(key); std::end(entries_) != entry) {
      return entry->second;
    } else {
      return unguarded_put(std::move(key), std::forward<T>(t));
    }
  }

private:
  constexpr typename std::unordered_map<key_type, T>::const_iterator
  unguarded_find(key_type const &key) const noexcept {
    return entries_.find(key);
  }

  constexpr T unguarded_put(key_type key, T &&t) {
    return entries_.emplace(std::move(key), std::forward<T>(t)).first->second;
  }

  mutable std::shared_mutex m_;
  std::unordered_map<key_type, T> entries_;
};
} // namespace concurrent_table_details_

template <typename T, concepts::identifier Id>
using concurrent_table = concurrent_table_details_::impl<T, Id>;

template <concepts::identifiable T>
struct concurrent_repository
    : private concurrent_table<T, std::remove_cvref_t<id_t<T>>> {
  using concurrent_table<T, std::remove_cvref_t<id_t<T>>>::get;
  using concurrent_table<T, std::remove_cvref_t<id_t<T>>>::get_or_put;
  using concurrent_table<T, std::remove_cvref_t<id_t<T>>>::contains;

  constexpr T add(T &&t) {
    auto key = id(t);
    this->add(std::forward<T>(t), std::move(key));
  }

  constexpr T put(T &&t) {
    auto key = id(t);
    this->put(std::forward<T>(t), std::move(key));
  }
};
} // namespace skizzay::cddd
