add_library(cddd INTERFACE)
target_include_directories(cddd INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/main/cpp>
  $<INSTALL_INTERFACE:include>
)
target_sources(cddd INTERFACE
  skizzay/cddd/boolean.h
  skizzay/cddd/domain_even.h
  skizzay/cddd/event_sourced.h
  skizzay/cddd/event_store.h
  skizzay/cddd/event_stream.h
  skizzay/cddd/identifier.h
  skizzay/cddd/in_memory_event_store.h
  skizzay/cddd/optimistic_concurrency_collision.h
  skizzay/cddd/timestamp.h
  skizzay/cddd/version.h
)

if (SUPPORT_KAFKA)
  add_library(cddd_kafka INTERFACE)
  target_include_directories(cddd_kafka INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/main
    $<INSTALL_INTERFACE:include/kafka>
  )

  find_package(RdKafka CONFIG REQUIRED)
  target_link_libraries(cddd_kafka PUBLIC cddd PRIVATE RdKafka::rdkafka RdKafka::rdkafka++)
endif()