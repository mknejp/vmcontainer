cmake_minimum_required(VERSION 3.10)

project(pinned_vector)

add_library(
    pinned_vector
    src/pinned_vector.cpp
    include/pinned_vector.hpp)

target_include_directories(pinned_vector PUBLIC include)
target_compile_features(pinned_vector PUBLIC cxx_std_11)

include(CTest)
option(PINNED_VECTOR_TESTS "Build pinned_vector tests" off)
if(BUILD_TESTING AND PINNED_VECTOR_TESTS)
    add_subdirectory(tests)
endif()
