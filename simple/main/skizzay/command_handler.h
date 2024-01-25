//
// Created by andrew on 1/22/24.
//

#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "command.h"
#include "event.h"
#include "event_stream.h"
#include <type_traits>
#include <utility>

namespace skizzay::simple::cqrs {

    template<typename AggregateRootFactory, typename EventStream>
    class command_handler {
    public:
        explicit command_handler(AggregateRootFactory &&ar_factory, EventStream &&event_stream
        ) noexcept(
            std::is_nothrow_move_constructible_v<AggregateRootFactory> &&
            std::is_nothrow_move_constructible_v<EventStream>)
            : ar_factory_{std::forward<AggregateRootFactory>(ar_factory)},
              event_stream_{std::forward<EventStream>(event_stream)} {
        }

        template<typename AggregateRoot=void>
        constexpr void handle(command auto const &cmd) {
            uuid const &id = command_aggregate_id(cmd);
            if constexpr (std::same_as<AggregateRoot, void>) {
                auto ar = ar_factory_.create(id);
                ar.handle_command(cmd);
                put_events(event_stream_, id, std::move(ar).commit());
            } else {
                auto ar = ar_factory_.template create<AggregateRoot>(id);
                ar.handle_command(cmd);
                put_events(event_stream_, id, std::move(ar).commit());
            }
        }

    private:
        AggregateRootFactory ar_factory_;
        EventStream event_stream_;
    };

} // skizzay::simple::cqrs

#endif //COMMAND_HANDLER_H
