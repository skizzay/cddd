//
// Created by andrew on 1/19/24.
//

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H
#include <utility>
#include <variant>

#include "event.h"
#include "state.h"


namespace skizzay::simple::cqrs {
    template<state... States>
    class state_machine {
    public:
        template<state S>
        requires (std::same_as<S, States> || ...)
        [[nodiscard]] constexpr bool is_state() const noexcept {
            return std::holds_alternative<S>(state_);
        }

        template<typename F>
        requires (std::is_invocable_v<F, States const &> && ...)
        constexpr decltype(auto) query(F &&f) const noexcept((std::is_nothrow_invocable_v<F, States const &> && ...)) {
            return std::visit(std::forward<F>(f), state_);
        }

        template<event... Events>
        constexpr void on_event(std::variant<Events...> const &evt) {
            state_ = std::visit([&](state auto &&current, event auto const &e) -> std::variant<States...> {
                return current.on_event(e);
            }, std::move(state_), evt);
        }

        constexpr void on_event(event auto const &evt) {
            state_ = std::visit([&evt](state auto &&current) -> std::variant<States...> {
                return current.on_event(evt);
            }, std::move(state_));
        }

    private:
        std::variant<States...> state_;
    };
} // skizzay

#endif //STATE_MACHINE_H
