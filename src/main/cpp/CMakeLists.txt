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
  skizzay/cddd/in_memory_event_store.h
  skizzay/cddd/optimistic_concurrency_collision.h
  skizzay/cddd/timestamp.h
  skizzay/cddd/version.h
)
target_link_libraries(cddd INTERFACE range-v3 range-v3-meta range-v3::meta range-v3-concepts)

if (CDDD_WITH_KAFKA)
  find_package(RdKafka CONFIG REQUIRED)

  add_library(cddd_kafka)
  target_include_directories(cddd_kafka INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/main
    $<INSTALL_INTERFACE:include/skizzay/cddd/kafka>
  )
  target_link_libraries(cddd_kafka PUBLIC cddd PRIVATE RdKafka::rdkafka RdKafka::rdkafka++)
endif()

if (CDDD_WITH_DYNAMODB)

conan_cmake_configure(REQUIRES zlib/1.2.13 aws-sdk-cpp/1.9.234
                      OPTIONS "aws-sdk-cpp:dynamodb=True;aws-sdk-cpp:text-to-speech=False"
                      SETTINGS cppstd=20
                      GENERATORS cmake_find_package)

conan_cmake_autodetect(settings)

conan_cmake_install(PATH_OR_REFERENCE .
                    BUILD missing
                    REMOTE conancenter
                    SETTINGS ${settings})

  find_package(ZLIB REQUIRED)
  find_package(aws-cpp-sdk-dynamodb REQUIRED)

  set(CUSTOM_MEMORY_MANAGEMENT OFF)
  add_library(cddd_dynamodb)
  target_include_directories(cddd_dynamodb INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/main
    $<INSTALL_INTERFACE:include/skizzay/cddd/dynamodb>
  )
  target_link_libraries(cddd_dynamodb PRIVATE cddd aws-cpp-sdk-dynamodb)
endif()