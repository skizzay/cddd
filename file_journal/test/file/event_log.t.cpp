//
// Created by andrew on 2/14/24.
//

#include <skizzay/file/event_log.h>
#include <catch2/catch_all.hpp>

#include <random>
#include <filesystem>
#include <skizzay/s11n/memory_buffer.h>

using namespace skizzay::simple::cqrs::file_journal;
using namespace skizzay;

namespace {
    template<std::uint64_t N>
    struct test_event final : simple::cqrs::event_base<test_event<N> > {
        test_event() noexcept
            : simple::cqrs::event_base<test_event>{simple::cqrs::uuid::v4()} {
            this->sequence_number = N;
            this->timestamp = std::chrono::system_clock::now();
        }

        template<s11n::sink S, std::endian E>
        friend s11n::binary_writer<S, E> &operator<<(s11n::binary_writer<S, E> &writer, test_event const &e) {
            return writer << e.stream_id.bytes() << e.sequence_number << e.timestamp;
        }
    };
}

TEST_CASE("event_log") {
    event_log target{s11n::memory_buffer{}, s11n::memory_buffer{}};

    REQUIRE(std::random_access_iterator<decltype(target.begin())>);
    REQUIRE(std::ranges::random_access_range<decltype(target)>);
    REQUIRE(std::movable<decltype(target)>);
    REQUIRE(std::ranges::enable_view<decltype(target)>);

    SECTION("empty") {
        REQUIRE(target.begin() == target.end());
    }

    SECTION("populated") {
        target.push(test_event<1>{});
        target.push(test_event<2>{});
        target.push(test_event<3>{});
        REQUIRE(target.begin() != target.end());
        REQUIRE(target.begin()->size() == sizeof(test_event<1>));
        REQUIRE(std::ranges::size(target) == 3);

        SECTION("iterator arithmetic") {
            auto i1 = target.begin();
            auto const i2 = i1 + 1;
            REQUIRE(1 == std::ranges::distance(i1, i2));
            REQUIRE(++i1 == i2);
            i1 += 2;
            REQUIRE(i1 == target.end());
        }

        REQUIRE(target.begin()[1].size() == sizeof(test_event<2>));
    }
}
