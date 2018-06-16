cmake_minimum_required(VERSION 3.10)

project(pinned_vector)

add_library(
    pinned_vector
    src/pinned_vector.cpp
    include/pinned_vector.hpp)

target_include_directories(pinned_vector PUBLIC include)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

option(WITH_PINNED_VECTOR_BENCHMARKS OFF "Enable to include targets for pinned_vector benchmark applications.")
if(WITH_PINNED_VECTOR_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()
