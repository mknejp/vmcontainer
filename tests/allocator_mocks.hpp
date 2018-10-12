//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include "catch.hpp"

#include <functional>
#include <map>

namespace vmcontainer_test
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
    using vm_stub = virtual_memory_system_stub<Tag>;

    tracking_allocator() { vm_stub::reset(); }

    auto expect_reserve(void* block, std::size_t expected_size) -> void
    {
      vm_stub::reserve = [this, block, expected_size](std::size_t num_bytes) {
        REQUIRE(num_bytes == expected_size);
        auto result = _reservations.insert(std::make_pair(block, num_bytes));
        REQUIRE(result.second == true);
        ++_reserve_calls;
        return block;
      };
    };

    auto expect_free(void* block) -> void
    {
      vm_stub::free = [this, block](void* p, std::size_t num_bytes) {
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
      vm_stub::commit = [this, offset, expected_size](void* p, std::size_t num_bytes) {
        REQUIRE(num_bytes == expected_size);
        REQUIRE(offset == p);
        ++_commit_calls;
      };
    };

    auto expect_commit_and_fail(void* offset, std::size_t expected_size) -> void
    {
      vm_stub::commit = [this, offset, expected_size](void* p, std::size_t num_bytes) {
        REQUIRE(num_bytes == expected_size);
        REQUIRE(offset == p);
        ++_commit_calls;
        throw std::bad_alloc();
      };
    };

    auto expect_decommit(void* offset, std::size_t expected_size) -> void
    {
      vm_stub::decommit = [this, offset, expected_size](void* p, std::size_t num_bytes) {
        REQUIRE(num_bytes == expected_size);
        REQUIRE(offset == p);
        ++_decommit_calls;
      };
    };

    auto set_page_size(std::size_t n)
    {
      vm_stub::page_size = [n] { return n; };
    }

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
