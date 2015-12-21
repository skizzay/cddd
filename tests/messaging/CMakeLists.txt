SET(messaging_sources dispatcher_test.cpp)
ADD_EXECUTABLE(messaging_tests ${messaging_sources})
TARGET_LINK_LIBRARIES(messaging_tests cddd_utils ${CONAN_LIBS})
ADD_TEST(messaging_tests bin/messaging_tests)
