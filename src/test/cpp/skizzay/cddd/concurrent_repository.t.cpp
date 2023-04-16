#include <skizzay/cddd/concurrent_repository.h>

#include "skizzay/cddd/nullable.h"
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

  SECTION("put member function will add a new entry") {
    int const key = 42;
    target.put(fake_value_type{key});
    REQUIRE(target.contains(key));
  }

  SECTION("add member function will add a new entry") {
    int const key = 42;
    target.add(fake_value_type{key});
    REQUIRE(target.contains(key));
  }

  SECTION("put member function will overwrite an existing entry") {
    int const key = 42;
    target.put(fake_value_type{key, 1});
    target.put(fake_value_type{key, 2});
    auto const actual = target.get(key);
    REQUIRE(2 == actual.other_value);
  }

  SECTION("add member function will not overwrite an existing entry") {
    int const key = 42;
    target.add(fake_value_type{key, 1});
    auto const actual = target.add(fake_value_type{key, 2});
    REQUIRE_FALSE(actual);
  }

  SECTION("get member function returns non-null value when item is found") {
    int const key = 15;
    target.put(fake_value_type{key});

    auto actual = target.get(key);

    REQUIRE_FALSE(skizzay::cddd::is_null(actual));
  }

  SECTION("get_or_put member function invokes handler when item is not found") {
    int const key = 15;
    bool invoked = false;

    [[maybe_unused]] auto actual =
        target.get_or_add(key, [&invoked](int key) noexcept -> fake_value_type {
          invoked = true;
          return fake_value_type{key};
        });

    REQUIRE(std::is_same_v<decltype(actual), fake_value_type>);
    REQUIRE(invoked);
    REQUIRE(target.contains(key));
  }

  SECTION(
      "get_or_put member function does not invoke handler when item is found") {
    int const key = 15;
    bool invoked = false;

    target.add(fake_value_type{key});
    [[maybe_unused]] auto actual =
        target.get_or_add(key, [&invoked](int key) noexcept -> fake_value_type {
          invoked = true;
          return fake_value_type{key};
        });

    REQUIRE_FALSE(invoked);
    REQUIRE(target.contains(key));
  }

  SECTION("contains CPO returns false when nothing has been added or put in "
          "the repository") {
    using skizzay::cddd::contains;
    int const key = 42;
    REQUIRE_FALSE(contains(std::as_const(target), key));
  }

  SECTION("contains CPO returns true after an item has been put in the "
          "repository") {
    using skizzay::cddd::contains;
    using skizzay::cddd::put;
    int const key = 42;
    put(target, fake_value_type{key});
    REQUIRE(contains(std::as_const(target), key));
  }

  SECTION("contains CPO returns true after an item has been added the "
          "repository") {
    using skizzay::cddd::add;
    using skizzay::cddd::contains;
    int const key = 42;
    add(target, fake_value_type{key});
    REQUIRE(contains(std::as_const(target), key));
  }
}