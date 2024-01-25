//
// Created by andrew on 1/18/24.
//

#ifndef EVENT_H
#define EVENT_H

#include <cstddef>
#include <variant>

#include "time_point.h"
#include "uuid.h"

namespace skizzay::simple::cqrs {
    template<typename E, time_point Timestamp=std::chrono::system_clock::time_point>
        requires std::is_class_v<E> && std::is_same_v<E, std::remove_cvref_t<E> >
    struct event_base {
        uuid stream_id{};
        std::size_t sequence_number{};
        Timestamp timestamp{};

        constexpr event_base() noexcept = default;
        constexpr explicit event_base(uuid const &id) noexcept
            : stream_id{id} {
        }
    };

    namespace event_details_ {
        template<typename E>
        concept of_event_base = std::derived_from<E, event_base<E>>;

        struct event_stream_id_fn final {
            constexpr uuid const &operator()(of_event_base auto const &event) const noexcept {
                return event.stream_id;
            }

            template<of_event_base... Es>
            constexpr uuid const &operator()(std::variant<Es...> const &event) const noexcept {
                return std::visit(*this, event);
            }
        };

        struct event_sequence_number_fn final {
            constexpr std::size_t operator()(of_event_base auto const &event) const noexcept {
                return event.sequence_number;
            }

            template<of_event_base... Es>
            constexpr std::size_t operator()(std::variant<Es...> const &event) const noexcept {
                return std::visit(*this, event);
            }
        };

        struct event_timestamp_fn final {
            constexpr auto const &operator()(of_event_base auto const &event) const noexcept {
                return event.timestamp;
            }

            template<of_event_base... Es>
            constexpr auto const &operator()(std::variant<Es...> const &event) const noexcept {
                return std::visit(*this, event);
            }
        };

        void on_event(auto &, auto const &) = delete;

        struct on_event_fn final {
            template<typename Handler, of_event_base Event>
            requires requires(Handler &handler, Event const &event) {
                { handler.on_event(event) };
            }
            constexpr decltype(auto) operator()(Handler &handler, Event const &event) const noexcept(noexcept(handler.on_event(event))) {
                return handler.on_event(event);
            }

            template<typename Handler, of_event_base Event>
            requires requires(Handler &handler, Event const &event) {
                { on_event(handler, event) };
            }
            constexpr decltype(auto) operator()(Handler &handler, Event const &event) const noexcept(noexcept(on_event(handler, event))) {
                return on_event(handler, event);
            }

            template<typename Handler, typename... Event>
            requires (std::invocable<on_event_fn const, Handler &, Event const &> && ...)
            constexpr decltype(auto) operator()(Handler &handler, std::variant<Event...> const &event) const noexcept((std::is_nothrow_invocable_v<on_event_fn const, Handler &, Event> && ...)) {
                return std::visit([this, &handler] (auto const &evt) {
                    return (*this)(handler, evt);
                }, event);
            }
        };
    }

    inline namespace event_fn_ {
        constexpr event_details_::event_stream_id_fn event_stream_id{};
        constexpr event_details_::event_sequence_number_fn event_sequence_number{};
        constexpr event_details_::event_timestamp_fn event_timestamp{};
        constexpr event_details_::on_event_fn on_event{};
    }

    template<typename E>
    struct is_event : std::conjunction<std::is_invocable<decltype(event_stream_id), E>, std::is_invocable<decltype(event_sequence_number), E>, std::is_invocable<decltype(event_timestamp), E>> {
    };

    template<typename E>
    constexpr inline bool is_event_v = is_event<E>::value;

    template<typename E>
    concept event = is_event_v<E>;

    template<typename>
    struct is_event_range : std::false_type {
    };

    template<std::ranges::range E>
    struct is_event_range<E> : is_event<std::ranges::range_value_t<E>> {
    };

    template<typename E>
    constexpr inline bool is_event_range_v = is_event_range<E>::value;

    template<typename R>
    concept event_range = is_event_range_v<R>;
} // skizzay

#endif //EVENT_H
