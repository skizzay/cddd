//
// Created by andrew on 1/19/24.
//

#ifndef STATE_H
#define STATE_H

#include <type_traits>
#include <variant>

#include "command.h"
#include "event.h"
#include "exceptions.h"

namespace skizzay::simple::cqrs {

    template<typename S>
    requires std::is_class_v<S> && std::is_same_v<S, std::remove_cvref_t<S> >
    struct state_base {
        // ReSharper disable once CppMemberFunctionMayBeStatic
        [[noreturn]] S on_event(event auto const &) {
            throw unexpected_event{"Unexpected event for current state"};
        }
    };

    namespace state_details_ {
        template<typename S>
        concept of_state_base = std::derived_from<S, state_base<S>>;

        void validate_command(...) = delete;

        struct validate_command_fn final {
            constexpr void operator()(
                auto const &state,
                [[maybe_unused]] uuid const &id,
                [[maybe_unused]] std::uint64_t const version,
                command auto const &cmd
            ) const {
                if constexpr (requires { state.validate_command(id, version, cmd); }) {
                    state.validate_command(id, version, cmd);
                }
                else if constexpr (requires { state.validate_command(id, cmd); }) {
                    state.validate_command(id, cmd);
                }
                else if constexpr (requires { state.validate_command(cmd); }) {
                    state.validate_command(cmd);
                }
                else if constexpr (requires { validate_command(state, id, version, cmd); }) {
                    validate_command(state, id, version, cmd);
                }
                else if constexpr (requires { validate_command(state, id, cmd); }) {
                    validate_command(state, id, cmd);
                }
                else if constexpr (requires { validate_command(state, cmd); }) {
                    validate_command(state, cmd);
                }
            }
        };

        event_range auto calculate_changes(...) = delete;

        struct calculate_changes_fn final {
            constexpr event_range auto operator()(
                auto const &state,
                uuid const &id,
                std::uint64_t const version,
                command auto const &cmd
            ) const requires requires { { state.calculate_changes(id, version, cmd) } -> event_range; } {
                    return state.calculate_changes(id, version, cmd);
            }

            constexpr event_range auto operator()(
                auto const &state,
                uuid const &id,
                [[maybe_unused]] std::uint64_t const version,
                command auto const &cmd
            ) const requires requires { { state.calculate_changes(id, cmd) } -> event_range; } {
                return state.calculate_changes(id, cmd);
            }

            constexpr event_range auto operator()(
                auto const &state,
                [[maybe_unused]] uuid const &id,
                [[maybe_unused]] std::uint64_t const version,
                command auto const &cmd
            ) const requires requires { { state.calculate_changes(cmd) } -> event_range; } {
                return state.calculate_changes(cmd);
            }

            constexpr event_range auto operator()(
                auto const &state,
                uuid const &id,
                std::uint64_t const version,
                command auto const &cmd
            ) const requires requires { { calculate_changes(state, id, version, cmd) } -> event_range; } {
                return calculate_changes(state, id, version, cmd);
            }

            constexpr event_range auto operator()(
                auto const &state,
                uuid const &id,
                [[maybe_unused]] std::uint64_t const version,
                command auto const &cmd
            ) const requires requires { { calculate_changes(state, id, cmd) } -> event_range; } {
                return calculate_changes(state, id, cmd);
            }

            constexpr event_range auto operator()(
                auto const &state,
                [[maybe_unused]] uuid const &id,
                [[maybe_unused]] std::uint64_t const version,
                command auto const &cmd
            ) const requires requires { { calculate_changes(state, cmd) } -> event_range; } {
                return calculate_changes(state, cmd);
            }
        };
    }

    inline namespace state_fn_ {
        inline constexpr state_details_::validate_command_fn validate_command{};
        inline constexpr state_details_::calculate_changes_fn calculate_changes{};
    }

    template<typename>
    struct is_state : std::false_type {};

    template<state_details_::of_state_base S>
    struct is_state<S> : std::true_type {};

    template<typename... S>
    struct is_state<std::variant<S...>> : std::conjunction<is_state<S>...> {};

    template<typename S>
    inline constexpr bool is_state_v = is_state<S>::value;

    template<typename S>
    concept state = is_state_v<S>;
} // skizzay


#endif //STATE_H