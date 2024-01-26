//
// Created by andrew on 1/25/24.
//

#include <skizzay/s11n/encode.h>
#include <catch2/catch_all.hpp>

using namespace skizzay::s11n;

TEST_CASE("Encoding an int to big endian", "[encode]") {
   SECTION("encode_big") {
      constexpr auto value = 0x12345678;
      constexpr auto expected = std::array{std::byte{0x12}, std::byte{0x34}, std::byte{0x56}, std::byte{0x78}};
      constexpr auto actual = encode_big(value);
      REQUIRE(expected == actual);
   }
   SECTION("encode<big>") {
      constexpr auto value = 0x12345678;
      constexpr auto expected = std::array{std::byte{0x12}, std::byte{0x34}, std::byte{0x56}, std::byte{0x78}};
      constexpr auto actual = encode<std::endian::big>(value);
      REQUIRE(expected == actual);
   }
}

TEST_CASE("Encoding a float to big endian", "[encode]") {
   SECTION("encode_big") {
      constexpr auto value = 1.0f;
      constexpr auto expected = std::array{std::byte{0x3F}, std::byte{0x80}, std::byte{0x00}, std::byte{0x00}};
      constexpr auto actual = encode_big(value);
      REQUIRE(expected == actual);
   }
   SECTION("encode<big>") {
      constexpr auto value = 1.0f;
      constexpr auto expected = std::array{std::byte{0x3F}, std::byte{0x80}, std::byte{0x00}, std::byte{0x00}};
      constexpr auto actual = encode<std::endian::big>(value);
      REQUIRE(expected == actual);
   }
}

TEST_CASE("Encoding a double to big endian", "[encode]") {
   SECTION("encode_big") {
      constexpr auto value = 1.0;
      constexpr auto expected = std::array{std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x3F}, std::byte{0xF0}, std::byte{0x00}, std::byte{0x00}};
      constexpr auto actual = encode_big(value);
      REQUIRE(expected == actual);
   }
   SECTION("encode<big>") {
      constexpr auto value = 1.0;
      constexpr auto expected = std::array{std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x3F}, std::byte{0xF0}, std::byte{0x00}, std::byte{0x00}};
      constexpr auto actual = encode<std::endian::big>(value);
      REQUIRE(expected == actual);
   }
}

TEST_CASE("Encoding an int to little endian", "[encode]") {
   SECTION("encode_little") {
      constexpr auto value = 0x12345678;
      constexpr auto expected = std::array{std::byte{0x78}, std::byte{0x56}, std::byte{0x34}, std::byte{0x12}};
      constexpr auto actual = encode_little(value);
      REQUIRE(expected == actual);
   }
   SECTION("encode<little>") {
      constexpr auto value = 0x12345678;
      constexpr auto expected = std::array{std::byte{0x78}, std::byte{0x56}, std::byte{0x34}, std::byte{0x12}};
      constexpr auto actual = encode<std::endian::little>(value);
      REQUIRE(expected == actual);
   }
}

TEST_CASE("Encoding a float to little endian", "[encode]") {
   SECTION("encode_little") {
      constexpr auto value = 1.0f;
      constexpr auto expected = std::array{std::byte{0x00}, std::byte{0x00}, std::byte{0x80}, std::byte{0x3F}};
      constexpr auto actual = encode_little(value);
      REQUIRE(expected == actual);
   }
   SECTION("encode<little>") {
      constexpr auto value = 1.0f;
      constexpr auto expected = std::array{std::byte{0x00}, std::byte{0x00}, std::byte{0x80}, std::byte{0x3F}};
      constexpr auto actual = encode<std::endian::little>(value);
      REQUIRE(expected == actual);
   }
}

// TEST_CASE("Encoding a double to little endian", "[encode]") {
//    SECTION("encode_little") {
//       constexpr auto value = 1.0;
//       constexpr auto expected = std::array{std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0xF0}, std::byte{0x3F}, std::byte{0x00}, std::byte{0x00}};
//       constexpr auto actual = encode_little(value);
//       REQUIRE(expected == actual);
//    }
//    SECTION("encode<little>") {
//       constexpr auto value = 1.0;
//       constexpr auto expected = std::array{std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0xF0}, std::byte{0x3F}, std::byte{0x00}, std::byte{0x00}};
//       constexpr auto actual = encode<std::endian::little>(value);
//       REQUIRE(expected == actual);
//    }
// }