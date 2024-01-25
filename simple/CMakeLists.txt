add_library(simple_cqrs INTERFACE
        main/skizzay/event.h
        main/skizzay/uuid.h
        main/skizzay/time_point.h
        main/skizzay/simple_traits.h
        main/skizzay/aggregate_root.h
        main/skizzay/state.h
        main/skizzay/command.h
        main/skizzay/state_machine.h
        main/skizzay/event_source.h
        main/skizzay/aggregate_root_factory.h
        main/skizzay/command_handler.h
        main/skizzay/event_stream.h
        main/skizzay/event_stream.h
        main/skizzay/memory_event_store.h
        main/skizzay/factory.h
        main/skizzay/tag.h
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
            test/uuid.t.cpp
            test/time_point.t.cpp
            test/event.t.cpp
            test/command.t.cpp
            test/state_machine.t.cpp
            test/aggregate_root.t.cpp
            test/memory_event_store.t.cpp
            test/command_handler.t.cpp
            test/aggregate_root_factory.t.cpp
    )
    set_property(TARGET simple_cqrs_test PROPERTY CXX_STANDARD 23)
    target_link_libraries(simple_cqrs_test
            Catch2::Catch2WithMain
            simple_cqrs
    )
    catch_discover_tests(simple_cqrs_test)
endif ()