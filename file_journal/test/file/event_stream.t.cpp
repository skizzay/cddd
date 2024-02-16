//
// Created by andrew on 2/9/24.
//

#include <skizzay/file/event_stream.h>
#include <catch2/catch_all.hpp>
#include <filesystem>
#include <iostream>
#include <skizzay/cqrs/event.h>

#include <skizzay/cqrs/event_stream.h>

using namespace skizzay::simple::cqrs::file_journal;

namespace { inline namespace events {
    template<int N>
    struct test_event : skizzay::simple::cqrs::event_base<test_event<N> > {
        explicit test_event(skizzay::simple::cqrs::uuid const &id) noexcept
            : skizzay::simple::cqrs::event_base<test_event>{id} {
            this->sequence_number = N;
        }

        template<typename Writer>
        friend Writer &operator<<(Writer &writer, test_event const &event) {
            return writer << event.stream_id.bytes() << event.sequence_number << event.timestamp;
        }
    };

    using domain_event = std::variant<test_event<1>, test_event<2>, test_event<3>>;
}}

TEST_CASE("Events are put onto the stream and written to file", "[file_event_stream]") {
    auto const id = skizzay::simple::cqrs::uuid::v4();
    event_stream target{std::filesystem::temp_directory_path()};
    domain_event events[] = {test_event<1>{id}, test_event<2>{id}, test_event<3>{id}};
    skizzay::simple::cqrs::put_events(target, id, std::views::all(events));
    std::cout << "Event stream id: " << id.to_string() << std::endl;

    REQUIRE(std::filesystem::exists(std::filesystem::temp_directory_path() / (id.to_string() + ".indx")));
    REQUIRE(std::filesystem::exists(std::filesystem::temp_directory_path() / (id.to_string() + ".jrnl")));
}
