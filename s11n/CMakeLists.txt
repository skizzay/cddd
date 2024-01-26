add_library(simple_s11n INTERFACE
        main/skizzay/s11n/encode.h
        main/skizzay/s11n/file_deser.h
        main/skizzay/s11n/seek_origin.h
        main/skizzay/s11n/sink.h
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
    )
    set_property(TARGET simple_s11n_test PROPERTY CXX_STANDARD 23)
    target_link_libraries(simple_s11n_test
            Catch2::Catch2WithMain
            simple_s11n
    )
    catch_discover_tests(simple_s11n_test)
endif ()