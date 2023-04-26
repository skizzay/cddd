add_library(cddd_in_memory INTERFACE)
set_target_properties(cddd_in_memory PROPERTIES
   CXX_STANDARD 20
   CXX_STANDARD_REQUIRED ON
   CXX_EXTENSIONS OFF
   LINKER_LANGUAGE CXX
)
target_sources(cddd_in_memory INTERFACE
   in_memory_event_buffer.h
   in_memory_event_source.h
   in_memory_event_store.h
   in_memory_event_stream.h
)
target_include_directories(cddd_in_memory INTERFACE
   $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/main
   $<INSTALL_INTERFACE:include/skizzay/cddd/in_memory>
)
target_link_libraries(cddd_in_memory INTERFACE cddd)