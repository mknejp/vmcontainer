find_package(benchmark CONFIG REQUIRED)

add_executable(pinned_vector.bench push_back.cpp)
target_link_libraries(pinned_vector.bench PRIVATE pinned_vector benchmark::benchmark benchmark::benchmark_main)
