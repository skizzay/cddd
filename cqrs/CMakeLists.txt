cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(cqrs)

add_library(cqrs STATIC
      artifact.h
      artifact_store.h
      artifact_traits.h
      commit.h
      copy_on_write.h
      domain_event.h
      exceptions.h
      null_store.h
      object_id.h
      pointer_traits.h
      repository.h
      simple_artifact_factory.h
      source.h
      store.h
      stream.h
      traits.h
      object_id.cpp
   )
