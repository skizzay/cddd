SET(messaging_sources dispatcher_test.cpp)
ADD_EXECUTABLE(messaging_tests ${messaging_sources})
TARGET_LINK_LIBRARIES(messaging_tests cddd_utils ${CONAN_LIBS_GTEST})
TARGET_INCLUDE_DIRECTORIES(messaging_tests PRIVATE ${CONAN_INCLUDE_DIRS_GTEST})
