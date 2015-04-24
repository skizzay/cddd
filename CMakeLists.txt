cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
add_compile_options(-std=c++1y)
add_compile_options(-Wall)
add_compile_options(-Wextra)
add_compile_options(-Werror)

option(GCC_LINK_TIME_OPTIMIZATION "Enable GCC Link Time Optimization (LTO)" ON)
if (GCC_LINK_TIME_OPTIMIZATION)
    add_compile_options(-flto=jobserver)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto=jobserver")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -flto=jobserver")
    set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} -flto=jobserver")
    set(CMAKE_CXX_ARCHIVE_CREATE "/usr/bin/gcc-ar cr <TARGET> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_APPEND "/usr/bin/gcc-ar r <TARGET> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_FINISH "/usr/bin/gcc-ranlib <TARGET>")
    #set(CMAKE_AR "/usr/bin/gcc-ar")
    #set(CMAKE_RANLIB "/usr/bin/gcc-ranlib")
endif()

project(cddd)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/sequence/include)

add_subdirectory(cqrs)
add_subdirectory(messaging)
add_subdirectory(utils)
add_subdirectory(tests)
