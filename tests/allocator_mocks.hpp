//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include "catch.hpp"

#include <functional>
#include <map>

namespace pinned_vector_test
{
  template<typename Tag>
  struct virtual_memory_system_stub
  {
    static std::function<auto(std::size_t)->void*> reserve;
    static std::function<auto(void*, std::size_t num_bytes)->void> free;
    static std::function<auto(void*, std::size_t)->void> commit;
    static std::function<auto(void*, std::size_t)->void> decommit;
    static std::function<auto()->size_t> page_size;

    static auto reset() -> void
    {
      reserve = [](std::size_t) -> void* {
        FAIL("virtual_memory_system_stub::reserve() called without setup");
        return nullptr;
      };
      free = [](void*, std::size_t) { FAIL("virtual_memory_system_stub::free() called without setup"); };
      commit = [](void*, std::size_t) { FAIL("virtual_memory_system_stub::commit() called without setup"); };
      decommit = [](void*, std::size_t) { FAIL("virtual_memory_system_stub::decommit() called without setup"); };
      page_size = []() -> std::size_t {
        FAIL("virtual_memory_system_stub::page_size() called without setup");
        return 0;
      };
    }
  };

  template<typename Tag>
  std::function<auto(std::size_t)->void*> virtual_memory_system_stub<Tag>::reserve;
  template<typename Tag>
  std::function<auto(void*, std::size_t num_bytes)->void> virtual_memory_system_stub<Tag>::free;
  template<typename Tag>
  std::function<auto(void*, std::size_t)->void> virtual_memory_system_stub<Tag>::commit;
  template<typename Tag>
  std::function<auto(void*, std::size_t)->void> virtual_memory_system_stub<Tag>::decommit;
  template<typename Tag>
  std::function<auto()->size_t> virtual_memory_system_stub<Tag>::page_size;

  template<typename Tag>
  class tracking_allocator
  {
  public:
    tracking_allocator() { virtual_memory_system_stub<Tag>::reset(); }

    auto expect_reserve(void* block, std::size_t expected_size) -> void
    {
      virtual_memory_system_stub<Tag>::reserve = [this, block, expected_size](std::size_t num_bytes) {
        REQUIRE(num_bytes == expected_size);
        auto result = _reservations.insert(std::make_pair(block, num_bytes));
        REQUIRE(result.second == true);
        ++_reserve_calls;
        return block;
      };
    };

    auto expect_free(void* block) -> void
    {
      virtual_memory_system_stub<Tag>::free = [this, block](void* p, std::size_t num_bytes) {
        REQUIRE(block == p);
        auto it = _reservations.find(p);
        REQUIRE(it != _reservations.end());
        REQUIRE(it->second == num_bytes);
        _reservations.erase(it);
        ++_free_calls;
      };
    };

    auto expect_commit(void* offset, std::size_t expected_size) -> void
    {
      virtual_memory_system_stub<Tag>::commit = [this, offset, expected_size](void* p, std::size_t num_bytes) {
        REQUIRE(num_bytes == expected_size);
        REQUIRE(offset == p);
        ++_commit_calls;
      };
    };

    auto expect_decommit(void* offset, std::size_t expected_size) -> void
    {
      virtual_memory_system_stub<Tag>::decommit = [this, offset, expected_size](void* p, std::size_t num_bytes) {
        REQUIRE(num_bytes == expected_size);
        REQUIRE(offset == p);
        ++_decommit_calls;
      };
    };

    auto reservations() const noexcept -> std::size_t { return _reservations.size(); }
    auto reserve_calls() const noexcept -> int { return _reserve_calls; }
    auto free_calls() const noexcept -> int { return _free_calls; }
    auto commit_calls() const noexcept -> int { return _commit_calls; }
    auto decommit_calls() const noexcept -> int { return _decommit_calls; }

  private:
    std::map<void*, std::size_t> _reservations;

    int _reserve_calls = 0;
    int _free_calls = 0;
    int _commit_calls = 0;
    int _decommit_calls = 0;
  };
}
