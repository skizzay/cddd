//
// Created by andrew on 2/18/24.
//

#pragma once

#include <bit>
#include <ostream>
#include <skizzay/cqrs/event.h>
#include <skizzay/s11n/binary_reader.h>
#include <skizzay/s11n/binary_writer.h>
#include "business/io_utils.h"

namespace gtd {
    struct project_task_reactivated : skizzay::simple::cqrs::event_base<project_task_reactivated> {
        static constexpr std::string_view event_type_name = "project_task_reactivated";

        skizzay::simple::cqrs::uuid task_id = {};

        [[nodiscard]] skizzay::simple::cqrs::uuid const &project_id() const noexcept {
            return stream_id;
        }

        friend std::ostream &operator<<(std::ostream &os, project_task_reactivated const &instance) {
            return os << R"({""stream_id":")" << instance.stream_id
                   << R"(","sequence_number":)" << instance.sequence_number
                   << R"(,"timestamp":)" << instance.timestamp
                   << R"(,"task_id":")" << instance.task_id << R"("})";
        }

        template<typename D, std::endian E>
        friend skizzay::s11n::binary_writer<D, E> &operator<<(skizzay::s11n::binary_writer<D, E> &writer,
                                                              project_task_reactivated const &instance
        ) {
            return writer << instance.stream_id << instance.sequence_number << instance.timestamp << instance.task_id;
        }

        template<typename D, std::endian E>
        friend skizzay::s11n::binary_reader<D, E> &operator>>(skizzay::s11n::binary_reader<D, E> &reader,
                                                              project_task_reactivated &instance
        ) {
            return reader >> instance.stream_id >> instance.sequence_number >> instance.timestamp >> instance.task_id;
        }
    };
} // gtd
