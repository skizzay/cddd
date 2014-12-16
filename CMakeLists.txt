cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
add_definitions(-std=c++1y)
add_definitions(-Wall)
add_definitions(-Wextra)
add_definitions(-Werror)

option(GCC_LINK_TIME_OPTIMIZATION "Enable GCC Link Time Optimization (LTO)" OFF)
set(CMAKE_AR gcc-ar)
#if (GCC_LINK_TIME_OPTIMIZATION)
    #add_definitions(-flto=jobserver)
    #set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto=jobserver -fwhole-program")
    #set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -flto=jobserver")
    #set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} -flto=jobserver")
#endif()

project(cddd)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/sequence/include)

add_subdirectory(cqrs)
add_subdirectory(messaging)
add_subdirectory(utils)
add_subdirectory(application/unit_tests)
