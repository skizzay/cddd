cmake_minimum_required(VERSION 3.13.0)
project(cddd VERSION 0.0.1)

option(BUILD_TESTS ON)
option(SUPPORT_KAFKA ON)

SET(CMAKE_EXPORT_COMPILE_COMMANDS 1)
if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

add_subdirectory(src/main/cpp)

# if (BUILD_TESTS)
  add_subdirectory(src/test/cpp)
# endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
