//
// Created by andrew on 1/19/24.
//

#ifndef COMMAND_H
#define COMMAND_H
#include <chrono>

#include "time_point.h"
#include "uuid.h"


namespace skizzay::simple::cqrs {

    template<typename C, time_point Timestamp=std::chrono::system_clock::time_point>
        requires std::is_class_v<C> && std::is_same_v<C, std::remove_cvref_t<C> >
    struct command_base {
        uuid aggregate_id{};
        Timestamp timestamp{};
    };

    namespace command_details_ {
        struct command_aggregate_id_fn final {
            template<typename C>
            requires std::derived_from<C, command_base<C>>
            constexpr uuid const &operator()(C const &command) const noexcept {
                return command.aggregate_id;
            }
        };

        struct command_timestamp_fn final {
            template<typename C>
            requires std::derived_from<C, command_base<C>>
            constexpr auto const &operator()(C const &command) const noexcept {
                return command.timestamp;
            }
        };
    }

    inline namespace command_fn_ {
        inline constexpr command_details_::command_aggregate_id_fn command_aggregate_id{};
        inline constexpr command_details_::command_timestamp_fn command_timestamp{};
    }

    template<typename C>
    struct is_command : std::conjunction<
                std::is_invocable<decltype(command_aggregate_id), C>,
                std::is_invocable<decltype(command_timestamp), C>> {
    };

    template<typename C>
    inline constexpr bool is_command_v = is_command<C>::value;

    template<typename C>
    concept command = is_command_v<C>;
} // skizzay

#endif //COMMAND_H
