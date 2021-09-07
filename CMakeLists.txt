# This file is intended for use by library developers/maintainers, as well as users and package managers.
# It declares targets for unit tests and benchmarks and exposes options.
# This CMake project supports being embedded into other projects using add_subdirectory.
# This CMake project also support being found by find_package(vmcontainer).

cmake_minimum_required(VERSION 3.12)

# TODO: To replace by PROJECT_IS_TOP_LEVEL when requiring CMake 3.21
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(VMCONTAINER_STANDALONE ON)
else()
    set(VMCONTAINER_STANDALONE OFF)
endif()

project(vmcontainer CXX)

option(VMCONTAINER_BUILD_TESTS "Enable the vmcontainer test suite" ${VMCONTAINER_STANDALONE})
option(VMCONTAINER_BUILD_BENCHMARKS "Enable the vmcontainer benchmark suite" ${VMCONTAINER_STANDALONE})

add_subdirectory(lib)

if(VMCONTAINER_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(VMCONTAINER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
