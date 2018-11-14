//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include <benchmark/benchmark.h>

namespace bench_utils
{
  template<typename>
  struct tag
  {};

  struct bigval
  {
    double a, b, c, d, e, f, g, h, i, j;
  };

  struct bumb_allocator_data
  {
    void* p;
    std::size_t available;
    std::vector<char> buffer;
  };

  template<typename T>
  class bumb_allocator
  {
  public:
    using value_type = T;

    explicit bumb_allocator(std::size_t size) : _data(new bumb_allocator_data{})
    {
      _data->buffer.resize(size);
      reset();
    }

    template<typename U>
    explicit bumb_allocator(bumb_allocator<U> const& other) : _data(other.data())
    {}

    T* allocate(std::size_t n)
    {
      if(std::align(alignof(T), sizeof(T) * n, _data->p, _data->available))
      {
        auto const p = static_cast<T*>(_data->p);
        _data->p = static_cast<char*>(_data->p) + sizeof(T) * n;
        _data->available -= sizeof(T) * n;
        return p;
      }
      else
      {
        throw std::bad_alloc();
      }
    }

    void deallocate(T*, std::size_t) {}

    template<typename U>
    bool operator==(bumb_allocator<U> const& other) const
    {
      return _data == other._data;
    }
    template<typename U>
    bool operator!=(bumb_allocator const& other) const
    {
      return !(*this == other);
    }

    auto data() const { return _data; }

    void reset()
    {
      _data->p = _data->buffer.data();
      _data->available = _data->buffer.size();
    }

  private:
    std::shared_ptr<bumb_allocator_data> _data;
  };
}
