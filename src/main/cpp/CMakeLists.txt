conan_cmake_configure(REQUIRES range-v3/0.12.0
                      SETTINGS cppstd=20
                      GENERATORS CMakeDeps CMakeToolchain)

conan_cmake_autodetect(settings)

conan_cmake_install(PATH_OR_REFERENCE .
                    BUILD missing
                    REMOTE conancenter
                    SETTINGS ${settings})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/src/main/cpp)
find_package(range-v3 CONFIG REQUIRED)

add_library(cddd INTERFACE)
target_include_directories(cddd INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/main/cpp>
  $<INSTALL_INTERFACE:include>
)
target_sources(cddd INTERFACE
  skizzay/cddd/boolean.h
  skizzay/cddd/domain_event.h
  skizzay/cddd/event_sourced.h
  skizzay/cddd/event_store.h
  skizzay/cddd/event_stream.h
  skizzay/cddd/identifier.h
  skizzay/cddd/optimistic_concurrency_collision.h
  skizzay/cddd/timestamp.h
  skizzay/cddd/version.h
)
target_link_libraries(cddd INTERFACE range-v3::range-v3)

if (CDDD_WITH_DYNAMODB)
add_subdirectory(skizzay/cddd/dynamodb)
endif()