//
// Created by andrew on 2/19/24.
//

#pragma once

#include <algorithm>
#include <bit>
#include <ostream>
#include <unordered_map>
#include <variant>

#include <skizzay/cqrs/event.h>
#include <skizzay/cqrs/uuid.h>
#include <skizzay/s11n/binary_reader.h>
#include <skizzay/s11n/binary_writer.h>

namespace gtd {
    template<typename, typename>
    struct message_reader_lookup_table;

    template<typename D, std::endian E, skizzay::simple::cqrs::event... Events>
    struct message_reader_lookup_table<skizzay::s11n::binary_reader<D, E>, std::variant<Events...> > {
    private:
        template<std::default_initializable Event>
        static constexpr auto read_event = [](skizzay::s11n::binary_reader<D, E> &reader,
                                              std::variant<Events...> &instance
        ) -> skizzay::s11n::binary_reader<D, E> &{
            Event event{};
            reader >> event;
            instance = std::move(event);
            return reader;
        };

        static constexpr std::unordered_map<std::string, skizzay::s11n::binary_reader<D, E> &(*)(
            skizzay::s11n::binary_reader<D, E> &, std::variant<Events...> &

        )> readers = {
            read_event<Events>...
        };

    public:
        static skizzay::s11n::binary_reader<D, E> &read(std::string const &name,
                                                        skizzay::s11n::binary_reader<D, E> &reader,
                                                        std::variant<Events...> &instance
        ) {
            return readers[name](reader, instance);
        }
    };

    template<typename Ch, typename Tr>
    std::basic_ostream<Ch, Tr> &operator<<(std::basic_ostream<Ch, Tr> &os, skizzay::simple::cqrs::uuid const &instance
    ) {
        return os << instance.to_string();
    }

    template<typename D, std::endian E>
    skizzay::s11n::binary_writer<D, E> &operator<<(skizzay::s11n::binary_writer<D, E> &writer,
                                                   skizzay::simple::cqrs::uuid const &instance
    ) {
        return writer << instance.bytes();
    }

    template<typename D, std::endian E>
    skizzay::s11n::binary_reader<D, E> &operator>>(skizzay::s11n::binary_reader<D, E> &reader,
                                                   skizzay::simple::cqrs::uuid &instance
    ) {
        std::array<std::byte, 16> buffer{};
        reader >> buffer;
        instance = skizzay::simple::cqrs::uuid::from_bytes(buffer);
        return reader;
    }

    template<typename Ch, typename Tr, skizzay::simple::cqrs::event... Events>
    std::ostream &operator<<(std::basic_ostream<Ch, Tr> &os, std::variant<Events...> const &instance) {
        return std::visit([&os]<typename T>(T const &event) -> std::ostream &{
            return os << R"({"type":")" << T::event_type_name << R"(","event":)" << event << R"(})";
        }, instance);
    }

    template<typename D, std::endian E, skizzay::simple::cqrs::event... Events>
    skizzay::s11n::binary_writer<D, E> &operator<<(skizzay::s11n::binary_writer<D, E> &writer,
                                                   std::variant<Events...> const &instance
    ) {
        return std::visit([&writer]<typename T>(T const &event) -> skizzay::s11n::binary_writer<D, E> &{
            return writer << static_cast<std::uint8_t>(T::event_type_name.size()) << T::event_type_name << event;
        }, instance);
    }

    template<typename D, std::endian E, skizzay::simple::cqrs::event... Events>
    skizzay::s11n::binary_reader<D, E> &operator>>(skizzay::s11n::binary_reader<D, E> &reader,
                                                   std::variant<Events...> &instance
    ) {
        std::uint8_t n{};
        reader >> n;
        std::string name{static_cast<std::size_t>(n), '\0'};
        reader >> name;
        return message_reader_lookup_table<decltype(reader), std::variant<Events...> >::read(name, reader, instance);
    }
} // gtd
