conan_cmake_configure(REQUIRES zlib/1.2.13 aws-sdk-cpp/1.9.234
                     OPTIONS "aws-sdk-cpp/*:dynamodb=True;aws-sdk-cpp/*:text-to-speech=False"
                     SETTINGS cppstd=20
                     GENERATORS CMakeDeps CMakeToolchain)

conan_cmake_autodetect(settings)

conan_cmake_install(PATH_OR_REFERENCE .
                  BUILD missing
                  REMOTE conancenter
                  SETTINGS ${settings})

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/src/main/cpp/skizzay/cddd/dynamodb)

find_package(AWSSDK REQUIRED COMPONENTS dynamodb)

set(CUSTOM_MEMORY_MANAGEMENT OFF)
add_library(cddd_dynamodb INTERFACE)
set_target_properties(cddd_dynamodb PROPERTIES
   CXX_STANDARD 20
   CXX_STANDARD_REQUIRED ON
   CXX_EXTENSIONS OFF
   LINKER_LANGUAGE CXX
)
target_sources(cddd_dynamodb INTERFACE
   dynamodb_attribute_value.h
   dynamodb_client.h
   dynamodb_clock.h
   dynamodb_event_log_config.h
   dynamodb_event_source.h
   dynamodb_event_store.h
   dynamodb_event_stream_buffer.h
   dynamodb_event_stream.h
   dynamodb_operation_failed_error.h
   dynamodb_version_validation_error.h
)
target_include_directories(cddd_dynamodb INTERFACE
   $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/main
   $<INSTALL_INTERFACE:include/skizzay/cddd/dynamodb>
)
target_link_libraries(cddd_dynamodb INTERFACE cddd aws-sdk-cpp::aws-sdk-cpp)