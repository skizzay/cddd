conan_cmake_configure(REQUIRES catch2/3.3.2
                      SETTINGS cppstd=20
                      GENERATORS CMakeDeps CMakeToolchain)

conan_cmake_autodetect(settings)

conan_cmake_install(PATH_OR_REFERENCE .
                    BUILD missing
                    REMOTE conancenter
                    SETTINGS ${settings})

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/src/test/cpp)
find_package(Catch2 CONFIG REQUIRED)

add_executable(cddd_unit_tests)
target_include_directories(cddd_unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/src/main/cpp)

target_sources(cddd_unit_tests PRIVATE
  # skizzay/cddd/dynamodb_version_service.t.cpp
  # skizzay/cddd/dynamodb_event_dispatcher.t.cpp
  skizzay/cddd/dynamodb_event_stream.t.cpp
  skizzay/cddd/dynamodb_event_source.t.cpp
  skizzay/cddd/dynamodb_event_store.t.cpp
  skizzay/cddd/in_memory_event_stream.t.cpp
  skizzay/cddd/in_memory_event_source.t.cpp
  skizzay/cddd/in_memory_event_buffer.t.cpp
  skizzay/cddd/event_sourced_aggregate_repository.t.cpp
  skizzay/cddd/fakes.t.cpp
  skizzay/cddd/in_memory_event_store.t.cpp
  skizzay/cddd/concurrent_repository.t.cpp
)
target_compile_definitions(cddd_unit_tests PUBLIC AWS_CUSTOM_MEMORY_MANAGEMENT)
target_link_libraries(cddd_unit_tests PRIVATE cddd Catch2::Catch2 Catch2::Catch2WithMain cddd_dynamodb)
set_property(TARGET cddd_unit_tests PROPERTY CXX_STANDARD 20)



include(CTest)
include(Catch)
catch_discover_tests(cddd_unit_tests)


if(CMAKE_COMPILER_IS_GNUCXX)
  LIST(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/src/test/cmake")
  include(CodeCoverage)
  APPEND_COVERAGE_COMPILER_FLAGS()
endif()