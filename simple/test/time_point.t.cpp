//
// Created by andrew on 1/20/24.
//

#include <catch2/catch_all.hpp>
#include <skizzay/time_point.h>

using namespace skizzay::simple::cqrs;

TEST_CASE("time_point detects std::chrono::time_point", "[time_point]") {
  REQUIRE(is_time_point_v<std::chrono::time_point<std::chrono::system_clock>>);
  REQUIRE_FALSE(is_time_point_v<int>);
}