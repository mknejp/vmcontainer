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
  std::is_base_of<mknejp::detail::_pinned_vector::virtual_memory_page_stack<>, mknejp::virtual_memory_page_stack>(),
  "");

static_assert(std::is_nothrow_move_constructible<mknejp::virtual_memory_page_stack>::value, "");
static_assert(std::is_nothrow_move_assignable<mknejp::virtual_memory_page_stack>::value, "");

TEST_CASE("virtual_memory_page_stack")
{
  struct Tag
  {
  };
  using virtual_memory_system_stub = pinned_vector_test::virtual_memory_system_stub<Tag>;
  auto alloc = pinned_vector_test::tracking_allocator<Tag>();

  virtual_memory_system_stub::page_size = [] { return 100; };

  char block1[4000];
  char block2[4000];

  alloc.expect_reserve(block1, 1000);
  alloc.expect_free(block1);

  using virtual_memory_page_stack =
    mknejp::detail::_pinned_vector::virtual_memory_page_stack<virtual_memory_system_stub>;

  SECTION("default constructed has no reservation")
  {
    {
      auto vmr = virtual_memory_page_stack();
      REQUIRE(vmr.base() == nullptr);
      REQUIRE(vmr.reserved_bytes() == 0);
      REQUIRE(vmr.committed_bytes() == 0);
    }
    REQUIRE(alloc.reservations() == 0);
    REQUIRE(alloc.reserve_calls() == 0);
    REQUIRE(alloc.free_calls() == 0);
    REQUIRE(alloc.commit_calls() == 0);
    REQUIRE(alloc.decommit_calls() == 0);
  }

  SECTION("ctor/dtor reserve/free virtual memory")
  {
    {
      auto vmps = virtual_memory_page_stack(1000);
      REQUIRE(alloc.reservations() == 1);
      REQUIRE(alloc.reserve_calls() == 1);
      REQUIRE(alloc.free_calls() == 0);
      REQUIRE(vmps.base() == block1);
      REQUIRE(vmps.reserved_bytes() == 1000);
      REQUIRE(vmps.committed_bytes() == 0);
      REQUIRE(vmps.page_size() == 100);
    }
    REQUIRE(alloc.reservations() == 0);
    REQUIRE(alloc.reserve_calls() == 1);
    REQUIRE(alloc.free_calls() == 1);
    REQUIRE(alloc.commit_calls() == 0);
    REQUIRE(alloc.decommit_calls() == 0);
  }

  SECTION("move construction")
  {
    {
      auto vmps1 = virtual_memory_page_stack(1000);
      alloc.expect_commit(block1, 400);
      vmps1.commit(400);

      auto vmps2 = std::move(vmps1);
      REQUIRE(alloc.reservations() == 1);
      REQUIRE(alloc.reserve_calls() == 1);
      REQUIRE(alloc.free_calls() == 0);
      REQUIRE(alloc.commit_calls() == 1);
      REQUIRE(alloc.decommit_calls() == 0);
      REQUIRE(vmps1.base() == nullptr);
      REQUIRE(vmps2.base() == block1);
      REQUIRE(vmps1.reserved_bytes() == 0);
      REQUIRE(vmps2.reserved_bytes() == 1000);
      REQUIRE(vmps1.committed_bytes() == 0);
      REQUIRE(vmps2.committed_bytes() == 400);
      REQUIRE(vmps1.page_size() == 0);
      REQUIRE(vmps2.page_size() == 100);

      alloc.expect_decommit(block1, 400);
    }
    REQUIRE(alloc.reservations() == 0);
    REQUIRE(alloc.reserve_calls() == 1);
    REQUIRE(alloc.free_calls() == 1);
    REQUIRE(alloc.commit_calls() == 1);
    REQUIRE(alloc.decommit_calls() == 1);
  }

  SECTION("move assignment")
  {
    {
      auto vmps1 = virtual_memory_page_stack(1000);
      alloc.expect_commit(block1, 400);
      vmps1.commit(400);

      {
        alloc.expect_reserve(block2, 2000);
        auto vmps2 = virtual_memory_page_stack(2000);
        alloc.expect_commit(block2, 300);
        vmps2.commit(300);

        alloc.expect_decommit(block1, 400);
        alloc.expect_free(block1);
        vmps1 = std::move(vmps2);

        REQUIRE(alloc.reservations() == 1);
        REQUIRE(alloc.reserve_calls() == 2);
        REQUIRE(alloc.free_calls() == 1);
        REQUIRE(alloc.commit_calls() == 2);
        REQUIRE(alloc.decommit_calls() == 1);
        REQUIRE(vmps1.base() == block2);
        REQUIRE(vmps2.base() == nullptr);
        REQUIRE(vmps1.reserved_bytes() == 2000);
        REQUIRE(vmps2.reserved_bytes() == 0);
        REQUIRE(vmps1.committed_bytes() == 300);
        REQUIRE(vmps2.committed_bytes() == 0);
        REQUIRE(vmps1.page_size() == 100);
        REQUIRE(vmps2.page_size() == 0);
      }
      alloc.expect_decommit(block2, 300);
      alloc.expect_free(block2);
    }
    REQUIRE(alloc.reservations() == 0);
    REQUIRE(alloc.reserve_calls() == 2);
    REQUIRE(alloc.free_calls() == 2);
    REQUIRE(alloc.commit_calls() == 2);
    REQUIRE(alloc.decommit_calls() == 2);
  }

  SECTION("self move assignment frees the reservation")
  {
    auto vmps = virtual_memory_page_stack(1000);
    alloc.expect_commit(block1, 400);
    vmps.commit(400);

    alloc.expect_decommit(block1, 400);
    alloc.expect_free(block1);

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
#endif
    vmps = std::move(vmps);
#ifdef __clang__
#pragma clang diagnostic pop
#endif

    REQUIRE(alloc.reservations() == 0);
    REQUIRE(alloc.reserve_calls() == 1);
    REQUIRE(alloc.free_calls() == 1);
    REQUIRE(alloc.commit_calls() == 1);
    REQUIRE(alloc.decommit_calls() == 1);
    REQUIRE(vmps.base() == nullptr);
    REQUIRE(vmps.reserved_bytes() == 0);
    REQUIRE(vmps.committed_bytes() == 0);
    REQUIRE(vmps.page_size() == 0);
  }

  SECTION("single commit() is decommited in dtor")
  {
    {
      auto vmps = virtual_memory_page_stack(1000);
      alloc.expect_commit(block1, 100);
      vmps.commit(100);
      REQUIRE(vmps.committed_bytes() == 100);
      REQUIRE(alloc.commit_calls() == 1);
      REQUIRE(alloc.decommit_calls() == 0);

      alloc.expect_decommit(block1, 100);
    }
    REQUIRE(alloc.reservations() == 0);
    REQUIRE(alloc.reserve_calls() == 1);
    REQUIRE(alloc.free_calls() == 1);
    REQUIRE(alloc.commit_calls() == 1);
    REQUIRE(alloc.decommit_calls() == 1);
  }
  SECTION("single commit() and matching decommit()")
  {
    {
      auto vmps = virtual_memory_page_stack(1000);
      alloc.expect_commit(block1, 100);
      vmps.commit(100);
      REQUIRE(vmps.committed_bytes() == 100);
      REQUIRE(alloc.commit_calls() == 1);
      REQUIRE(alloc.decommit_calls() == 0);

      alloc.expect_decommit(block1, 100);
      vmps.decommit(100);
      REQUIRE(vmps.committed_bytes() == 0);
      REQUIRE(alloc.commit_calls() == 1);
      REQUIRE(alloc.decommit_calls() == 1);
    }
    REQUIRE(alloc.reservations() == 0);
    REQUIRE(alloc.reserve_calls() == 1);
    REQUIRE(alloc.free_calls() == 1);
    REQUIRE(alloc.commit_calls() == 1);
    REQUIRE(alloc.decommit_calls() == 1);
  }
  SECTION("multiple commit() are decommited in dtor as one")
  {
    {
      auto vmps = virtual_memory_page_stack(1000);
      alloc.expect_commit(block1, 100);
      vmps.commit(100);
      REQUIRE(vmps.committed_bytes() == 100);
      REQUIRE(alloc.commit_calls() == 1);
      REQUIRE(alloc.decommit_calls() == 0);

      alloc.expect_commit(block1 + 100, 100);
      vmps.commit(100);
      REQUIRE(vmps.committed_bytes() == 200);
      REQUIRE(alloc.commit_calls() == 2);
      REQUIRE(alloc.decommit_calls() == 0);

      alloc.expect_decommit(block1, 200);
    }
    REQUIRE(alloc.reservations() == 0);
    REQUIRE(alloc.reserve_calls() == 1);
    REQUIRE(alloc.free_calls() == 1);
    REQUIRE(alloc.commit_calls() == 2);
    REQUIRE(alloc.decommit_calls() == 1);
  }
  SECTION("multiple commit() and decommit()")
  {
    {
      auto vmps = virtual_memory_page_stack(1000);
      alloc.expect_commit(block1, 100);
      vmps.commit(100);
      REQUIRE(vmps.committed_bytes() == 100);
      REQUIRE(alloc.commit_calls() == 1);
      REQUIRE(alloc.decommit_calls() == 0);

      alloc.expect_commit(block1 + 100, 200);
      vmps.commit(200);
      REQUIRE(vmps.committed_bytes() == 300);
      REQUIRE(alloc.commit_calls() == 2);
      REQUIRE(alloc.decommit_calls() == 0);

      alloc.expect_decommit(block1 + 200, 100);
      vmps.decommit(100);
      REQUIRE(vmps.committed_bytes() == 200);
      REQUIRE(alloc.commit_calls() == 2);
      REQUIRE(alloc.decommit_calls() == 1);

      alloc.expect_commit(block1 + 200, 200);
      vmps.commit(200);
      REQUIRE(vmps.committed_bytes() == 400);
      REQUIRE(alloc.commit_calls() == 3);
      REQUIRE(alloc.decommit_calls() == 1);

      alloc.expect_decommit(block1 + 300, 100);
      vmps.decommit(100);
      REQUIRE(vmps.committed_bytes() == 300);
      REQUIRE(alloc.commit_calls() == 3);
      REQUIRE(alloc.decommit_calls() == 2);

      alloc.expect_decommit(block1, 300);
    }
    REQUIRE(alloc.reservations() == 0);
    REQUIRE(alloc.reserve_calls() == 1);
    REQUIRE(alloc.free_calls() == 1);
    REQUIRE(alloc.commit_calls() == 3);
    REQUIRE(alloc.decommit_calls() == 3);
  }
  SECTION("commit() amount is rounded up to page size")
  {
    {
      auto vmps = virtual_memory_page_stack(1000);
      vmps.commit(0);
      REQUIRE(vmps.committed_bytes() == 0);
      REQUIRE(alloc.commit_calls() == 0);
      REQUIRE(alloc.decommit_calls() == 0);

      alloc.expect_commit(block1, 100);
      vmps.commit(1);
      REQUIRE(vmps.committed_bytes() == 100);
      REQUIRE(alloc.commit_calls() == 1);
      REQUIRE(alloc.decommit_calls() == 0);

      alloc.expect_commit(block1 + 100, 100);
      vmps.commit(99);
      REQUIRE(vmps.committed_bytes() == 200);
      REQUIRE(alloc.commit_calls() == 2);
      REQUIRE(alloc.decommit_calls() == 0);

      alloc.expect_commit(block1 + 200, 200);
      vmps.commit(101);
      REQUIRE(vmps.committed_bytes() == 400);
      REQUIRE(alloc.commit_calls() == 3);
      REQUIRE(alloc.decommit_calls() == 0);

      alloc.expect_commit(block1 + 400, 200);
      vmps.commit(199);
      REQUIRE(vmps.committed_bytes() == 600);
      REQUIRE(alloc.commit_calls() == 4);
      REQUIRE(alloc.decommit_calls() == 0);

      alloc.expect_decommit(block1, 600);
    }
    REQUIRE(alloc.reservations() == 0);
    REQUIRE(alloc.reserve_calls() == 1);
    REQUIRE(alloc.free_calls() == 1);
    REQUIRE(alloc.commit_calls() == 4);
    REQUIRE(alloc.decommit_calls() == 1);
  }
  SECTION("decommit() amount is rounded down to page size")
  {
    {
      auto vmps = virtual_memory_page_stack(1000);
      alloc.expect_commit(block1, 400);
      vmps.commit(400);
      REQUIRE(vmps.committed_bytes() == 400);
      REQUIRE(alloc.commit_calls() == 1);
      REQUIRE(alloc.decommit_calls() == 0);

      vmps.decommit(0);
      REQUIRE(vmps.committed_bytes() == 400);
      REQUIRE(alloc.commit_calls() == 1);
      REQUIRE(alloc.decommit_calls() == 0);

      vmps.decommit(1);
      REQUIRE(vmps.committed_bytes() == 400);
      REQUIRE(alloc.commit_calls() == 1);
      REQUIRE(alloc.decommit_calls() == 0);

      vmps.decommit(99);
      REQUIRE(vmps.committed_bytes() == 400);
      REQUIRE(alloc.commit_calls() == 1);
      REQUIRE(alloc.decommit_calls() == 0);

      alloc.expect_decommit(block1 + 300, 100);
      vmps.decommit(101);
      REQUIRE(vmps.committed_bytes() == 300);
      REQUIRE(alloc.commit_calls() == 1);
      REQUIRE(alloc.decommit_calls() == 1);

      alloc.expect_decommit(block1 + 200, 100);
      vmps.decommit(199);
      REQUIRE(vmps.committed_bytes() == 200);
      REQUIRE(alloc.commit_calls() == 1);
      REQUIRE(alloc.decommit_calls() == 2);

      alloc.expect_decommit(block1, 200);
    }
    REQUIRE(alloc.reservations() == 0);
    REQUIRE(alloc.reserve_calls() == 1);
    REQUIRE(alloc.free_calls() == 1);
    REQUIRE(alloc.commit_calls() == 1);
    REQUIRE(alloc.decommit_calls() == 3);
  }

  SECTION("swap")
  {
    {
      auto vmps1 = virtual_memory_page_stack(1000);
      alloc.expect_commit(block1, 400);
      vmps1.commit(400);

      {
        alloc.expect_reserve(block2, 2000);
        auto vmps2 = virtual_memory_page_stack(2000);
        alloc.expect_commit(block2, 300);
        vmps2.commit(300);

        static_assert(noexcept(swap(vmps1, vmps2)), "virtual_memory_page_stack::swap() is not noexcept");

        swap(vmps1, vmps2);
        REQUIRE(alloc.reservations() == 2);
        REQUIRE(alloc.reserve_calls() == 2);
        REQUIRE(alloc.free_calls() == 0);
        REQUIRE(alloc.commit_calls() == 2);
        REQUIRE(alloc.decommit_calls() == 0);
        REQUIRE(vmps1.base() == block2);
        REQUIRE(vmps2.base() == block1);
        REQUIRE(vmps1.reserved_bytes() == 2000);
        REQUIRE(vmps2.reserved_bytes() == 1000);
        REQUIRE(vmps1.committed_bytes() == 300);
        REQUIRE(vmps2.committed_bytes() == 400);

        alloc.expect_decommit(block1, 400);
        alloc.expect_free(block1);
      }
      alloc.expect_decommit(block2, 300);
      alloc.expect_free(block2);
    }
    REQUIRE(alloc.reservations() == 0);
    REQUIRE(alloc.reserve_calls() == 2);
    REQUIRE(alloc.free_calls() == 2);
    REQUIRE(alloc.commit_calls() == 2);
    REQUIRE(alloc.decommit_calls() == 2);
  }
}
