//
// Created by andrew on 1/19/24.
//

#include <catch2/catch_all.hpp>
#include <cstddef>
#include <iostream>
#include <skizzay/cqrs/uuid.h>

using namespace skizzay::simple::cqrs;

TEST_CASE("uuid default ctor", "[uuid]") {
  constexpr uuid sut{};
  constexpr uuid expected = uuid::nil();
  REQUIRE(sut == expected);
  REQUIRE("00000000-0000-0000-0000-000000000000" == sut.to_string());
}

TEST_CASE("uuid v7 ctor", "[uuid]") {
  uuid sut = uuid::v7(std::chrono::milliseconds{0x0000000000000000});
  constexpr uuid expected = uuid::nil();
  REQUIRE(sut != expected);
}

TEST_CASE("printing to screen", "[uuid]") {
  // This test case is used to generate a random uuid for comparing against online generators/validators.
  std::cout << "random: " << uuid::v4().to_string() << std::endl;
  std::cout << "v7: " << uuid::v7().to_string() << std::endl;
}

TEST_CASE("uuid can be created from a uuid string", "[uuid]") {
  constexpr std::string_view expected = "647bc532-8d01-7487-ba4c-f56a4ff65ceb";
  constexpr uuid sut = uuid::from_string(expected);
  REQUIRE(uuid::variant::rfc4122 == sut.variant());
  REQUIRE(7 == sut.version());
  REQUIRE(expected == sut.to_string());
}

TEST_CASE("uuid can be created from a uuid wstring", "[uuid]") {
  constexpr std::wstring_view expected = L"647bc532-8d01-7487-ba4c-f56a4ff65ceb";
  constexpr uuid sut = uuid::from_string(expected);
  REQUIRE(uuid::variant::rfc4122 == sut.variant());
  REQUIRE(7 == sut.version());
  REQUIRE(expected == sut.to_wstring());
}