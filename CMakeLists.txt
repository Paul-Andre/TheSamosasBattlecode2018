cmake_minimum_required(VERSION 3.9)

project(battlecode)

option(debug "Use debugging flag" ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

if (debug)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
else ()
  set(CMAKE_CXX_FLAGS "-O2")
endif()

include_directories(
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/include)

file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")
add_library(engine ${SOURCES})

add_executable(agent "${PROJECT_SOURCE_DIR}/agent/main.cpp")
add_dependencies(agent engine)
