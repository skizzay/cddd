//
// Created by andrew on 2/15/24.
//

#include <skizzay/s11n/binary_reader.h>
#include <skizzay/s11n/binary_writer.h>
#include <catch2/catch_all.hpp>
#include <random>

#include <skizzay/s11n/memory_buffer.h>

using namespace skizzay::s11n;
using namespace std::string_view_literals;

namespace {
    template<typename I>
        requires std::is_integral_v<I> || std::is_floating_point_v<I>
    constexpr inline auto random_number = [](
        I const min = std::numeric_limits<I>::min(), I const max = std::numeric_limits<I>::max()
    ) noexcept -> I {
        thread_local std::random_device rd;
        thread_local std::mt19937_64 gen{rd()};
        if constexpr (std::is_floating_point_v<I>) {
            return std::uniform_real_distribution<I>{min, max}(gen);
        }
        else {
            return std::uniform_int_distribution<I>{min, max}(gen);
        }
    };
}

TEMPLATE_TEST_CASE_SIG("integral (de)serialization", "[binary_writer,binary_reader]",
                       ((typename I, std::endian E), I, E),
                       (std::uint8_t, std::endian::little), (std::uint8_t, std::endian::big),
                       (std::uint16_t, std::endian::little), (std::uint16_t, std::endian::big),
                       (std::uint32_t, std::endian::little), (std::uint32_t, std::endian::big),
                       (std::uint64_t, std::endian::little), (std::uint64_t, std::endian::big)) {
    memory_buffer buffer;
    I const original = random_number<I>(0, 100);
    binary_writer<decltype(buffer), E> writer{buffer};
    writer << original;
    binary_reader<decltype(buffer), E> reader{buffer};
    I actual;
    reader >> actual;

    REQUIRE(original == actual);
}

TEMPLATE_TEST_CASE_SIG("floating-point (de)serialization", "[binary_writer,binary_reader]",
                       ((typename I, std::endian E), I, E),
                       (float, std::endian::little), (float, std::endian::big),
                       (double, std::endian::little), (double, std::endian::big),
                       (long double, std::endian::little), (long double, std::endian::big)) {
    memory_buffer buffer;
    I const original = random_number<I>(0.0, 1000.0) / 1024.0;
    binary_writer<decltype(buffer), E> writer{buffer};
    writer << original;
    binary_reader<decltype(buffer), E> reader{buffer};
    I actual;
    reader >> actual;

    REQUIRE(original == actual);
}

TEMPLATE_TEST_CASE_SIG("enum (de)serialization", "[binary_writer,binary_reader]",
                       ((std::endian E), E), (std::endian::little), (std::endian::big), (std::endian::native)) {
    memory_buffer buffer;
    binary_writer<decltype(buffer), E> writer{buffer};
    writer << E;
    binary_reader<decltype(buffer), E> reader{buffer};
    std::endian actual;
    reader >> actual;

    REQUIRE(E == actual);
}

TEMPLATE_TEST_CASE_SIG("string (de)serialization", "[binary_writer,binary_reader]",
                       ((std::endian E), E), (std::endian::little), (std::endian::big), (std::endian::native)) {
    memory_buffer buffer;
    binary_writer<decltype(buffer), E> writer{buffer};
    constexpr auto original = "Hello, World!"sv;
    writer << std::ranges::size(original) << original;
    binary_reader<decltype(buffer), E> reader{buffer};
    std::size_t n;
    reader >> n;
    std::string actual(n, '\0');
    reader >> actual;

    REQUIRE(original.size() == n);
    REQUIRE(original == actual);
}

TEMPLATE_TEST_CASE_SIG("time (de)serialization", "[binary_writer,binary_reader]",
                       ((std::endian E), E), (std::endian::little), (std::endian::big), (std::endian::native)) {
    memory_buffer buffer;
    binary_writer<decltype(buffer), E> writer{buffer};
    auto const original = std::chrono::system_clock::now();
    writer << original;
    binary_reader<decltype(buffer), E> reader{buffer};
    std::chrono::system_clock::time_point actual;
    reader >> actual;

    REQUIRE(original == actual);
}

TEMPLATE_TEST_CASE_SIG("boolean (de)serialization", "[binary_writer,binary_reader]",
                       ((std::endian E, bool B), E, B),
                       (std::endian::little, true), (std::endian::big, true), (std::endian::native, true),
                       (std::endian::little, false), (std::endian::big, false), (std::endian::native, false)) {
    memory_buffer buffer;
    binary_writer<decltype(buffer), E> writer{buffer};
    writer << B;
    binary_reader<decltype(buffer), E> reader{buffer};
    bool actual;
    reader >> actual;

    REQUIRE(B == actual);
}
