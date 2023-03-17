#pragma once

#include "skizzay/cddd/factory.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/nullable.h"

#include <concepts>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace skizzay::cddd {

template <concepts::identifier Key>
struct key_not_found : std::invalid_argument {
  key_not_found(Key key_value)
      : std::invalid_argument{"Key not found"}, key_{std::move(key_value)} {}

  constexpr Key const &key() const noexcept { return key_; }

  Key key_;
};

namespace concurrent_table_details_ {

struct throw_key_not_found_fn final {
  template <template <typename> typename Template, typename T>
  constexpr T operator()(Template<T> const,
                         concepts::identifier auto key_value) noexcept(false) {
    throw key_not_found{std::move(key_value)};
  }
};

struct provide_null_value_fn final {
  template <template <typename> typename Template, typename T>
  constexpr nullable_t<T>
  operator()(Template<T> const,
             concepts::identifier auto const &) const noexcept {
    return null_value<T>;
  }
};

struct provide_nonnull_value_fn final {
  template <template <typename> typename Template, typename T,
            concepts::identifier Id>
  constexpr std::invoke_result_t<nonnull_default_factory<T>, Id const &>
  operator()(Template<T> const, Id const &id_value) const {
    return nonnull_default_factory<T>{}(id_value);
  }

  template <template <typename> typename Template, typename T,
            concepts::identifier Id>
  constexpr std::invoke_result_t<nonnull_default_factory<T>>
  operator()(Template<T> const, Id const &) const {
    return nonnull_default_factory<T>{}();
  }
};
} // namespace concurrent_table_details_

inline namespace concurrent_table_fn_ {
inline constexpr concurrent_table_details_::throw_key_not_found_fn
    throw_key_not_found = {};
inline constexpr concurrent_table_details_::provide_null_value_fn
    provide_null_value = {};
inline constexpr auto provide_default_value = []<typename T>(T &default_value) {
  return [&default_value]<template <typename> typename Template, typename U>
  requires std::convertible_to<T &, U &>(Template<U> const,
                                         concepts::identifier auto const &)
  noexcept { return default_value; };
};
inline constexpr concurrent_table_details_::provide_nonnull_value_fn
    provide_nonnull_default_value = {};
} // namespace concurrent_table_fn_

namespace concurrent_table_details_ {

template <typename T> struct identity final { using type = T; };

template <typename, typename, typename> struct get_result;

template <concepts::nullable T, concepts::identifier Id, typename Factory>
requires concepts::nullable < std::invoke_result_t < Factory, identity<T>,
    std::remove_reference_t<Id>
const & >> &&std::convertible_to<
               std::invoke_result_t<Factory, identity<T>,
                                    std::remove_reference_t<Id> const &>,
               T> struct get_result<T, Id, Factory> {
  using type = T;
};

template <typename T, concepts::identifier Id, typename Factory>
requires std::is_reference_v < std::invoke_result_t < Factory, identity<T>,
    std::remove_reference_t<Id>
const & >> &&std::convertible_to<
               std::invoke_result_t<Factory, identity<T>,
                                    std::remove_reference_t<Id> const &>,
               T &> struct get_result<T, Id, Factory> {
  using type = T &;
};

template <typename T, concepts::identifier Id, typename Factory>
using get_result_t = typename get_result<T, Id, Factory>::type;

template <typename T, concepts::identifier Id, typename Factory>
struct get_or_put_result {
  using type = std::invoke_result_t<Factory, identity<T>,
                                    std::remove_reference_t<Id> const &>;
};

template <typename T, concepts::identifier Id, typename Factory>
using get_or_put_result_t = std::add_lvalue_reference_t<
    typename get_or_put_result<T, Id, Factory>::type>;

template <typename T, concepts::identifier Id>
requires(!std::is_reference_v<T>) struct impl {
  using key_type = std::remove_cvref_t<Id>;
  using value_type = T;

  template <typename OnKeyNotFound =
                concurrent_table_details_::throw_key_not_found_fn>
  [[nodiscard]] constexpr get_result_t<T, Id, OnKeyNotFound>
  get(key_type const &key, OnKeyNotFound on_key_not_found = {}) const
      noexcept(std::is_nothrow_copy_constructible_v<T>
                   &&std::is_nothrow_invocable_v<OnKeyNotFound, key_type>) {
    std::shared_lock l_{m_};
    if (auto const entry = entries_.find(key); std::end(entries_) != entry) {
      return entry->second;
    } else {
      l_.unlock();
      return on_key_not_found(identity<T>{}, key);
    }
  }

  template <typename OnKeyNotFound =
                concurrent_table_details_::throw_key_not_found_fn>
  [[nodiscard]] constexpr get_result_t<T, Id, OnKeyNotFound>
  get(key_type const &key, OnKeyNotFound on_key_not_found = {}) noexcept(
      std::is_nothrow_copy_constructible_v<T>
          &&std::is_nothrow_invocable_v<OnKeyNotFound, key_type>) {
    std::shared_lock l_{m_};
    if (auto const entry = entries_.find(key); std::end(entries_) != entry) {
      return entry->second;
    } else {
      l_.unlock();
      return on_key_not_found(identity<T>{}, key);
    }
  }

  template <
      typename Factory = concurrent_table_details_::provide_nonnull_value_fn>
  [[nodiscard]] constexpr get_or_put_result_t<T, Id, Factory>
  get_or_put(key_type const &key, Factory create = {}) {
    std::lock_guard l_{m_};
    if (auto const entry = entries_.find(key); std::end(entries_) != entry) {
      return entry->second;
    } else {
      return unguarded_put(key, create(identity<T>{}, key));
    }
  }

  [[nodiscard]] constexpr bool contains(key_type const &key) const noexcept {
    std::shared_lock l_{m_};
    return entries_.find(key) != std::end(entries_);
  }

  constexpr T &put(key_type key, T &&t) {
    std::lock_guard l_{m_};
    return unguarded_put(std::move(key), std::forward<T>(t));
  }

  constexpr T &add(key_type key, T &&t) {
    std::lock_guard l_{m_};
    return entries_.emplace(std::move(key), std::forward<T>(t)).first->second;
  }

private:
  template <typename U> constexpr T &unguarded_put(key_type key, U &&u) {
    auto [entry, created] =
        entries_.emplace(std::move(key), std::forward<U>(u));
    if (not created) {
      entry->second = std::forward<U>(u);
    }
    return entry->second;
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

  constexpr T &add(T &&t) {
    auto key = id(std::as_const(t));
    return this->concurrent_table<T, std::remove_cvref_t<id_t<T>>>::add(
        std::move(key), std::forward<T>(t));
  }

  constexpr T &put(T &&t) {
    auto key = id(std::as_const(t));
    return this->concurrent_table<T, std::remove_cvref_t<id_t<T>>>::put(
        std::move(key), std::forward<T>(t));
  }
};
} // namespace skizzay::cddd
