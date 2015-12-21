SET(cqrs_test_sources domain_event_test.cpp)
ADD_EXECUTABLE(cqrs_tests ${cqrs_test_sources})
TARGET_LINK_LIBRARIES(cqrs_tests cddd_utils ${CONAN_LIBS})
ADD_TEST(cqrs_tests bin/cqrs_tests)
