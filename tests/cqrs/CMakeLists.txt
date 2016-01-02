SET(cqrs_test_sources artifact_test.cpp
	              artifact_store_test.cpp
		      command_handler_test.cpp
		      domain_event_test.cpp
		      domain_event_stream_store_test.cpp
		      memento_store_test.cpp
		      stream_test.cpp)
ADD_EXECUTABLE(cqrs_tests ${cqrs_test_sources})
TARGET_LINK_LIBRARIES(cqrs_tests cddd_utils ${CONAN_LIBS_KERCHOW} ${CONAN_LIBS_GTEST})
TARGET_INCLUDE_DIRECTORIES(cqrs_tests PRIVATE ${CONAN_INCLUDE_DIRS_GTEST})
