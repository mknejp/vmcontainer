cmake_minimum_required(VERSION 3.10)

project(vmcontainer)

add_library(
  vmcontainer
    src/vm.cpp
    include/vmcontainer/detail.hpp
    include/vmcontainer/pinned_vector.hpp
    include/vmcontainer/vm.hpp
    )

target_include_directories(vmcontainer PUBLIC include)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

option(WITH_VMCONTAINER_BENCHMARKS OFF "Enable to include targets for vmcontainer benchmark applications.")
if(WITH_VMCONTAINER_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()
