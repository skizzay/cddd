//
// Created by andrew on 1/23/24.
//

#include <skizzay/aggregate_root_factory.h>
#include <catch2/catch_all.hpp>

using namespace skizzay::simple::cqrs;

namespace {
    template<int M>
    struct test_event : event_base<test_event<M>> {
        using event_base<test_event>::event_base;

        [[nodiscard]] static constexpr int value() noexcept { return M; }
    };

    using event_type = std::variant<test_event<1>, test_event<2>>;
    using event_vector = std::vector<event_type>;

    template<int M>
    struct test_command : command_base<test_command<M>> {
        using command_base<test_command>::command_base;
        [[nodiscard]] static constexpr int value() noexcept { return M; }
    };

    struct fake_event_source {
        [[nodiscard]] constexpr event_vector get_events(uuid const &id, std::size_t const begin, std::size_t const end) {
            get_events_requests.emplace_back(id, begin, end);
            return {};
        }

        std::vector<std::tuple<uuid, std::size_t, std::size_t>> get_events_requests;
    };

    struct fake_aggregate_root : aggregate_root<event_type, fake_aggregate_root> {
        using aggregate_root::aggregate_root;

        template<int M>
        event_vector calculate_changes(test_command<M> const &cmd) const noexcept {
            return {test_event<M>{command_aggregate_id(cmd)}};
        }

        template<int M>
        constexpr void on_event(test_event<M> const &) {
        }
    };

    struct fake_aggregate_root_factory {
        [[nodiscard]] fake_aggregate_root operator()(uuid const &id) const noexcept {
            // We're trying to simulate a snapshot here, so we'll just create a fake aggregate root and restore it.
            fake_aggregate_root result{id};
            result.restore_from(event_vector{
                test_event<1>{id},
                test_event<2>{id}
            });
            return result;
        }
    };
}

TEST_CASE("aggregate root factory creates unknown aggregate root", "[aggregate_root_factory]") {
    // Arrange
    aggregate_root_factory target{fake_event_source{}};

    // Act
    auto const result = target.create<fake_aggregate_root>(uuid::v7());

    // Assert
    REQUIRE(0 == result.version());
}

TEST_CASE("aggregate root factory creates known aggregate root", "[aggregate_root_factory]") {
    // Arrange
    aggregate_root_factory<fake_event_source, fake_aggregate_root> target{fake_event_source{}};

    // Act
    auto const result = target.create(uuid::v4());

    // Assert
    REQUIRE(0 == result.version());
}

TEST_CASE("aggregate root factory can load aggregates prebuilt from a snapshot", "[aggregate_root_factory]") {
    // Arrange
    aggregate_root_factory<fake_event_source, fake_aggregate_root, fake_aggregate_root_factory> target{fake_event_source{}};

    // Act
    auto const result = target.create(uuid::v4());

    // Assert
    REQUIRE(2 == result.version());
}