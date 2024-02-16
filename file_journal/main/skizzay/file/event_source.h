//
// Created by andrew on 2/12/24.
//

#pragma once

#include <algorithm>
#include <filesystem>
#include <ranges>
#include <skizzay/cqrs/event.h>
#include <skizzay/cqrs/uuid.h>
#include "event_log_provider.h"

namespace skizzay::simple::cqrs::file_journal {
    template<event Event, std::endian E = std::endian::native>
    class event_source {
    public:
        using event_type = Event;

        explicit event_source(std::filesystem::path directory)
            : log_provider_{std::move(directory)} {
        }

        [[nodiscard]] std::ranges::range auto get_events(uuid const &id, std::uint64_t const b, std::uint64_t const e) {
            auto log = std::make_shared<event_log<s11n::memory_map, E>>(log_provider_.get(id));
            auto const log_size = static_cast<std::uint64_t>(std::ranges::size(*log));
            return std::views::iota(b - 1, std::min(e, log_size))
                   | std::views::transform([log=std::move(log)](std::int64_t const i) {
                       return (*log)[i];
                   })
                   | std::views::transform([this](std::span<std::byte const> const bytes) {
                       return s11n::buffer_source{bytes};
                   })
                   | std::views::transform([this](s11n::buffer_source s) {
                       s11n::binary_reader<s11n::buffer_source, E> reader{s};
                       event_type event{};
                       reader >> event;
                       return event;
                   });
        }

    private:
        event_log_provider<E> log_provider_;
    };
} // skizzay::simple::cqrs::file_journal
