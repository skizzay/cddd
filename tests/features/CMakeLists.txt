SET(features_test_sources main.cpp
	                  step_definitions/artifact_steps.cpp
	                  step_definitions/command_steps.cpp)
ADD_EXECUTABLE(features_tests ${features_test_sources})
TARGET_LINK_LIBRARIES(features_tests cddd_utils ${CONAN_LIBS_CUCUMBER-CPP} ${CONAN_LIBS_KERCHOW} ${CONAN_LIBS_GTEST})
TARGET_INCLUDE_DIRECTORIES(features_tests PRIVATE ${CONAN_INCLUDE_DIRS_GTEST})
