#pragma once

#include "skizzay/cddd/factory.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/key_not_found.h"
#include "skizzay/cddd/nullable.h"
#include "skizzay/cddd/repository.h"

#include <concepts>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace skizzay::cddd {

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

template <typename T, concepts::identifier Id,
          typename Alloc =
              std::allocator<std::pair<std::remove_reference_t<Id> const, T>>>
requires std::copyable<T> && std::same_as<T, std::remove_cvref_t<T>>
struct impl {
  using key_type = std::remove_cvref_t<Id>;
  using value_type = T;
  using allocator_type =
      typename std::unordered_map<key_type, value_type, std::hash<key_type>,
                                  std::equal_to<key_type>,
                                  Alloc>::allocator_type;

  constexpr impl(allocator_type a = {}) : m_{}, entries_{std::move(a)} {}

  constexpr impl(impl const &other) : m_{}, entries_{} {
    std::shared_lock l_{other.m_};
    entries_ = other.entries_;
  }

  constexpr impl(impl &&other) : m_{}, entries_{} {
    std::lock_guard l_{other.m_};
    entries_ = std::move(other.entries_);
  }

  constexpr ~impl() noexcept {
    std::lock_guard l_{m_};
    entries_.clear();
  }

  constexpr impl &operator=(impl const &other) {
    if (this != &other) {
      std::shared_lock l_{other.m_};
      entries_ = other.entries_;
    }
    return *this;
  }

  constexpr impl &operator=(impl &&other) {
    if (this != &other) {
      std::lock_guard l_{other.m_};
      entries_ = std::move(other.entries_);
    }
    return *this;
  }

  template <typename ValueFactory = nonnull_default_factory<T>>
  [[nodiscard]] constexpr T get(key_type const &key,
                                ValueFactory &&create = {}) const
      noexcept(false) {
    std::shared_lock l_{m_};
    return skizzay::cddd::get(entries_, key,
                              std::forward<ValueFactory>(create));
  }

  template <typename Factory = nonnull_default_factory<T>>
  constexpr T get_or_add(key_type const &key, Factory &&create = {}) {
    std::lock_guard l_{m_};
    return skizzay::cddd::get_or_add(entries_, key,
                                     std::forward<Factory>(create));
  }

  [[nodiscard]] constexpr bool contains(key_type const &key) const noexcept {
    std::shared_lock l_{m_};
    return skizzay::cddd::contains(entries_, key);
  }

  constexpr void put(key_type key, T &&t) {
    std::lock_guard l_{m_};
    skizzay::cddd::put(entries_, std::move(key), std::forward<T>(t));
  }

  constexpr bool add(key_type key, T &&t) {
    std::lock_guard l_{m_};
    return skizzay::cddd::add(entries_, std::move(key), std::move(t));
  }

  constexpr bool remove(key_type const &key) {
    std::lock_guard l_{m_};
    return skizzay::cddd::remove(entries_, key);
  }

private:
  mutable std::shared_mutex m_;
  std::unordered_map<key_type, T, std::hash<key_type>, std::equal_to<key_type>,
                     Alloc>
      entries_;
};
} // namespace concurrent_table_details_

template <typename T, concepts::identifier Id,
          typename Alloc =
              std::allocator<std::pair<std::remove_reference_t<Id> const, T>>>
using concurrent_table = concurrent_table_details_::impl<T, Id>;

template <concepts::identifiable T>
struct concurrent_repository
    : private concurrent_table<T, std::remove_cvref_t<id_t<T>>> {
  using concurrent_table<T, std::remove_cvref_t<id_t<T>>>::get;
  using concurrent_table<T, std::remove_cvref_t<id_t<T>>>::get_or_add;
  using concurrent_table<T, std::remove_cvref_t<id_t<T>>>::contains;

  constexpr bool add(T &&t) {
    auto key = id(std::as_const(t));
    return this->concurrent_table<T, std::remove_cvref_t<id_t<T>>>::add(
        std::move(key), std::forward<T>(t));
  }

  constexpr void put(T &&t) {
    auto key = id(std::as_const(t));
    this->concurrent_table<T, std::remove_cvref_t<id_t<T>>>::put(
        std::move(key), std::forward<T>(t));
  }
};
} // namespace skizzay::cddd
