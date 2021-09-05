cmake_minimum_required(VERSION 3.12)

project(vmcontainer)

file(GLOB_RECURSE vmcontainer-sources CONFIGURE_DEPENDS *.cpp)
add_library(vmcontainer ${vmcontainer-sources})
add_library(mknejp::vmcontainer ALIAS vmcontainer)

target_include_directories(vmcontainer PUBLIC include)
target_compile_features(vmcontainer PUBLIC cxx_std_14)
