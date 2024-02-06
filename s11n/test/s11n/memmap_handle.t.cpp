//
// Created by andrew on 2/5/24.
//

#include <iostream>
#include <skizzay/s11n/memmap_handle.h>
#include <catch2/catch_all.hpp>
#include <random>
#include <skizzay/s11n/sink.h>

#include "skizzay/s11n/encode.h"

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

    constexpr inline auto temp_file = []() -> std::filesystem::path {
        auto const temp_filename_template = std::filesystem::temp_directory_path() / "s11n-XXXXXX";
        char temp_filename[PATH_MAX];
        std::strcpy(temp_filename, temp_filename_template.c_str());
        mktemp(temp_filename);
        std::cout << temp_filename << std::endl;
        return std::filesystem::path{temp_filename};
    };
}

TEST_CASE("writing to the memmap advances the position", "[posix_memmap_sink,memmap_handle]") {
    auto target = posix_memmap_sink::open_private(temp_file());
    REQUIRE(sink<decltype(target)>);
    REQUIRE(sink_position(target) == 0);

    SECTION("write a 64-bit integer to file") {
        auto const expected = random_number();
        auto const span_num_bytes = sink_write(target, encode_native(expected));
        sink_flush(target);
        REQUIRE(static_cast<std::size_t>(target.sink_position()) == span_num_bytes);

        SECTION("write a string_view to file") {
            constexpr auto payload = "Hello, World!"sv;
            auto const string_num_bytes = sink_write(target, payload);
            sink_flush(target);
            auto const expected_position = span_num_bytes + string_num_bytes;
            REQUIRE(string_num_bytes == payload.size());
            REQUIRE(static_cast<std::size_t>(sink_position(target)) == expected_position);
        }

        SECTION("write a string to file") {
            auto const string_num_bytes = sink_write(target, "Hello, World!");
            sink_flush(target);
            auto const expected_position = span_num_bytes + string_num_bytes;
            REQUIRE(string_num_bytes == "Hello, World!"sv.size());
            REQUIRE(static_cast<std::size_t>(sink_position(target)) == expected_position);
        }

        SECTION("write a wide-string to file") {
            auto const string_num_bytes = sink_write(target, L"Hello, World!");
            sink_flush(target);
            auto const expected_position = span_num_bytes + string_num_bytes;
            REQUIRE(string_num_bytes == (L"Hello, World!"sv.size() * sizeof(wchar_t)));
            REQUIRE(static_cast<std::size_t>(sink_position(target)) == expected_position);
        }
    }

    SECTION("write a null-terminated string") {
        auto const string_num_bytes = sink_write(target, "Hello, World!");
        auto const null_character_num_bytes = sink_write(target, '\0');
        sink_flush(target);
        auto const expected_position = string_num_bytes + null_character_num_bytes;
        REQUIRE(null_character_num_bytes == sizeof(char));
        REQUIRE(static_cast<std::size_t>(sink_position(target)) == expected_position);
    }
}
