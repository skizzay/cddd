cmake_minimum_required(VERSION 3.13.0)
project(cddd VERSION 0.0.2)

SET(CMAKE_EXPORT_COMPILE_COMMANDS 1)
if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror -fconcepts-diagnostics-depth=2)
endif()

include(CTest)

add_subdirectory(s11n)
add_subdirectory(simple)
add_subdirectory(file_journal)


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)