//
// Created by andrew on 1/22/24.
//

#ifndef EVENT_STREAM_H
#define EVENT_STREAM_H

#include <utility>
#include "event.h"

namespace skizzay::simple::cqrs {
    class uuid;

    namespace event_stream_details_ {
        void put_events(...) = delete;

        struct put_events_fn final {
            template<typename T, event_range Events>
            requires requires (T &&t, uuid const &id, Events &&events) {
                t.put_events(id, events);
            }
            constexpr decltype(auto) operator()(T &&t, uuid const &id, Events &&events) const {
                return std::forward<T>(t).put_events(id, std::forward<Events>(events));
            }

            template<typename T, event_range Events>
            requires requires (T &&t, uuid const &id, Events &&events) {
                put_events(t, id, events);
            }
            constexpr decltype(auto) operator()(T &&t, uuid const &id, Events &&events) const {
                return put_events(std::forward<T>(t), id, std::forward<Events>(events));
            }
        };
    }

    inline namespace event_stream_cpo {
        inline constexpr event_stream_details_::put_events_fn put_events{};
    }

} // skizzay::simple::cqrs

#endif //EVENT_STREAM_H
