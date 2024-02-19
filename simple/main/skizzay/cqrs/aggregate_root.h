//
// Created by andrew on 1/19/24.
//

#ifndef AGGREGATE_ROOT_H
#define AGGREGATE_ROOT_H
#include <algorithm>
#include <utility>

#include "command.h"
#include "event.h"
#include "state.h"
#include "state_machine.h"
#include "type_sequence.h"
#include "uuid.h"

namespace skizzay::simple::cqrs { namespace aggregate_root_details_ {
        template<typename, typename>
        struct calculate_changes_result_type_impl;

        template<command C, state S>
            requires std::is_invocable_v<decltype(calculate_changes), S const &, uuid const &, std::uint64_t, C const &>
        struct calculate_changes_result_type_impl<C, S> {
            using type = type_sequence<std::invoke_result_t<decltype(calculate_changes), S const &, uuid const &,
                std::uint64_t,
                C const &> >;
        };

        template<command C, state S>
        struct calculate_changes_result_type_impl<C, S> {
            using type = type_sequence<>;
        };

        template<typename, typename>
        struct calculate_changes_result_type;

        template<command C, state... Ss>
        struct calculate_changes_result_type<C, type_sequence<Ss...> > {
            using type = typename type_sequence<>::concat_t<typename calculate_changes_result_type_impl<C, Ss>::type
                ...>::common_type_t;
        };

        template<typename Command, typename States>
        using calculate_changes_result_type_t = typename calculate_changes_result_type<Command, States>::type;

        template<typename>
        struct impl_storage;

        template<typename FSM>
            requires is_template_v<FSM, state_machine>
        struct impl_storage<FSM> {
            [[nodiscard]] constexpr decltype(auto) current_state() const noexcept {
                return fsm_.current_state();
            }

            constexpr decltype(auto) query(auto &&fn) const noexcept(noexcept(fsm_.query(std::forward<decltype(fn)>(fn)))) {
                return fsm_.query(std::forward<decltype(fn)>(fn));
            }

            template<state S>
            [[nodiscard]] constexpr bool is_state() const noexcept {
                return fsm_.template is_state<S>();
            }

        protected:
            [[nodiscard]] constexpr FSM &event_handler() noexcept {
                return fsm_;
            }

            [[nodiscard]] constexpr event_range auto validate_command_and_calculate_changes(uuid const &id,
                std::uint64_t const version, command auto const &cmd
            ) const {
                return fsm_.query([&](state auto const &current
            ) -> calculate_changes_result_type_t<decltype(cmd), typename FSM::states_sequence> {
                        if constexpr (std::is_invocable_v<decltype(calculate_changes), decltype(current) const &, uuid
                            const &, std::uint64_t, decltype(cmd) const &>) {
                            validate_command(current, id, version, cmd);
                            return calculate_changes(current, id, version, cmd);
                        }
                        else {
                            throw unexpected_command{"Unexpected command for current state"};
                        }
                    });
            }

        private:
            FSM fsm_{};
        };

        template<typename Crtp>
            requires std::is_class_v<Crtp> && std::is_same_v<Crtp, std::remove_cvref_t<Crtp> > && (!is_template_v<Crtp,
                         state_machine>)
        struct impl_storage<Crtp> {
        protected:
            [[nodiscard]] constexpr Crtp &event_handler() noexcept {
                return self();
            }

            [[nodiscard]] constexpr event_range auto validate_command_and_calculate_changes(uuid const &id,
                std::uint64_t const version, command auto const &cmd
            ) const {
                validate_command(self(), id, version, cmd);
                return calculate_changes(self(), id, version, cmd);
            }

        private:
            [[nodiscard]] constexpr Crtp &self() noexcept
                requires std::derived_from<Crtp, impl_storage> {
                return static_cast<Crtp &>(*this);
            }

            [[nodiscard]] constexpr Crtp const &self() const noexcept
                requires std::derived_from<Crtp, impl_storage> {
                return static_cast<Crtp const &>(*this);
            }
        };
    }

    template<event Event, typename Impl>
    class aggregate_root : public aggregate_root_details_::impl_storage<Impl> {
    public:
        explicit constexpr aggregate_root(uuid const &id) noexcept
            : id_{id} {
        }

        [[nodiscard]] constexpr uuid const &id() const noexcept {
            return id_;
        }

        [[nodiscard]] constexpr std::uint64_t version() const noexcept {
            return version_;
        }

        [[nodiscard]] constexpr event_range auto const &uncommitted_events() const & noexcept {
            return uncommitted_events_;
        }

        constexpr void restore_from(event_range auto const &events) {
            std::ranges::for_each(events, [this](event auto const &evt) {
                on_event(this->event_handler(), evt);
                ++version_;
            });
        }

        constexpr void handle_command(command auto const &cmd) {
            event_range auto events = this->validate_command_and_calculate_changes(id(), version(), cmd);
            if constexpr (std::ranges::sized_range<decltype(events)>) {
                uncommitted_events_.reserve(std::max(
                    uncommitted_events_.capacity(),
                    std::ranges::size(uncommitted_events_) + std::ranges::size(events)));
            }
            std::ranges::for_each(events, [this](event auto &evt) {
                on_event(this->event_handler(), evt);
                uncommitted_events_.emplace_back(std::move(evt));
            });
        }

        [[nodiscard]]
        constexpr std::vector<Event> commit() && noexcept {
            version_ += uncommitted_events_.size();
            return std::move(uncommitted_events_);
        }

    private:
        uuid id_;
        std::uint64_t version_{};
        std::vector<Event> uncommitted_events_{};
    };
} // skizzay::simple::cqrs

#endif //AGGREGATE_ROOT_H
