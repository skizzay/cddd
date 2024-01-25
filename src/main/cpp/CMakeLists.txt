find_package(range-v3 CONFIG REQUIRED)

add_library(cddd INTERFACE)
target_include_directories(cddd INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/main/cpp>
  $<INSTALL_INTERFACE:include>
)
target_sources(cddd INTERFACE
  skizzay/cddd/aggregate_root.h
  skizzay/cddd/boolean.h
  skizzay/cddd/commit_failed.h
  skizzay/cddd/concurrent_repository.h
  skizzay/cddd/dereference.h
  skizzay/cddd/dictionary.h
  skizzay/cddd/domain_event.h
  skizzay/cddd/domain_event_sequence.h
  skizzay/cddd/domain_event_visitor.h
  skizzay/cddd/domain_event_wrapper.h
  skizzay/cddd/event_sourced_aggregate_repository.h
  skizzay/cddd/event_sourced.h
  skizzay/cddd/event_store.h
  skizzay/cddd/event_stream_buffer.h
  skizzay/cddd/event_stream.h
  skizzay/cddd/factory.h
  skizzay/cddd/history_load_failed.h
  skizzay/cddd/identifier.h
  skizzay/cddd/key_not_found.h
  skizzay/cddd/narrow_cast.h
  skizzay/cddd/nonvoid.h
  skizzay/cddd/nullable.h
  skizzay/cddd/optimistic_concurrency_collision.h
  skizzay/cddd/repository.h
  skizzay/cddd/sourced_aggregate_factory.h
  skizzay/cddd/timestamp.h
  skizzay/cddd/value.h
  skizzay/cddd/version.h
  skizzay/cddd/views.h
)
target_link_libraries(cddd INTERFACE range-v3::range-v3)

add_subdirectory(skizzay/cddd/in_memory)
if (CDDD_WITH_DYNAMODB)
add_subdirectory(skizzay/cddd/dynamodb)
endif()