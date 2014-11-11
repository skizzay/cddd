cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
add_definitions(-std=c++1y)
add_definitions(-Wall)
add_definitions(-Wextra)
add_definitions(-Werror)

project(cddd)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/sequence/include)

add_subdirectory(cqrs)
add_subdirectory(application/unit_tests)
