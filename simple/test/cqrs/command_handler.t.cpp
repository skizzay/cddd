//
// Created by andrew on 1/23/24.
//

#include <skizzay/cqrs/command_handler.h>
#include <skizzay/cqrs/memory_event_store.h>
#include <catch2/catch_all.hpp>

using namespace skizzay::simple::cqrs;

namespace {
    template<int N>
    struct fake_event final : event_base<fake_event<N>> {
        using event_base<fake_event>::event_base;
        [[nodiscard]] constexpr int value() const noexcept {
            return N;
        }
    };

    using event_type = std::variant<fake_event<1>, fake_event<2>, fake_event<3>>;
    using event_vector = std::vector<event_type>;
    using event_store = memory_event_store<std::chrono::system_clock, fake_event<1>, fake_event<2>, fake_event<3>>;

    template<int N>
    struct fake_command : command_base<fake_command<N>> {
        [[nodiscard]] constexpr int value() const noexcept {
            return N;
        }
    };

    struct fake_aggregate_root {
        constexpr void handle_command(command auto const &cmd) noexcept {
            REQUIRE(id_ == command_aggregate_id(cmd));
        }

        [[nodiscard]] constexpr event_vector commit() && noexcept {
            return std::move(events_);
        }

        uuid id_;
        event_vector events_;
    };

    struct fake_factory {
        template<std::same_as<fake_aggregate_root> AggregateRoot>
        constexpr AggregateRoot create(uuid const &id) const noexcept {
            return {id, {}};
        }

        [[nodiscard]] constexpr fake_aggregate_root create(uuid const &id) const noexcept {
            return {id, {}};
        }
    };
}

TEST_CASE("command handler can exactly know which aggregate root to create", "[command_handler]") {
    event_store store{};
    auto target = command_handler{fake_factory{}, store.as_event_stream()};
    fake_command<1> const cmd{{uuid::v4(), std::chrono::system_clock::now()}};

    target.handle(cmd);
}

TEST_CASE("command handler can be told which aggregate root to create", "[command_handler]") {
    event_store store{};
    auto target = command_handler{fake_factory{}, store.as_event_stream()};
    fake_command<1> const cmd{{uuid::v4(), std::chrono::system_clock::now()}};

    target.handle<fake_aggregate_root>(cmd);
}