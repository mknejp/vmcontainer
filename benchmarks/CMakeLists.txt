find_package(benchmark CONFIG REQUIRED)

add_executable(
  vmcontainer.bench.push_back

  bench-utils.hpp
  push_back.cpp
)

add_library(vmcontainer.bench.dependencies INTERFACE)
target_link_libraries(
  vmcontainer.bench.dependencies
  INTERFACE
  vmcontainer
  benchmark::benchmark
  benchmark::benchmark_main
)

target_link_libraries(vmcontainer.bench.push_back PRIVATE vmcontainer.bench.dependencies)

add_executable(
  vmcontainer.bench.push_back_copy

  bench-utils.hpp
  push_back_copy.cpp
)
target_link_libraries(vmcontainer.bench.push_back_copy PRIVATE vmcontainer.bench.dependencies)

add_executable(
  vmcontainer.bench.push_back_copy_and_alloc

  bench-utils.hpp
  push_back_copy_and_alloc.cpp
)
target_link_libraries(vmcontainer.bench.push_back_copy_and_alloc PRIVATE vmcontainer.bench.dependencies)
