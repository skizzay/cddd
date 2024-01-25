//
// Created by andrew on 1/22/24.
//

#ifndef EVENT_STORE_H
#define EVENT_STORE_H
#include <cstddef>
#include "event.h"

namespace skizzay::simple::cqrs {
    class uuid;

    namespace event_source_details_ {
        event_range auto get_events(...) = delete;

        struct get_events_fn final {
            template<typename T>
            requires requires (T &&t, uuid const &id, std::size_t const b, std::size_t const e) {
                { t.get_events(id, b, e) } -> event_range;
            }
            constexpr event_range auto operator()(T &&t, uuid const &id, std::size_t const b, std::size_t const e) const {
                return std::forward<T>(t).get_events(id, b, e);
            }

            template<typename T>
            requires requires (T &&t, uuid const &id, std::size_t const b, std::size_t const e) {
                { get_events(t, id, b, e) } -> event_range;
            }
            constexpr event_range auto operator()(T &&t, uuid const &id, std::size_t const b, std::size_t const e) const {
                return get_events(std::forward<T>(t), id, b, e);
            }

            template<typename T>
            constexpr event_range auto operator()(T &&t, uuid const &id, std::size_t const b) const {
                if constexpr (requires {
                    { t.get_events(id, b) } -> event_range;
                }) {
                    return std::forward<T>(t).get_events(id, b);
                }
                else if constexpr (requires {
                    { get_events(t, id, b) } -> event_range;
                }) {
                    return get_events(std::forward<T>(t), id, b);
                }
                else {
                    return (*this)(std::forward<T>(t), id, b, std::numeric_limits<std::size_t>::max());
                }
            }

            template<typename T>
            constexpr event_range auto operator()(T &&t, uuid const &id) const {
                if constexpr (requires {
                    { t.get_events(id) } -> event_range;
                }) {
                    return std::forward<T>(t).get_events(id);
                }
                else if constexpr (requires {
                    { get_events(t, id) } -> event_range;
                }) {
                    return get_events(std::forward<T>(t), id);
                }
                else {
                    return (*this)(std::forward<T>(t), id, 1);
                }
            }
        };
    }

    inline namespace event_source_cpo {
        inline constexpr event_source_details_::get_events_fn get_events{};
    }
} // namespace skizzay::simple::cqrs

#endif //EVENT_STORE_H
