add_library(simple_cqrs INTERFACE
        main/skizzay/cqrs/event.h
        main/skizzay/cqrs/uuid.h
        main/skizzay/cqrs/time_point.h
        main/skizzay/cqrs/simple_traits.h
        main/skizzay/cqrs/aggregate_root.h
        main/skizzay/cqrs/state.h
        main/skizzay/cqrs/command.h
        main/skizzay/cqrs/state_machine.h
        main/skizzay/cqrs/event_source.h
        main/skizzay/cqrs/aggregate_root_factory.h
        main/skizzay/cqrs/command_handler.h
        main/skizzay/cqrs/event_stream.h
        main/skizzay/cqrs/event_stream.h
        main/skizzay/cqrs/memory_event_store.h
        main/skizzay/cqrs/factory.h
        main/skizzay/cqrs/tag.h
)
target_include_directories(simple_cqrs INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/simple/main>
        $<INSTALL_INTERFACE:include>
)

if (CDDD_BUILD_TESTS)
    find_package(Catch2 REQUIRED)
    include(CTest)
    include(Catch)

    add_executable(simple_cqrs_test
            test/cqrs/uuid.t.cpp
            test/cqrs/time_point.t.cpp
            test/cqrs/event.t.cpp
            test/cqrs/command.t.cpp
            test/cqrs/state_machine.t.cpp
            test/cqrs/aggregate_root.t.cpp
            test/cqrs/memory_event_store.t.cpp
            test/cqrs/command_handler.t.cpp
            test/cqrs/aggregate_root_factory.t.cpp
    )
    set_property(TARGET simple_cqrs_test PROPERTY CXX_STANDARD 23)
    target_link_libraries(simple_cqrs_test
            Catch2::Catch2WithMain
            simple_cqrs
    )
    catch_discover_tests(simple_cqrs_test)
endif ()