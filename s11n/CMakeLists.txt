add_library(simple_s11n INTERFACE
        main/skizzay/s11n/encode.h
        main/skizzay/s11n/file_deser.h
        main/skizzay/s11n/seek_origin.h
        main/skizzay/s11n/sink.h
        main/skizzay/s11n/source.h
        main/skizzay/s11n/buffer_reader.h
        main/skizzay/s11n/decode.h
        main/skizzay/s11n/end_of_file_error.h
        main/skizzay/s11n/memory_buffer.h
        main/skizzay/s11n/memmap_handle.h
        main/skizzay/s11n/file_descriptor.h
        main/skizzay/s11n/throw_system_error.h
        main/skizzay/s11n/buffer_position.h
)
target_include_directories(simple_s11n INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/s11n/main>
        $<INSTALL_INTERFACE:include>
)

if (CDDD_BUILD_TESTS)
    find_package(Catch2 REQUIRED)
    include(CTest)
    include(Catch)

    add_executable(simple_s11n_test
            test/s11n/encode.t.cpp
            test/s11n/file_deser.t.cpp
            test/s11n/buffer_reader.t.cpp
            test/s11n/memmap_handle.t.cpp
    )
    set_property(TARGET simple_s11n_test PROPERTY CXX_STANDARD 23)
    target_link_libraries(simple_s11n_test
            Catch2::Catch2WithMain
            simple_s11n
    )
    catch_discover_tests(simple_s11n_test)
endif ()