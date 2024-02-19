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
    struct project_created : skizzay::simple::cqrs::event_base<project_created> {
        static constexpr std::string_view event_type_name = "project_created";

        skizzay::simple::cqrs::uuid owner_id;
        std::string name;

        [[nodiscard]] skizzay::simple::cqrs::uuid const &project_id() const noexcept {
            return stream_id;
        }

        friend std::ostream &operator<<(std::ostream &os, project_created const &instance) {
            return os << R"({""stream_id":")" << instance.stream_id
                   << R"(","sequence_number":)" << instance.sequence_number
                   << R"(,"timestamp":)" << instance.timestamp
                   << R"(,"owner_id":")" << instance.owner_id
                   << R"(","name":")" << instance.name << R"("})";
        }

        template<typename D, std::endian E>
        friend skizzay::s11n::binary_writer<D, E> &operator<<(skizzay::s11n::binary_writer<D, E> &writer,
                                                              project_created const &instance
        ) {
            return writer << instance.stream_id << instance.sequence_number <<
                   instance.timestamp << instance.owner_id << static_cast<std::uint16_t>(instance.name.size()) <<
                   instance.name;
        }

        template<typename D, std::endian E>
        friend skizzay::s11n::binary_reader<D, E> &operator>>(skizzay::s11n::binary_reader<D, E> &reader,
                                                              project_created &instance
        ) {
            reader >> instance.stream_id >> instance.sequence_number >> instance.timestamp >> instance.owner_id;
            std::uint16_t name_size{};
            reader >> name_size;
            instance.name.resize(name_size);
            return reader >> instance.name;
        }
    };
} // gtd
