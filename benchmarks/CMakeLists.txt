find_package(benchmark CONFIG REQUIRED)

add_executable(vmcontainer.bench push_back.cpp)
target_link_libraries(vmcontainer.bench PRIVATE vmcontainer benchmark::benchmark benchmark::benchmark_main)
