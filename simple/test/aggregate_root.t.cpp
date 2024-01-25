//
// Created by andrew on 1/20/24.
//

#include <catch2/catch_all.hpp>
#include <skizzay/command.h>
#include <skizzay/event.h>
#include <skizzay/state.h>
#include <variant>
#include <vector>

#include "skizzay/aggregate_root.h"
#include "skizzay/state_machine.h"
#include "skizzay/tag.h"

using namespace skizzay::simple::cqrs;

namespace {
    template<int N>
    struct test_command : command_base<test_command<N>> {
        using command_base<test_command>::command_base;
        [[nodiscard]] static constexpr int value() noexcept { return N; }
    };

    template<int N>
    struct test_event : event_base<test_event<N>> {
        using event_base<test_event>::event_base;

        [[nodiscard]] static constexpr int value() noexcept { return N; }
    };

    using event_type = std::variant<test_event<1>, test_event<2>>;
    using event_vector = std::vector<event_type>;

    template<int N>
    struct test_state : state_base<test_state<N>> {
        [[nodiscard]] static constexpr int value() noexcept { return N; }
        event_vector events;

        template<int M>
        constexpr void validate_command(uuid const &id, test_command<M> const &cmd) const {
            REQUIRE(id == command_aggregate_id(cmd));
        }

        template<int M>
        [[nodiscard]] constexpr event_vector calculate_changes(uuid const &aggregate_id, test_command<M> const &) const noexcept {
            return {test_event<M>{aggregate_id}};
        }

        template<int M>
        [[nodiscard]] constexpr auto on_event(test_event<M> const &) const noexcept {
            return test_state<M>{};
        }
    };

    struct fake_aggregate_root : aggregate_root<event_type, fake_aggregate_root> {
        friend tag_t<skizzay::simple::cqrs::calculate_changes>;
        friend tag_t<skizzay::simple::cqrs::on_event>;

        using aggregate_root::aggregate_root;

        event_vector events_encountered;

    private:
        template<int M>
        constexpr void validate_command(uuid const &id, test_command<M> const &cmd) const {
            REQUIRE(id == command_aggregate_id(cmd));
        }

        template<int M>
        [[nodiscard]] constexpr event_vector calculate_changes(uuid const &aggregate_id, test_command<M> const &) const noexcept {
            return {test_event<M>{aggregate_id}};
        }

        template<int M>
        constexpr void on_event(test_event<M> const &event) {
            events_encountered.push_back(event);
        }
    };
}

TEST_CASE("aggregate root can handle commands", "[aggregate_root]") {
    SECTION("as a state machine") {
        // Arrange
        aggregate_root<event_type, state_machine<test_state<1>, test_state<2>>> ar{uuid::v4()};

        // Act
        ar.handle_command(test_command<2>{ar.id()});

        // Assert
        REQUIRE(ar.is_state<test_state<2>>());
        REQUIRE(ar.uncommitted_events().size() == 1);
        // We haven't yet committed the events, so the version should still be 0
        REQUIRE(ar.version() == 0);
    }

    SECTION("as a standalone aggregate root") {
        // Arrange
        fake_aggregate_root ar{uuid::v4()};

        // Act
        ar.handle_command(test_command<2>{ar.id()});

        // Assert
        REQUIRE(ar.events_encountered.size() == 1);
        REQUIRE(2 == std::visit([]<int M>(test_event<M> const &e) { return e.value(); }, ar.events_encountered[0]));
        REQUIRE(ar.uncommitted_events().size() == 1);
        // We haven't yet committed the events, so the version should still be 0
        REQUIRE(ar.version() == 0);
    }
}

TEST_CASE("aggregate root increments version after commit", "[aggregate_root]") {
    SECTION("as a state machine") {
        // Arrange
        aggregate_root<event_type, state_machine<test_state<1>, test_state<2>>> ar{uuid::v4()};

        // Act
        ar.handle_command(test_command<2>{ar.id()});
        [[maybe_unused]] auto events = std::move(ar).commit();

        // Assert
        REQUIRE(ar.uncommitted_events().empty());
        REQUIRE(ar.version() == 1);
    }
    SECTION("as a standalone aggregate root") {
        // Arrange
        fake_aggregate_root ar{uuid::v4()};

        // Act
        ar.handle_command(test_command<2>{ar.id()});
        [[maybe_unused]] auto events = std::move(ar).commit();

        // Assert
        REQUIRE(ar.uncommitted_events().empty());
        REQUIRE(ar.version() == 1);
    }
}

TEST_CASE("aggregate root can restore from event range", "[aggregate_root]") {
    SECTION("as a state machine") {
        // Arrange
        aggregate_root<event_type, state_machine<test_state<1>, test_state<2>>> ar{uuid::v4()};
        event_vector const events{test_event<1>{ar.id()}, test_event<2>{ar.id()}};

        // Act
        ar.restore_from(events);

        // Assert
        REQUIRE(ar.uncommitted_events().empty());
        REQUIRE(ar.version() == 2);
    }
    SECTION("as a standalone aggregate root") {
        // Arrange
        fake_aggregate_root ar{uuid::v4()};
        event_vector const events{test_event<1>{ar.id()}, test_event<2>{ar.id()}};

        // Act
        ar.restore_from(events);

        // Assert
        REQUIRE(ar.uncommitted_events().empty());
        REQUIRE(ar.version() == 2);
    }
}