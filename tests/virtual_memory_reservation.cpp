//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "pinned_vector.hpp"

#include "allocator_mocks.hpp"

#include "catch.hpp"

#include <type_traits>

static_assert(
  std::is_base_of<mknejp::detail::_pinned_vector::virtual_memory_reservation<>, mknejp::virtual_memory_reservation>(),
  "");

TEST_CASE("virtual_memory_reservation")
{
  struct Tag
  {
  };
  using allocator_stub = pinned_vector_test::allocator_stub<Tag>;
  auto alloc = pinned_vector_test::reservation_tracking_allocator<Tag>();

  char block1[100];

  alloc.expect_reserve(block1, 100);
  alloc.expect_free(block1);

  using virtual_memory_reservation = mknejp::detail::_pinned_vector::virtual_memory_reservation<allocator_stub>;

  SECTION("ctor/dtor reserve/free virtual memory")
  {
    {
      auto vmr = virtual_memory_reservation(100);
      REQUIRE(alloc.reservations() == 1);
      REQUIRE(alloc.reserve_calls() == 1);
      REQUIRE(alloc.free_calls() == 0);
      REQUIRE(vmr.base() == block1);
      REQUIRE(vmr.reserved_bytes() == 100);
    }
    REQUIRE(alloc.reservations() == 0);
    REQUIRE(alloc.reserve_calls() == 1);
    REQUIRE(alloc.free_calls() == 1);
  }

  SECTION("copy construction duplicates virtual memory reservaton")
  {
    {
      auto vmr1 = virtual_memory_reservation(100);
      {
        // Give the copy a new reservation
        char block2[100];
        alloc.expect_reserve(block2, 100);
        alloc.expect_free(block2);

        auto vmr2 = vmr1;
        REQUIRE(alloc.reservations() == 2);
        REQUIRE(alloc.reserve_calls() == 2);
        REQUIRE(alloc.free_calls() == 0);
        REQUIRE(vmr1.base() == block1);
        REQUIRE(vmr1.reserved_bytes() == 100);
        REQUIRE(vmr2.base() == block2);
        REQUIRE(vmr2.reserved_bytes() == 100);
      }
      // Check vmr1 is freeing its reservation
      alloc.expect_free(block1);
    }
    REQUIRE(alloc.reservations() == 0);
    REQUIRE(alloc.reserve_calls() == 2);
    REQUIRE(alloc.free_calls() == 2);
  }

  SECTION("copy assignment duplicates virtual memory reservaton")
  {
    {
      auto vmr1 = virtual_memory_reservation(100);

      // Give the copies a new reservation
      char block2[200];
      char block3[200];
      {
        alloc.expect_reserve(block2, 200);
        auto vmr2 = virtual_memory_reservation(200);

        // vmr1 gets a new reservation but must free block1
        alloc.expect_reserve(block3, 200);

        vmr1 = vmr2;
        REQUIRE(alloc.reservations() == 2);
        REQUIRE(alloc.reserve_calls() == 3);
        REQUIRE(alloc.free_calls() == 1);
        REQUIRE(vmr1.base() == block3);
        REQUIRE(vmr1.reserved_bytes() == 200);
        REQUIRE(vmr2.base() == block2);
        REQUIRE(vmr2.reserved_bytes() == 200);
        alloc.expect_free(block2);
      }
      // vmr1 now frees the new reservation
      alloc.expect_free(block3);
    }
    REQUIRE(alloc.reservations() == 0);
    REQUIRE(alloc.reserve_calls() == 3);
    REQUIRE(alloc.free_calls() == 3);
  }

  SECTION("move construction does not affect reservations")
  {
    {
      auto vmr1 = virtual_memory_reservation(100);

      auto vmr2 = std::move(vmr1);
      REQUIRE(alloc.reservations() == 1);
      REQUIRE(alloc.reserve_calls() == 1);
      REQUIRE(alloc.free_calls() == 0);
      REQUIRE(vmr1.base() == nullptr);
      REQUIRE(vmr1.reserved_bytes() == 0);
      REQUIRE(vmr2.base() == block1);
      REQUIRE(vmr2.reserved_bytes() == 100);
    }
    REQUIRE(alloc.reservations() == 0);
    REQUIRE(alloc.reserve_calls() == 1);
    REQUIRE(alloc.free_calls() == 1);
  }

  SECTION("move assignment does not affect reservations")
  {
    {
      auto vmr1 = virtual_memory_reservation(100);

      char block2[200];
      alloc.expect_reserve(block2, 200);
      auto vmr2 = virtual_memory_reservation(200);

      vmr1 = std::move(vmr2);
      REQUIRE(alloc.reservations() == 1);
      REQUIRE(alloc.reserve_calls() == 2);
      REQUIRE(alloc.free_calls() == 1);
      REQUIRE(vmr1.base() == block2);
      REQUIRE(vmr1.reserved_bytes() == 200);
      REQUIRE(vmr2.base() == nullptr);
      REQUIRE(vmr2.reserved_bytes() == 0);
      alloc.expect_free(block2);
    }
    REQUIRE(alloc.reservations() == 0);
    REQUIRE(alloc.reserve_calls() == 2);
    REQUIRE(alloc.free_calls() == 2);
  }
}
