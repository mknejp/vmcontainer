//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "vmcontainer/pinned_vector.hpp"

#include <benchmark/benchmark.h>

#include <algorithm>
#include <chrono>
#include <vector>

using mknejp::pinned_vector;

namespace
{
  template<typename>
  struct tag
  {};

  template<typename T>
  auto init_vector(std::size_t max_size, tag<std::vector<T>>) -> std::vector<T>
  {
    return std::vector<T>();
  }

  template<typename T>
  auto init_vector(std::size_t max_size, tag<pinned_vector<T>>) -> pinned_vector<T>
  {
    return pinned_vector<T>(max_size);
  }

  constexpr auto max_memory_bytes = std::size_t(4) * 1024 * 1024 * 1024;

  auto configure_all(benchmark::internal::Benchmark* b) -> void { b->UseManualTime()->Unit(benchmark::kNanosecond); }
  auto configure_int(benchmark::internal::Benchmark* b) -> void
  {
    configure_all(b);
    b->RangeMultiplier(5)->Range(1, max_memory_bytes / sizeof(int));
  }
  auto configure_string(benchmark::internal::Benchmark* b) -> void
  {
    configure_all(b);
    b->RangeMultiplier(5)->Range(1, max_memory_bytes / sizeof(std::string));
  }
}

///////////////////////////////////////////////////////////////////////////////
// push_back_basline
//

// Establish a test baseline by only doing push_back without any allocations
template<typename Vector, typename T>
static void baseline_push_back(benchmark::State& state, tag<Vector>, T x)
{
  auto max_size = static_cast<typename Vector::size_type>(state.range(0));
  auto v = init_vector(max_size, tag<Vector>());
  v.reserve(max_size);

  for(auto _: state)
  {
    (void)_;
    // Do not count reserve + destructor
    auto start = std::chrono::high_resolution_clock::now();
    std::fill_n(std::back_inserter(v), max_size, x);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count());
    v.clear();
  }
  benchmark::DoNotOptimize(v.end());
}

// trivially copyable types
BENCHMARK_CAPTURE(baseline_push_back, std::vector<int>, tag<std::vector<int>>(), 12345)->Apply(configure_int);
BENCHMARK_CAPTURE(baseline_push_back, pinned_vector<int>, tag<pinned_vector<int>>(), 12345)->Apply(configure_int);

// std::string with small stirng optimization
BENCHMARK_CAPTURE(baseline_push_back, std::vector<string>, tag<std::vector<std::string>>(), std::string("abcd"))
  ->Apply(configure_string);
BENCHMARK_CAPTURE(baseline_push_back, pinned_vector<string>, tag<pinned_vector<std::string>>(), std::string("abcd"))
  ->Apply(configure_string);

///////////////////////////////////////////////////////////////////////////////
// push_back
//

template<typename Vector, typename T>
static void push_back(benchmark::State& state, tag<Vector>, T x)
{
  for(auto _: state)
  {
    (void)_;
    auto max_size = static_cast<typename Vector::size_type>(state.range(0));
    auto v = init_vector(max_size, tag<Vector>());

    // Do not count destructor
    auto start = std::chrono::high_resolution_clock::now();
    std::fill_n(std::back_inserter(v), max_size, x);
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count());
    benchmark::DoNotOptimize(v.end());
  }
}

// trivially copyable types
BENCHMARK_CAPTURE(push_back, std::vector<int>, tag<std::vector<int>>(), 12345)->Apply(configure_int);
BENCHMARK_CAPTURE(push_back, pinned_vector<int>, tag<pinned_vector<int>>(), 12345)->Apply(configure_int);

// std::string with small stirng optimization
BENCHMARK_CAPTURE(push_back, std::vector<string>, tag<std::vector<std::string>>(), std::string("abcd"))
  ->Apply(configure_string);
BENCHMARK_CAPTURE(push_back, pinned_vector<string>, tag<pinned_vector<std::string>>(), std::string("abcd"))
  ->Apply(configure_string);
