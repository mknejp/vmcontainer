cmake_minimum_required(VERSION 3.10)

project(vmcontainer)

add_library(
  vmcontainer

  include/vmcontainer/detail.hpp
  include/vmcontainer/pinned_vector.hpp
  include/vmcontainer/vm.hpp
  src/vmcontainer/vm.cpp
)
add_library(mknejp::vmcontainer ALIAS vmcontainer)

target_include_directories(vmcontainer PUBLIC include)
target_compile_features(vmcontainer PUBLIC cxx_std_14)
