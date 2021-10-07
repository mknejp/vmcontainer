cmake_minimum_required(VERSION 3.12)

project(vmcontainer CXX)

option(VMCONTAINER_DEV_MODE "Enables all the dev option by default")
option(VMCONTAINER_BUILD_TESTS "Enable the vmcontainer test suite" ${VMCONTAINER_DEV_MODE})
option(VMCONTAINER_BUILD_BENCHMARKS "Enable the vmcontainer benchmark suite" ${VMCONTAINER_DEV_MODE})

add_subdirectory(lib)

if(VMCONTAINER_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

if(VMCONTAINER_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
