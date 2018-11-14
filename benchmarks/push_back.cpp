//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "bench-utils.hpp"

#include "vmcontainer/pinned_vector.hpp"

#include <algorithm>
#include <chrono>
#include <vector>

using namespace mknejp::vmcontainer;
using namespace bench_utils;

namespace
{
  template<typename T>
  auto init_vector(std::size_t max_size, tag<std::vector<T>>) -> std::vector<T>
  {
    return std::vector<T>();
  }

  template<typename T>
  auto init_vector(std::size_t max_size, tag<pinned_vector<T>>) -> pinned_vector<T>
  {
    return pinned_vector<T>(max_elements(max_size));
  }

  auto const max_bytes_tests = {
    std::int64_t(64),
    std::int64_t(128),
    std::int64_t(256),
    std::int64_t(512),
    std::int64_t(1) * 1024,
    std::int64_t(4) * 1024,
    std::int64_t(16) * 1024,
    std::int64_t(64) * 1024,
    std::int64_t(128) * 1024,
    std::int64_t(512) * 1024,
    std::int64_t(1) * 1024 * 1024,
    std::int64_t(4) * 1024 * 1024,
    std::int64_t(16) * 1024 * 1024,
    std::int64_t(64) * 1024 * 1024,
    std::int64_t(128) * 1024 * 1024,
    std::int64_t(512) * 1024 * 1024,
    std::int64_t(1) * 1024 * 1024 * 1024,
    std::int64_t(2) * 1024 * 1024 * 1024,
    std::int64_t(4) * 1024 * 1024 * 1024,
  };

  template<typename T>
  auto configure_runs(benchmark::internal::Benchmark* b)
  {
    b->UseManualTime();
    b->Unit(benchmark::kNanosecond);
    for(auto max_bytes: max_bytes_tests)
    {
      if(max_bytes / sizeof(T) > 0)
      {
        b->Arg(max_bytes);
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// push_back_basline
//

// Establish a test baseline by only doing push_back without any allocations
template<typename Vector, typename T>
static auto baseline_push_back(benchmark::State& state, tag<Vector>, T x)
{
  auto const max_size = static_cast<typename Vector::size_type>(state.range(0)) / sizeof(T);
  auto v = init_vector(max_size, tag<Vector>());
  v.reserve(max_size);
  benchmark::DoNotOptimize(v.data());

  for(auto _: state)
  {
    (void)_;
    // Do not count reserve + destructor
    auto start = std::chrono::high_resolution_clock::now();
    std::fill_n(std::back_inserter(v), max_size, x);
    benchmark::ClobberMemory();
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count());
    v.clear();
  }
}

// trivially copyable types
BENCHMARK_CAPTURE(baseline_push_back, std::vector<int>, tag<std::vector<int>>(), 12345)->Apply(configure_runs<int>);
BENCHMARK_CAPTURE(baseline_push_back, pinned_vector<int>, tag<pinned_vector<int>>(), 12345)->Apply(configure_runs<int>);

BENCHMARK_CAPTURE(baseline_push_back,
                  std::vector<bigval>,
                  tag<std::vector<bigval>>(),
                  bigval{1, 2, 3, 4, 5, 6, 7, 8, 9, 0})
  ->Apply(configure_runs<bigval>);
BENCHMARK_CAPTURE(baseline_push_back,
                  pinned_vector<bigval>,
                  tag<pinned_vector<bigval>>(),
                  bigval{1, 2, 3, 4, 5, 6, 7, 8, 9, 0})
  ->Apply(configure_runs<bigval>);

// std::string with small string optimization
BENCHMARK_CAPTURE(baseline_push_back, std::vector<small string>, tag<std::vector<std::string>>(), std::string("abcd"))
  ->Apply(configure_runs<std::string>);
BENCHMARK_CAPTURE(baseline_push_back,
                  pinned_vector<small string>,
                  tag<pinned_vector<std::string>>(),
                  std::string("abcd"))
  ->Apply(configure_runs<std::string>);

///////////////////////////////////////////////////////////////////////////////
// push_back
//

template<typename Vector, typename T>
static auto push_back(benchmark::State& state, tag<Vector>, T x)
{
  auto const max_size = static_cast<typename Vector::size_type>(state.range(0)) / sizeof(T);
  for(auto _: state)
  {
    (void)_;
    auto v = init_vector(max_size, tag<Vector>());

    // Do not count destructor
    auto start = std::chrono::high_resolution_clock::now();
    std::fill_n(std::back_inserter(v), max_size, x);
    benchmark::DoNotOptimize(v.data());
    benchmark::ClobberMemory();
    auto end = std::chrono::high_resolution_clock::now();

    state.SetIterationTime(std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count());
  }
}

// trivially copyable types
BENCHMARK_CAPTURE(push_back, std::vector<int>, tag<std::vector<int>>(), 12345)->Apply(configure_runs<int>);
BENCHMARK_CAPTURE(push_back, pinned_vector<int>, tag<pinned_vector<int>>(), 12345)->Apply(configure_runs<int>);

BENCHMARK_CAPTURE(push_back, std::vector<bigval>, tag<std::vector<bigval>>(), bigval{1, 2, 3, 4, 5, 6, 7, 8, 9, 0})
  ->Apply(configure_runs<bigval>);
BENCHMARK_CAPTURE(push_back, pinned_vector<bigval>, tag<pinned_vector<bigval>>(), bigval{1, 2, 3, 4, 5, 6, 7, 8, 9, 0})
  ->Apply(configure_runs<bigval>);

// std::string with small string optimization
BENCHMARK_CAPTURE(push_back, std::vector<small string>, tag<std::vector<std::string>>(), std::string("abcd"))
  ->Apply(configure_runs<std::string>);
BENCHMARK_CAPTURE(push_back, pinned_vector<small string>, tag<pinned_vector<std::string>>(), std::string("abcd"))
  ->Apply(configure_runs<std::string>);
