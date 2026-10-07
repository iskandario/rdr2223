cmake_minimum_required(VERSION 3.20)
project(TrafficCarsRDR2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED RDR2_SDK)
  message(FATAL_ERROR "RDR2_SDK must point to the SDK directory")
endif()

add_library(TrafficCarsRDR2 SHARED src/Plugin.cpp)

target_include_directories(TrafficCarsRDR2 PRIVATE
  "${RDR2_SDK}/inc"
)

target_link_directories(TrafficCarsRDR2 PRIVATE
  "${RDR2_SDK}/lib"
)

target_link_libraries(TrafficCarsRDR2 PRIVATE ScriptHookRDR2)

set_target_properties(TrafficCarsRDR2 PROPERTIES
  PREFIX ""
  SUFFIX ".asi"
)
