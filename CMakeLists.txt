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

# Earth agent.
add_executable(earth-agent "${PROJECT_SOURCE_DIR}/agents/earth.cpp")
add_dependencies(earth-agent engine)

# Mars agent.
add_executable(mars-agent "${PROJECT_SOURCE_DIR}/agents/mars.cpp")
add_dependencies(mars-agent engine)
