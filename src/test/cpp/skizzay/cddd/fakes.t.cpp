#include "fakes.h"

#include <array>
#include <concepts>
#include <memory>
#include <skizzay/cddd/aggregate_root.h>
#include <skizzay/cddd/dereference.h>
#include <skizzay/cddd/identifier.h>
#include <skizzay/cddd/timestamp.h>
#include <type_traits>

#include <catch2/catch_test_macros.hpp>

using namespace skizzay::cddd;

// These tests just bring some sanity checks to the overall unit tests.  If
// something in here fails, then it means one of our axioms doesn't hold.

TEST_CASE("fake clock is a clock") {
  using ptr = std::shared_ptr<test::fake_clock>;
  REQUIRE(concepts::clock<test::fake_clock>);
  REQUIRE(std::same_as<dereference_t<ptr>, test::fake_clock &>);
  REQUIRE(concepts::clock<test::fake_clock &>);
  REQUIRE(concepts::clock<ptr>);
  REQUIRE(concepts::clock<std::chrono::system_clock>);
}

TEST_CASE("fake clock's result is a timestamp") {
  REQUIRE(concepts::timestamp<timestamp_t<test::fake_clock>>);
}

TEST_CASE("std::string is an identifier") {
  REQUIRE(std::regular<std::remove_cvref_t<dereference_t<std::string>>>);
  REQUIRE(concepts::identifier<std::string>);
}

TEST_CASE("std::size_t is a version") {
  REQUIRE(std::unsigned_integral<std::size_t>);
}

TEST_CASE("fake_event is a domain event") {
  using ptr = std::unique_ptr<test::fake_event<1>>;
  REQUIRE(concepts::domain_event<test::fake_event<1>>);
  REQUIRE(concepts::domain_event<ptr>);
}

TEST_CASE("fake aggregate is an aggregate root") {
  using aggregate_type = test::fake_aggregate<std::array<char, 1>>;
  REQUIRE(concepts::aggregate_root<aggregate_type>);
  REQUIRE(std::same_as<aggregate_type,
                       std::remove_cvref_t<dereference_t<aggregate_type>>>);
  REQUIRE(std::same_as<std::add_lvalue_reference_t<aggregate_type>,
                       dereference_t<aggregate_type>>);
  REQUIRE(concepts::identifiable<aggregate_type>);
  REQUIRE(concepts::identifiable<std::unique_ptr<aggregate_type>>);
  REQUIRE(concepts::timestamped<std::unique_ptr<aggregate_type>>);
  REQUIRE(concepts::aggregate_root<std::unique_ptr<aggregate_type>>);
}