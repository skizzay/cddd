#pragma once

#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/nullable.h"

#include <concepts>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace skizzay::cddd {

namespace concurrent_table_details_ {

template <typename T> struct fn;

template <std::default_initializable T> struct fn<T> final {
  constexpr T operator()() const
      noexcept(std::is_nothrow_default_constructible_v<T>) {
    return T{};
  }
};

template <std::default_initializable T> struct fn<std::optional<T>> final {
  constexpr std::optional<T> operator()() const
      noexcept(std::is_nothrow_default_constructible_v<T>) {
    return std::in_place;
  }
};

template <std::default_initializable T> struct fn<std::shared_ptr<T>> final {
  constexpr std::shared_ptr<T> operator()() const {
    return std::make_shared<T>();
  }
};

template <std::default_initializable T, typename D>
struct fn<std::unique_ptr<T, D>> final {
  constexpr std::unique_ptr<T, D> operator()(D d = {}) const {
    return std::unique_ptr<T, D>{new T{}, std::move(d)};
  }
};

template <typename T> inline constexpr fn<T> default_value = {};

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

  constexpr T
  get_or_add(key_type const &key) requires std::default_initializable<T> {
    if (auto const result = get(key); null_value<T> != result) {
      return result;
    } else {
      return add(key, default_value<T>());
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
  using concurrent_table<T, std::remove_cvref_t<id_t<T>>>::get_or_add;
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
