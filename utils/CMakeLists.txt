cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(utils)

add_library(utils
      function_traits.h
      type_id_generator.cpp
      type_id_generator.h
   )
