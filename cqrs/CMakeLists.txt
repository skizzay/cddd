cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(cqrs)

add_library(cqrs
      artifact.h
      artifact_store.h
      commit.h
      copy_on_write.h
      cqrs.cpp
      event_dispatcher.h
      event.h
      event_source.h
      event_store.h
      event_stream.h
      exceptions.h
      object_id.cpp
      object_id.h
      repository.h
      source.h
      store.h
      stream.h
      traits.h
   )

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/sequence)
