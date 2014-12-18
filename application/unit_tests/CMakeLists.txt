cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(unit_tests)

find_package(Boost REQUIRED coroutine context system)
find_package(Threads REQUIRED)

add_executable(unit_tests
      main.cpp
      cqrs/artifact_store_test.cpp
      cqrs/artifact_test.cpp
      cqrs/copy_on_write_test.cpp
      cqrs/object_id_test.cpp
      messaging/dispatcher_test.cpp
   )

include_directories(/usr/src/gmock/include /usr/src/gmock/gtest/include)
target_link_libraries(unit_tests cqrs messaging utils ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} $ENV{HOME}/third-party/gmock/libgmock.a $ENV{HOME}/third-party/gmock/gtest/libgtest.a)
