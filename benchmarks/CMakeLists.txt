find_package(benchmark CONFIG REQUIRED)

add_executable(
  vmcontainer.bench.push_back

  bench-utils.hpp
  push_back.cpp
)
target_link_libraries(vmcontainer.bench.push_back PRIVATE vmcontainer benchmark::benchmark benchmark::benchmark_main)

add_executable(
  vmcontainer.bench.push_back_copy

  bench-utils.hpp
  push_back_copy.cpp
)
target_link_libraries(vmcontainer.bench.push_back_copy PRIVATE vmcontainer benchmark::benchmark benchmark::benchmark_main)

add_executable(
  vmcontainer.bench.push_back_copy_and_alloc

  bench-utils.hpp
  push_back_copy_and_alloc.cpp
)
target_link_libraries(vmcontainer.bench.push_back_copy_and_alloc PRIVATE vmcontainer benchmark::benchmark benchmark::benchmark_main)
