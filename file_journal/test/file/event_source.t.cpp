//
// Created by andrew on 2/14/24.
//


#include <skizzay/file/event_source.h>
#include <catch2/catch_all.hpp>
#include <skizzay/file/event_stream.h>
#include <skizzay/cqrs/event_stream.h>
#include <iostream>
#include <skizzay/cqrs/event_source.h>

using namespace skizzay::simple::cqrs::file_journal;
using namespace skizzay;

namespace { inline namespace events {
    template<int N>
    struct test_event : simple::cqrs::event_base<test_event<N> > {
        explicit test_event(simple::cqrs::uuid const &id={}, std::chrono::system_clock::time_point const timestamp={}) noexcept
            : simple::cqrs::event_base<test_event>{id} {
            this->sequence_number = N;
            this->timestamp = timestamp;
        }

        template<typename Writer>
        friend Writer &operator<<(Writer &writer, test_event const &event) {
            return writer << event.stream_id.bytes() << event.sequence_number << event.timestamp;
        }
    };

    using domain_event = std::variant<test_event<1>, test_event<2>, test_event<3> >;

    template<s11n::source S, std::endian E>
    s11n::binary_reader<S, E> &operator>>(s11n::binary_reader<S, E> &reader, domain_event &event) {
        std::array<std::byte, 16> id{};
        std::uint64_t sequence_number{};
        std::chrono::system_clock::time_point timestamp{};
        reader >> id >> sequence_number >> timestamp;

        switch (sequence_number) {
            case 1:
                event = test_event<1>{simple::cqrs::uuid::from_bytes(id), timestamp};
                break;
            case 2:
                event = test_event<2>{simple::cqrs::uuid::from_bytes(id), timestamp};
                break;
            case 3:
                event = test_event<3>{simple::cqrs::uuid::from_bytes(id), timestamp};
                break;
            default:
                throw std::runtime_error{"Invalid sequence number"};
        }
        return reader;
    }
}}

TEST_CASE("Events are put onto the stream and read from it", "[file_event_stream]") {
    auto const id = simple::cqrs::uuid::v4();
    auto directory = std::filesystem::temp_directory_path();
        domain_event events[] = {test_event<1>{id}, test_event<2>{id}, test_event<3>{id}};

    {
        event_stream stream{directory};
        simple::cqrs::put_events(stream, id, std::views::all(events));
        std::cout << "Event stream id: " << id.to_string() << std::endl;

        REQUIRE(std::filesystem::exists(std::filesystem::temp_directory_path() / (id.to_string() + ".indx")));
        REQUIRE(std::filesystem::exists(std::filesystem::temp_directory_path() / (id.to_string() + ".jrnl")));

        SECTION("read with event log open") {
            event_source<domain_event> target{directory};

            for (auto const [sequence_number, e]: std::views::enumerate(simple::cqrs::get_events(target, id))) {
                std::visit([id, sequence_number](auto const &event) {
                    REQUIRE(event.stream_id == id);
                    REQUIRE(event.sequence_number == static_cast<std::uint64_t>(sequence_number + 1));
                }, e);
            }

            REQUIRE(std::ranges::size(events) == std::ranges::size(target.get_events(id, 1, 3)));
        }
    }

    SECTION("read after event log closed") {
        event_source<domain_event> target{directory};

        for (auto const [sequence_number, e]: std::views::enumerate(simple::cqrs::get_events(target, id))) {
            std::visit([id, sequence_number](auto const &event) {
                REQUIRE(event.stream_id == id);
                REQUIRE(event.sequence_number == static_cast<std::uint64_t>(sequence_number + 1));
            }, e);
        }

        REQUIRE(std::ranges::size(events) == std::ranges::size(target.get_events(id, 1, 3)));
    }
}
