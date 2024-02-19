add_library(simple_s11n INTERFACE
        main/skizzay/s11n/encode.h
        main/skizzay/s11n/file.h
        main/skizzay/s11n/seek_origin.h
        main/skizzay/s11n/decode.h
        main/skizzay/s11n/memory_buffer.h
        main/skizzay/s11n/memory_map.h
        main/skizzay/s11n/throw_system_error.h
        main/skizzay/s11n/buffer_position.h
        main/skizzay/s11n/binary_writer.h
        main/skizzay/s11n/range_of_bytes.h
        main/skizzay/s11n/binary_reader.h
        main/skizzay/s11n/io_device.h
        main/skizzay/s11n/boolean.h
        main/skizzay/s11n/write_transaction.h
)
set_property(TARGET simple_s11n PROPERTY CXX_STANDARD 23)
target_include_directories(simple_s11n INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/s11n/main>
        $<INSTALL_INTERFACE:include>
)

if (BUILD_TESTING)
    find_package(Catch2 REQUIRED)
    include(Catch)

    add_executable(simple_s11n_test
            test/s11n/encode.t.cpp
            test/s11n/file.t.cpp
            test/s11n/memory_map.t.cpp
            test/s11n/memory_buffer.t.cpp
            test/s11n/binary_serialization.t.cpp
    )
    set_property(TARGET simple_s11n_test PROPERTY CXX_STANDARD 23)
    target_link_libraries(simple_s11n_test
            Catch2::Catch2WithMain
            simple_s11n
    )
    catch_discover_tests(simple_s11n_test)
endif ()