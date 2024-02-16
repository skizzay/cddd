add_library(file_journal INTERFACE
        main/skizzay/file/event_stream.h
        main/skizzay/file/event_source.h
        main/skizzay/file/event_log.h
        main/skizzay/file/event_log_provider.h
)

target_link_libraries(file_journal INTERFACE
        simple_cqrs
        simple_s11n
)

target_include_directories(simple_cqrs INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/file_journal/main>
        $<INSTALL_INTERFACE:include>
)

if (BUILD_TESTING)
    find_package(Catch2 REQUIRED)
    include(Catch)

    add_executable(file_journal_test
            test/file/event_stream.t.cpp
            test/file/event_log.t.cpp
            test/file/event_source.t.cpp
    )
    set_property(TARGET file_journal_test PROPERTY CXX_STANDARD 23)
    target_link_libraries(file_journal_test
            Catch2::Catch2WithMain
            file_journal
    )
    catch_discover_tests(file_journal_test)
endif ()