//
// Created by andrew on 1/24/24.
//

#pragma once

#include <filesystem>
#include <fstream>

#include <skizzay/cqrs/uuid.h>
#include <skizzay/cqrs/event.h>
#include <skizzay/s11n/binary_writer.h>
#include <skizzay/s11n/memory_map.h>
#include "event_log.h"
#include "event_log_provider.h"

namespace skizzay::simple::cqrs::file_journal {
    template<typename Clock=std::chrono::system_clock, std::endian E=std::endian::native>
        requires requires(Clock c) {
            { c.now() } -> time_point;
        }
    class event_stream {
    public:
        explicit event_stream(std::filesystem::path directory, Clock clock = {})
            : log_provider_(std::move(directory)),
              clock_(std::move(clock)) {
        }

        void put_events(uuid const &id, event_range auto events) {
            auto log = log_provider_.get(id);
            auto txn = log.begin_transaction();
            event_serializer serialize{id, log, clock_.now()};
            for (auto &event: events) {
                serialize(event);
            }
        }

    private:
        using time_point_t = decltype(std::declval<Clock>().now());

        event_log_provider<E> log_provider_;
        Clock clock_;

        class event_serializer final {
        public:
            explicit event_serializer(uuid const &id, event_log<s11n::memory_map, E> &log, time_point_t const timestamp)
                : id_{id},
                  log_{log},
                  timestamp_{timestamp},
                  sequence_number_{static_cast<std::uint64_t>(std::ranges::size(log_))} {
            }

            template<event Event>
            void operator()(Event &event) {
                event.stream_id = id_;
                event.timestamp = timestamp_;
                event.sequence_number = ++sequence_number_;
                log_.push(event);
            }

            template<event... Events>
            void operator()(std::variant<Events...> &event) {
                std::visit(*this, event);
            }

        private:
            uuid const &id_;
            event_log<s11n::memory_map, E> &log_;
            time_point_t const timestamp_;
            std::uint64_t sequence_number_;
        };
    };
} // skizzay
