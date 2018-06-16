//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "pinned_vector.hpp"

#include <benchmark/benchmark.h>

#include <algorithm>
#include <vector>

using mknejp::pinned_vector;

namespace
{
  template<typename>
  struct tag
  {
  };

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
}

template<typename Vector, typename T>
static void push_back_baseline(benchmark::State& state, tag<Vector>, T x)
{
  auto max_size = static_cast<typename Vector::size_type>(state.range(0));
  auto v = init_vector(max_size, tag<Vector>());
  v.reserve(max_size);
  for(auto _ : state)
  {
    std::fill_n(std::back_inserter(v), max_size, x);
    v.clear();
  }
  benchmark::DoNotOptimize(v.end());
}

// trivially copyable types
BENCHMARK_CAPTURE(push_back_baseline, std::vector<int>, tag<std::vector<int>>(), 12345)
  ->RangeMultiplier(2)
  ->Range(1024, 2 << 14);
BENCHMARK_CAPTURE(push_back_baseline, pinned_vector<int>, tag<pinned_vector<int>>(), 12345)
  ->RangeMultiplier(2)
  ->Range(1024, 2 << 14);

// std::string with small stirng optimization
BENCHMARK_CAPTURE(push_back_baseline, std::vector<string>, tag<std::vector<std::string>>(), std::string("abcd"))
  ->RangeMultiplier(2)
  ->Range(1024, 2 << 20);
BENCHMARK_CAPTURE(push_back_baseline, pinned_vector<string>, tag<pinned_vector<std::string>>(), std::string("abcd"))
  ->RangeMultiplier(2)
  ->Range(1024, 2 << 20);
