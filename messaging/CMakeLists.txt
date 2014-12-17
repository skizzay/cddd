cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(messaging)

add_library(messaging
      dispatcher.h
      dispatching_error_policies.h
      messaging.cpp
      message.h
      traits.h
   )
