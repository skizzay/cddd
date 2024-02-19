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
    struct project_removed_task : skizzay::simple::cqrs::event_base<project_removed_task> {
        static constexpr std::string_view event_type_name = "project_removed_task";

        skizzay::simple::cqrs::uuid task_id = {};
        bool is_completed_task = false;

        [[nodiscard]] skizzay::simple::cqrs::uuid const &project_id() const noexcept {
            return stream_id;
        }

        friend std::ostream &operator<<(std::ostream &os, project_removed_task const &instance) {
            return os << R"({""stream_id":")" << instance.stream_id
                   << R"(","sequence_number":)" << instance.sequence_number
                   << R"(,"timestamp":)" << instance.timestamp
                   << R"(,"task_id":")" << instance.task_id
                   << R"(","is_completed_task":)" << std::boolalpha << instance.is_completed_task << "}";
        }

        template<typename D, std::endian E>
        friend skizzay::s11n::binary_writer<D, E> &operator<<(skizzay::s11n::binary_writer<D, E> &writer,
                                                              project_removed_task const &instance
                                                              ) {
            return writer << instance.stream_id << instance.sequence_number << instance.timestamp << instance.task_id <<
                   instance.is_completed_task;
        }

        template<typename D, std::endian E>
        friend skizzay::s11n::binary_reader<D, E> &operator>>(skizzay::s11n::binary_reader<D, E> &reader,
                                                              project_removed_task &instance
                                                              ) {
            return reader >> instance.stream_id >> instance.sequence_number >> instance.timestamp >> instance.task_id >>
                   instance.is_completed_task;
        }
    };
} // gtd
