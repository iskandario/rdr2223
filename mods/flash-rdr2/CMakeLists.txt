cmake_minimum_required(VERSION 3.21)
project(FlashRDR2 VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
  message(FATAL_ERROR "FlashRDR2 is Windows x64 only")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Only Windows x64 is supported")
endif()

set(RDR2_SDK "" CACHE PATH "Directory containing SDK/inc and SDK/lib")

if(NOT EXISTS "${RDR2_SDK}/inc/natives.h"
   OR NOT EXISTS "${RDR2_SDK}/lib/ScriptHookRDR2.lib")
  message(FATAL_ERROR "Set RDR2_SDK to the ScriptHookRDR2 SDK directory")
endif()

add_library(FlashRDR2 SHARED src/Plugin.cpp)

target_include_directories(FlashRDR2 SYSTEM PRIVATE "${RDR2_SDK}/inc")
target_link_libraries(FlashRDR2 PRIVATE "${RDR2_SDK}/lib/ScriptHookRDR2.lib")
target_compile_options(FlashRDR2 PRIVATE /W3 /utf-8)

set_target_properties(
  FlashRDR2 PROPERTIES
  PREFIX ""
  SUFFIX ".asi"
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
