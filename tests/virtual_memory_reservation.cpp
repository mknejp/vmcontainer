//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "pinned_vector.hpp"

#include "catch.hpp"

#include <functional>
#include <map>
#include <type_traits>

static_assert(
  std::is_base_of<mknejp::detail::_pinned_vector::virtual_memory_reservation<>, mknejp::virtual_memory_reservation>(),
  "");

namespace
{
  struct mock_allocator
  {
    static std::function<auto(std::size_t)->void*> reserve;
    static std::function<auto(void*, std::size_t num_bytes)->void> free;
    static std::function<auto(void*, std::size_t)->void*> commit;
    static std::function<auto(void*, std::size_t)->void*> decommit;
    static std::function<auto()->size_t> page_size;
  };

  std::function<auto(std::size_t)->void*> mock_allocator::reserve;
  std::function<auto(void*, std::size_t num_bytes)->void> mock_allocator::free;
  std::function<auto(void*, std::size_t)->void*> mock_allocator::commit;
  std::function<auto(void*, std::size_t)->void*> mock_allocator::decommit;
  std::function<auto()->size_t> mock_allocator::page_size;
}

TEST_CASE("virtual_memory_reservation")
{
  auto reservations = std::map<void*, std::size_t>();

  auto reserve_calls = 0;
  auto free_calls = 0;

  auto expect_reserve = [&](void* block, std::size_t size) {
    mock_allocator::reserve = [&, block, size](std::size_t num_bytes) {
      REQUIRE(num_bytes == size);
      auto result = reservations.insert(std::make_pair(block, size));
      REQUIRE(result.second == true);
      ++reserve_calls;
      return block;
    };
  };
  auto expect_free = [&](void* block) {
    mock_allocator::free = [&, block](void* p, std::size_t num_bytes) {
      REQUIRE(block == p);
      auto it = reservations.find(p);
      REQUIRE(it != reservations.end());
      REQUIRE(it->second == num_bytes);
      reservations.erase(it);
      ++free_calls;
    };
  };

  char block1[100];

  expect_reserve(block1, 100);
  expect_free(block1);

  using virtual_memory_reservation = mknejp::detail::_pinned_vector::virtual_memory_reservation<mock_allocator>;

  SECTION("ctor/dtor reserve/free virtual memory")
  {
    {
      auto vmr = virtual_memory_reservation(100);
      REQUIRE(reservations.size() == 1);
      REQUIRE(reserve_calls == 1);
      REQUIRE(free_calls == 0);
      REQUIRE(vmr.base() == block1);
      REQUIRE(vmr.reserved_bytes() == 100);
    }
    REQUIRE(reservations.empty());
    REQUIRE(reserve_calls == 1);
    REQUIRE(free_calls == 1);
  }

  SECTION("copy construction duplicates virtual memory reservaton")
  {
    {
      auto vmr1 = virtual_memory_reservation(100);
      {
        // Give the copy a new reservation
        char block2[100];
        expect_reserve(block2, 100);
        expect_free(block2);

        auto vmr2 = vmr1;
        REQUIRE(reservations.size() == 2);
        REQUIRE(reserve_calls == 2);
        REQUIRE(free_calls == 0);
        REQUIRE(vmr1.base() == block1);
        REQUIRE(vmr1.reserved_bytes() == 100);
        REQUIRE(vmr2.base() == block2);
        REQUIRE(vmr2.reserved_bytes() == 100);
      }
      // Check vmr1 is freeing its reservation
      expect_free(block1);
    }
    REQUIRE(reservations.empty());
    REQUIRE(reserve_calls == 2);
    REQUIRE(free_calls == 2);
  }

  SECTION("copy assignment duplicates virtual memory reservaton")
  {
    {
      auto vmr1 = virtual_memory_reservation(100);

      // Give the copies a new reservation
      char block2[200];
      char block3[200];
      {
        expect_reserve(block2, 200);
        auto vmr2 = virtual_memory_reservation(200);

        // vmr1 gets a new reservation but must free block1
        expect_reserve(block3, 200);

        vmr1 = vmr2;
        REQUIRE(reservations.size() == 2);
        REQUIRE(reserve_calls == 3);
        REQUIRE(free_calls == 1);
        REQUIRE(vmr1.base() == block3);
        REQUIRE(vmr1.reserved_bytes() == 200);
        REQUIRE(vmr2.base() == block2);
        REQUIRE(vmr2.reserved_bytes() == 200);
        expect_free(block2);
      }
      // vmr1 now frees the new reservation
      expect_free(block3);
    }
    REQUIRE(reservations.empty());
    REQUIRE(reserve_calls == 3);
    REQUIRE(free_calls == 3);
  }

  SECTION("move construction does not affect reservations")
  {
    {
      auto vmr1 = virtual_memory_reservation(100);

      auto vmr2 = std::move(vmr1);
      REQUIRE(reservations.size() == 1);
      REQUIRE(reserve_calls == 1);
      REQUIRE(free_calls == 0);
      REQUIRE(vmr1.base() == nullptr);
      REQUIRE(vmr1.reserved_bytes() == 0);
      REQUIRE(vmr2.base() == block1);
      REQUIRE(vmr2.reserved_bytes() == 100);
    }
    REQUIRE(reservations.empty());
    REQUIRE(reserve_calls == 1);
    REQUIRE(free_calls == 1);
  }

  SECTION("move assignment does not affect reservations")
  {
    {
      auto vmr1 = virtual_memory_reservation(100);

      char block2[200];
      expect_reserve(block2, 200);
      auto vmr2 = virtual_memory_reservation(200);

      vmr1 = std::move(vmr2);
      REQUIRE(reservations.size() == 1);
      REQUIRE(reserve_calls == 2);
      REQUIRE(free_calls == 1);
      REQUIRE(vmr1.base() == block2);
      REQUIRE(vmr1.reserved_bytes() == 200);
      REQUIRE(vmr2.base() == nullptr);
      REQUIRE(vmr2.reserved_bytes() == 0);
      expect_free(block2);
    }
    REQUIRE(reservations.empty());
    REQUIRE(reserve_calls == 2);
    REQUIRE(free_calls == 2);
  }
}
