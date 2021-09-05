# This file is intended for use by library developers/maintainers.
# It declares targets for unit tests and benchmarks.
# If you only want to use the library consume lib/CMakeLists.txt instead.

cmake_minimum_required(VERSION 3.12)

project(vmcontainer)

enable_testing()

add_subdirectory(lib)

add_subdirectory(benchmarks)
add_subdirectory(tests)
