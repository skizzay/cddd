//
// Created by andrew on 2/17/24.
//

#pragma once

#include <bit>
#include <ostream>
#include <skizzay/cqrs/event.h>
#include <skizzay/s11n/binary_reader.h>
#include <skizzay/s11n/binary_writer.h>
#include "business/io_utils.h"

namespace gtd {
    enum class project_reactivation_reason : std::uint8_t {
        unknown,
        task_added,
        task_reactivated
    };

    inline std::ostream &operator<<(std::ostream &os, project_reactivation_reason reason) {
        switch (reason) {
            case project_reactivation_reason::unknown:
                return os << "unknown";
            case project_reactivation_reason::task_added:
                return os << "task added";
            case project_reactivation_reason::task_reactivated:
                return os << "task reactivated";
            default:
                std::unreachable();
        }
    }

    struct project_reactivated : skizzay::simple::cqrs::event_base<project_reactivated> {
        static constexpr std::string_view event_type_name = "project_reactivated";

        project_reactivation_reason reason{};

        [[nodiscard]] skizzay::simple::cqrs::uuid const &project_id() const noexcept {
            return stream_id;
        }

        friend std::ostream &operator<<(std::ostream &os, project_reactivated const &instance) {
            return os << R"({"stream_id":")" << instance.stream_id
                   << R"(","sequence_number":)" << instance.sequence_number
                   << R"(,"timestamp":)" << instance.timestamp
                   << R"(,"reason":")" << instance.reason << R"("})";
        }

        template<typename D, std::endian E>
        friend skizzay::s11n::binary_writer<D, E> &operator<<(skizzay::s11n::binary_writer<D, E> &writer,
                                                              project_reactivated const &instance
        ) {
            return writer << instance.stream_id << instance.sequence_number << instance.timestamp << instance.reason;
        }

        template<typename D, std::endian E>
        friend skizzay::s11n::binary_reader<D, E> &operator>>(skizzay::s11n::binary_reader<D, E> &reader,
                                                              project_reactivated &instance
        ) {
            return reader >> instance.stream_id >> instance.sequence_number >> instance.timestamp >> instance.reason;
        }
    };
} // gtd
