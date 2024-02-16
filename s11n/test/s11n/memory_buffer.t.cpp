//
// Created by andrew on 2/5/24.
//

#include <skizzay/s11n/memory_buffer.h>
#include <catch2/catch_all.hpp>
#include <random>
#include <skizzay/s11n/io_device.h>

#include <skizzay/s11n/encode.h>

using namespace skizzay::s11n;
using namespace std::string_view_literals;

namespace {
    constexpr inline auto random_number = []<std::integral I=std::uint64_t>(
        I const min = std::numeric_limits<I>::min(), I const max = std::numeric_limits<I>::max()
    ) noexcept -> I {
        thread_local std::random_device rd;
        thread_local std::mt19937_64 gen{rd()};
        return std::uniform_int_distribution<I>{min, max}(gen);
    };
}

TEST_CASE("memory buffer has independent read/write points", "[memory_buffer]") {
    auto target = memory_buffer{};
    REQUIRE(has_independent_read_write_pointers_v<decltype(target)>);
}

TEST_CASE("seeking before buffer beginning throws", "[memory_buffer]") {
    auto target = memory_buffer{};
    REQUIRE(sink<decltype(target)>);
    REQUIRE(write_position(target) == 0);
    REQUIRE_THROWS_AS(seek_write(target, -1, seek_origin::beginning), std::out_of_range);
}

TEST_CASE("seeking beyond end throws", "[memory_buffer]") {
    auto target = memory_buffer{};
    REQUIRE(sink<decltype(target)>);
    REQUIRE(write_position(target) == 0);
    REQUIRE_THROWS_AS(seek_write(target, 1, seek_origin::end), std::out_of_range);
}

TEST_CASE("writing to the buffer advances the position", "[memory_buffer]") {
    using skizzay::s11n::write;
    auto target = memory_buffer{};
    REQUIRE(sink<decltype(target)>);
    REQUIRE(write_position(target) == 0);

    SECTION("write a 64-bit integer to file") {
        auto const expected = random_number();
        auto const span_num_bytes = write(target, encode_native(expected));
        flush(target);
        REQUIRE(static_cast<std::size_t>(write_position(target)) == span_num_bytes);

        SECTION("write a string_view to file") {
            constexpr auto payload = "Hello, World!"sv;
            auto const string_num_bytes = write(target, payload);
            flush(target);
            auto const expected_position = span_num_bytes + string_num_bytes;
            REQUIRE(string_num_bytes == payload.size());
            REQUIRE(static_cast<std::size_t>(write_position(target)) == expected_position);
        }

        SECTION("write a string to file") {
            auto const string_num_bytes = write(target, "Hello, World!");
            flush(target);
            auto const expected_position = span_num_bytes + string_num_bytes;
            REQUIRE(string_num_bytes == "Hello, World!"sv.size());
            REQUIRE(static_cast<std::size_t>(write_position(target)) == expected_position);
        }

        SECTION("write a wide-string to file") {
            auto const string_num_bytes = write(target, L"Hello, World!");
            flush(target);
            auto const expected_position = span_num_bytes + string_num_bytes;
            REQUIRE(string_num_bytes == (L"Hello, World!"sv.size() * sizeof(wchar_t)));
            REQUIRE(static_cast<std::size_t>(write_position(target)) == expected_position);
        }
    }

    SECTION("write a null-terminated string") {
        auto const string_num_bytes = write(target, "Hello, World!");
        auto const null_character_num_bytes = write(target, encode_native('\0'));
        flush(target);
        auto const expected_position = string_num_bytes + null_character_num_bytes;
        REQUIRE(null_character_num_bytes == sizeof(char));
        REQUIRE(static_cast<std::size_t>(write_position(target)) == expected_position);
    }
}

TEST_CASE("buffer can write (sink) and read (source)", "[memory_buffer]") {
    using skizzay::s11n::write;
    using skizzay::s11n::read;

    auto target = memory_buffer{};
    REQUIRE(sink<decltype(target)>);
    REQUIRE(source<decltype(target)>);

    write(target, "Hello, World!");
    write(target, encode_native(random_number(static_cast<short>(1), static_cast<short>(20))));

    seek_read(target, 0, seek_origin::beginning);
    REQUIRE(read_position(target) == 0);

    SECTION("reading less than the number of bytes written") {
        std::array<std::byte, 5> buffer{};
        auto const num_bytes_read = read(target, buffer, buffer.size());
        REQUIRE(num_bytes_read == buffer.size());
        REQUIRE(buffer == std::array<std::byte, 5>{std::byte{'H'}, std::byte{'e'}, std::byte{'l'}, std::byte{'l'},
                std::byte{'o'}});
        REQUIRE(num_bytes_read == static_cast<std::size_t>(read_position(target) ));
    }

    SECTION("reading more than number of bytes written") {
        std::vector<std::byte> buffer(100);
        auto const num_bytes_read = read(target, buffer, buffer.size());
        REQUIRE(num_bytes_read == "Hello, World!"sv.size() + sizeof(short));
        REQUIRE(num_bytes_read == static_cast<std::size_t>(read_position(target) ));
    }
}
