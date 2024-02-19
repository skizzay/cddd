//
// Created by andrew on 1/20/24.
//


#include <catch2/catch_all.hpp>
#include <skizzay/cqrs/command.h>

using namespace skizzay::simple::cqrs;

namespace {
    struct expected_command final : command_base<expected_command> {
    };

    struct unexpected_command final {
    };
}

TEST_CASE("commands are detected", "[command]") {
    REQUIRE(is_command_v<expected_command>);
    REQUIRE_FALSE(is_command_v<unexpected_command>);
}

TEST_CASE("properties can be retrieved", "[command]") {
    // Arrange
    uuid const id = uuid::v4();
    time_point auto const timestamp = std::chrono::system_clock::now();
    expected_command const cmd{{id, timestamp}};

    // Act & Assert
    REQUIRE(command_aggregate_id(cmd) == id);
    REQUIRE(command_timestamp(cmd) == timestamp);
}