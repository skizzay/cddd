find_package(Catch2 CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(aws-cpp-sdk-core REQUIRED)

add_executable(cddd_unit_tests)
target_include_directories(cddd_unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/src/main/cpp)

target_sources(cddd_unit_tests PRIVATE
  skizzay/cddd/dynamodb_version_service.t.cpp
  skizzay/cddd/dynamodb_event_stream.t.cpp
  skizzay/cddd/in_memory_event_stream.t.cpp
)
target_compile_definitions(cddd_unit_tests PUBLIC AWS_CUSTOM_MEMORY_MANAGEMENT)
target_link_libraries(cddd_unit_tests PRIVATE Catch2::Catch2 Catch2::Catch2WithMain cddd_dynamodb aws-cpp-sdk-core)
set_property(TARGET cddd_unit_tests PROPERTY CXX_STANDARD 20)

# add_executable(fsm_integration_tests)
# target_link_libraries(fsm_integration_tests PRIVATE Catch2::Catch2 Catch2::Catch2WithMain)
# set_property(TARGET fsm_integration_tests PROPERTY CXX_STANDARD 20)
# target_include_directories(fsm_integration_tests PRIVATE ${PROJECT_SOURCE_DIR}/src/main/cpp)
# target_sources(fsm_integration_tests PRIVATE
#   skizzay/fsm/integration/full_fsm.t.cpp
# )



  include(CTest)
  include(Catch)
  catch_discover_tests(cddd_unit_tests)
  # catch_discover_tests(cddd_integration_tests)


if(CMAKE_COMPILER_IS_GNUCXX)
  LIST(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/src/test/cmake")
  include(CodeCoverage)
  APPEND_COVERAGE_COMPILER_FLAGS()
  # set(COVERAGE_LCOV_EXCLUDES 'dir1/*' 'dir2/*') // this is optional if you want to exclude some directory from the report
  # SETUP_TARGET_FOR_COVERAGE_LCOV(NAME foo_coverage
  #                                EXECUTABLE foo
  #                               DEPENDENCIES foo)
endif()