# This file is intended for use by library developers/maintainers, as well as users and package managers.
# It declares targets for unit tests and benchmarks and exposes options.
# This CMake project supports being embedded into other projects using add_subdirectory.
# This CMake project also support being found by find_package(vmcontainer).

cmake_minimum_required(VERSION 3.12)

project(vmcontainer CXX)

add_subdirectory(lib)

add_subdirectory(benchmarks)

enable_testing()
add_subdirectory(tests)
