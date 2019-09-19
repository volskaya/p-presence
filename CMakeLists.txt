cmake_minimum_required (VERSION 2.6)
project (presence)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  message ("Running on Windows")
else ()
  set(CMAKE_CXX_COMPILER g++)
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 ${CLANG_FLAGS}")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -s -Wall -Wfatal-errors")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/build)

SET(BUILD_SHARED_LIBS OFF)
SET(CMAKE_EXE_LINKER_FLAGS "-static")

set(DISCORD_DISABLE_IO_THREAD OFF)

configure_file(
  ${CMAKE_SOURCE_DIR}/configure.h.in 
  ${CMAKE_SOURCE_DIR}/configure.h)

include_directories(${CMAKE_SOURCE_DIR})
add_subdirectory(src)
