#include <skizzay/cddd/concurrent_repository.h>

#include "skizzay/cddd/repository.h"
#include <catch.hpp>
#include <type_traits>

namespace {
struct fake_value_type {
  int id = {};
  int other_value = {};
};
} // namespace

TEST_CASE("concurrent table can use value objects", "[unit][repository]") {
  using skizzay::cddd::get;
  using skizzay::cddd::put;
  using target_type = skizzay::cddd::concurrent_repository<fake_value_type>;

  target_type target;

  SECTION("get member function invokes handler when item is not found") {
    int const key = 15;
    bool invoked = false;
    fake_value_type result;

    [[maybe_unused]] auto &actual = target.get(
        key,
        [&invoked, &result]<template <typename> typename Template, typename T>(
            [[maybe_unused]] Template<T> return_type,
            int) noexcept -> fake_value_type & {
          invoked = std::is_same_v<T, fake_value_type>;
          return result;
        });

    REQUIRE(std::is_same_v<decltype(actual), fake_value_type &>);
    REQUIRE(invoked);
  }

  SECTION("put member function will add a new entry") {
    int const key = 42;
    auto &actual = target.put(fake_value_type{key});
    REQUIRE(std::is_same_v<decltype(actual), fake_value_type &>);
    REQUIRE(target.contains(key));
  }

  SECTION("add member function will add a new entry") {
    int const key = 42;
    auto &actual = target.add(fake_value_type{key});
    REQUIRE(std::is_same_v<decltype(actual), fake_value_type &>);
    REQUIRE(target.contains(key));
  }

  SECTION("put member function will overwrite an existing entry") {
    int const key = 42;
    [[maybe_unused]] auto const first =
        target.put(fake_value_type{key, 1}).other_value;
    auto const second = target.put(fake_value_type{key, 2}).other_value;
    REQUIRE(2 == second);
  }

  SECTION("add member function will not overwrite an existing entry") {
    int const key = 42;
    [[maybe_unused]] auto const first =
        target.add(fake_value_type{key, 1}).other_value;
    auto const second = target.add(fake_value_type{key, 2}).other_value;
    REQUIRE(1 == second);
  }

  SECTION("get member function does not invoke handler when item is found") {
    int const key = 15;
    bool invoked = false;
    auto &result = target.put(fake_value_type{key});

    [[maybe_unused]] auto &actual = target.get(
        key,
        [&invoked, &result]<template <typename> typename Template, typename T>(
            [[maybe_unused]] Template<T> return_type,
            int) noexcept -> fake_value_type & {
          invoked = std::is_same_v<T, fake_value_type>;
          return result;
        });

    REQUIRE(std::is_same_v<decltype(actual), decltype(result)>);
    REQUIRE_FALSE(invoked);
  }

  SECTION("get_or_put member function invokes handler when item is not found") {
    int const key = 15;
    bool invoked = false;

    [[maybe_unused]] auto &actual = target.get_or_put(
        key,
        [&invoked]<template <typename> typename Template, typename T>(
            [[maybe_unused]] Template<T> return_type,
            int key) noexcept -> fake_value_type {
          invoked = std::is_same_v<T, fake_value_type>;
          return T{key};
        });

    REQUIRE(std::is_same_v<decltype(actual), fake_value_type &>);
    REQUIRE(invoked);
    REQUIRE(target.contains(key));
  }

  SECTION(
      "get_or_put member function does not invoke handler when item is found") {
    int const key = 15;
    bool invoked = false;

    target.add(fake_value_type{key});
    [[maybe_unused]] auto &actual = target.get_or_put(
        key,
        [&invoked]<template <typename> typename Template, typename T>(
            [[maybe_unused]] Template<T> return_type,
            int key) noexcept -> fake_value_type {
          invoked = std::is_same_v<T, fake_value_type>;
          return T{key};
        });

    REQUIRE_FALSE(invoked);
    REQUIRE(target.contains(key));
  }
}