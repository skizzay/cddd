//
// Created by andrew on 2/16/24.
//

#pragma once

#include <bit>
#include <ostream>
#include <skizzay/cqrs/event.h>
#include <skizzay/s11n/binary_reader.h>
#include <skizzay/s11n/binary_writer.h>
#include "business/io_utils.h"

namespace gtd {
    enum class project_completion_reason : std::uint8_t {
        unknown,
        all_tasks_completed,
        cancelled
    };

    inline std::ostream &operator<<(std::ostream &os, project_completion_reason reason) {
        switch (reason) {
            case project_completion_reason::unknown:
                return os << "unknown";
            case project_completion_reason::all_tasks_completed:
                return os << "all tasks completed";
            case project_completion_reason::cancelled:
                return os << "cancelled";
            default:
                std::unreachable();
        }
    }

    struct project_completed : skizzay::simple::cqrs::event_base<project_completed> {
        constexpr static std::string_view event_type_name = "project_completed";

        project_completion_reason reason{};

        [[nodiscard]] skizzay::simple::cqrs::uuid const &project_id() const noexcept {
            return stream_id;
        }

        friend std::ostream &operator<<(std::ostream &os, project_completed const &instance) {
            return os << R"({"stream_id":")" << instance.stream_id
                   << R"(","sequence_number":)" << instance.sequence_number
                   << R"(,"timestamp":)" << instance.timestamp
                   << R"(,"reason":")" << instance.reason << R"("})";
        }

        template<typename D, std::endian E>
        friend skizzay::s11n::binary_writer<D, E> &operator<<(skizzay::s11n::binary_writer<D, E> &writer,
                                                              project_completed const &instance
        ) {
            return writer << instance.stream_id.bytes() << instance.sequence_number
                   << instance.timestamp << instance.reason;
        }

        template<typename D, std::endian E>
        friend skizzay::s11n::binary_reader<D, E> &operator>>(skizzay::s11n::binary_reader<D, E> &reader,
                                                              project_completed &instance
        ) {
            return reader >> instance.stream_id >> instance.sequence_number >> instance.timestamp >> instance.reason;
        }
    };
} // gtd
