#include <skizzay/cddd/concurrent_repository.h>

#include "skizzay/cddd/repository.h"
#include <catch.hpp>
#include <type_traits>

namespace {
struct fake_value_type {
  int id = {};
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

  //   SECTION("get member function does not invoke handler when item is found")
  //   {
  //     int const key = 15;
  //     bool invoked = false;

  //     auto actual = target.get(
  //         key, [&invoked]<template <typename> typename Template, typename T>(
  //                  [[maybe_unused]] Template<T> return_type, int) {
  //           invoked = std::is_same_v<T, fake_value_type>;
  //           return fake_value_type{};
  //         });

  //     REQUIRE(std::is_same_v<decltype(actual), fake_value_type>);
  //     REQUIRE(invoked);
  //   }
}