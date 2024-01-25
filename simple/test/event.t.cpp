//
// Created by andrew on 1/20/24.
//

#include <catch2/catch_all.hpp>
#include <skizzay/event.h>

using namespace skizzay::simple::cqrs;

namespace {
    struct expected_event final : event_base<expected_event> {
    };

    struct unexpected_event final {
    };
}

TEST_CASE("events are detected", "[event]") {
    REQUIRE(is_event_v<expected_event>);
    REQUIRE_FALSE(is_event_v<unexpected_event>);
}

TEST_CASE("variants of events are detected", "[event]") {
    REQUIRE(is_event_v<std::variant<expected_event>>);
    REQUIRE_FALSE(is_event_v<std::variant<unexpected_event>>);
}

TEST_CASE("event range is detected", "[event]") {
    REQUIRE(is_event_range_v<std::vector<std::variant<expected_event>>>);
    REQUIRE_FALSE(is_event_range_v<std::vector<unexpected_event>>);
}