# This file is intended for use by library developers/maintainers, as well as users and package managers.
# It declares targets for unit tests and benchmarks and exposes options.
# This CMake project supports being embedded into other projects using add_subdirectory.
# This CMake project also support being found by find_package(vmcontainer).
# Also supports being used with FetchContent

cmake_minimum_required(VERSION 3.12)

if(VMCONTAINER_DEV_MODE)
  set(VMCONTAINER_DEV_OPTION ON)
else()
  set(VMCONTAINER_DEV_OPTION OFF)
endif()

project(vmcontainer CXX)

option(VMCONTAINER_BUILD_TESTS "Enable the vmcontainer test suite" ${VMCONTAINER_DEV_OPTION})
option(VMCONTAINER_BUILD_BENCHMARKS "Enable the vmcontainer benchmark suite" ${VMCONTAINER_DEV_OPTION})

add_subdirectory(lib)

if(VMCONTAINER_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

if(VMCONTAINER_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
