//
// Created by andrew on 1/23/24.
//

#include <skizzay/cqrs/memory_event_store.h>
#include <catch2/catch_all.hpp>

using namespace skizzay::simple::cqrs;

namespace {
    struct fake_clock final {
        std::chrono::system_clock::time_point now() {
            return last_value = std::chrono::system_clock::now();
        }

        std::chrono::system_clock::time_point last_value{};
    };

    template<int N>
    struct fake_event final : event_base<fake_event<N>> {
        [[nodiscard]] constexpr int value() const noexcept {
            return N;
        }
    };
}

TEST_CASE("get_events given unknown id returns empty range", "[memory_event_store]") {
    memory_event_store<fake_clock, fake_event<1>> const sut{};
    REQUIRE(sut.get_events(uuid{}, 1, 1).empty());
}

TEST_CASE("get_events after put_events returns events", "[memory_event_store]") {
    memory_event_store<std::chrono::system_clock, fake_event<1>> sut{};
    uuid const id = uuid::v4();
    sut.put_events(id, std::vector{fake_event<1>{}, fake_event<1>{}});
    REQUIRE(sut.get_events(id, 1, 2).size() == 1);
}