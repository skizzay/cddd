//
// Created by andrew on 2/14/24.
//

#pragma once

#include <bit>
#include <filesystem>
#include <skizzay/cqrs/uuid.h>
#include <skizzay/s11n/memory_map.h>
#include "event_log.h"

namespace skizzay::simple::cqrs::file_journal {
    template<std::endian E>
    class event_log_provider {
    public:
        explicit event_log_provider(std::filesystem::path directory)
            : directory_{std::move(directory)} {
        }

        event_log<s11n::memory_map, E> get(uuid const &id) {
            // TODO: Leverage LRU cache to keep the logs open
            auto filename_base = directory_ / id.to_string();
            return event_log{
                open_memory_map(filename_base.replace_extension(".jrnl")),
                open_memory_map(filename_base.replace_extension(".indx"))
            };
        }

    private:
        std::filesystem::path directory_;

        [[nodiscard]] static s11n::memory_map open_memory_map(std::filesystem::path const &path) {
            return s11n::memory_map::open_shared(path);
        }
    };
} // skizzay
