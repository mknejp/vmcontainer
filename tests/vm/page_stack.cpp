//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "vmcontainer/vm.hpp"

#include "allocator_mocks.hpp"

#include "catch.hpp"

#include <type_traits>

using namespace mknejp::vmcontainer;

static_assert(std::is_base_of<vm::page_stack_base<vm::system_default>, vm::page_stack>(), "");

static_assert(std::is_nothrow_default_constructible<vm::page_stack>::value, "");
static_assert(std::is_nothrow_move_constructible<vm::page_stack>::value, "");
static_assert(std::is_nothrow_move_assignable<vm::page_stack>::value, "");

TEST_CASE("vm::page_stack", "[page_stack]")
{
  struct Tag
  {};
  using virtual_memory_system_stub = vmcontainer_test::virtual_memory_system_stub<Tag>;
  auto alloc = vmcontainer_test::tracking_allocator<Tag>();

  virtual_memory_system_stub::page_size = [] { return 100; };

  using page_stack = vm::page_stack_base<virtual_memory_system_stub>;

  SECTION("default constructed has no reservation")
  {
    {
      auto vmps = page_stack();
      CHECK(vmps.base() == nullptr);
      CHECK(vmps.reserved_bytes() == 0);
      CHECK(vmps.committed_bytes() == 0);
      CHECK(vmps.page_size() == 100);
    }
    CHECK(alloc.reservations() == 0);
    CHECK(alloc.reserve_calls() == 0);
    CHECK(alloc.free_calls() == 0);
    CHECK(alloc.commit_calls() == 0);
    CHECK(alloc.decommit_calls() == 0);
  }

  SECTION("ctor/dtor reserve/free virtual memory")
  {
    {
      char block[1000];
      alloc.expect_reserve(block, 1000);
      auto vmps = page_stack(num_bytes(1000));
      CHECK(alloc.reservations() == 1);
      CHECK(alloc.reserve_calls() == 1);
      CHECK(alloc.free_calls() == 0);
      CHECK(vmps.base() == block);
      CHECK(vmps.reserved_bytes() == 1000);
      CHECK(vmps.committed_bytes() == 0);
      CHECK(vmps.page_size() == 100);
      alloc.expect_free(block);
    }
    CHECK(alloc.reservations() == 0);
    CHECK(alloc.reserve_calls() == 1);
    CHECK(alloc.free_calls() == 1);
    CHECK(alloc.commit_calls() == 0);
    CHECK(alloc.decommit_calls() == 0);
  }

  SECTION("ctor taking num_pages reserves the equivalent number of bytes")
  {
    {
      char block[1000];
      alloc.expect_reserve(block, 1000);
      auto vmps = page_stack(num_pages(10));
      CHECK(alloc.reservations() == 1);
      CHECK(alloc.reserve_calls() == 1);
      CHECK(alloc.free_calls() == 0);
      CHECK(vmps.base() == block);
      CHECK(vmps.reserved_bytes() == 1000);
      CHECK(vmps.committed_bytes() == 0);
      CHECK(vmps.page_size() == 100);
      alloc.expect_free(block);
    }
    CHECK(alloc.reservations() == 0);
    CHECK(alloc.reserve_calls() == 1);
    CHECK(alloc.free_calls() == 1);
    CHECK(alloc.commit_calls() == 0);
    CHECK(alloc.decommit_calls() == 0);
  }

  SECTION("move construction")
  {
    {
      char block[1000];
      alloc.expect_reserve(block, 1000);
      auto vmps1 = page_stack(num_bytes(1000));
      alloc.expect_commit(block, 400);
      vmps1.resize(400);

      auto vmps2 = std::move(vmps1);
      CHECK(alloc.reservations() == 1);
      CHECK(alloc.reserve_calls() == 1);
      CHECK(alloc.free_calls() == 0);
      CHECK(alloc.commit_calls() == 1);
      CHECK(alloc.decommit_calls() == 0);
      CHECK(vmps1.base() == nullptr);
      CHECK(vmps2.base() == block);
      CHECK(vmps1.reserved_bytes() == 0);
      CHECK(vmps2.reserved_bytes() == 1000);
      CHECK(vmps1.committed_bytes() == 0);
      CHECK(vmps2.committed_bytes() == 400);
      alloc.expect_free(block);
    }
    CHECK(alloc.reservations() == 0);
    CHECK(alloc.reserve_calls() == 1);
    CHECK(alloc.free_calls() == 1);
    CHECK(alloc.commit_calls() == 1);
    CHECK(alloc.decommit_calls() == 0);
  }

  SECTION("move assignment")
  {
    {
      char block1[1000];
      char block2[2000];
      alloc.expect_reserve(block1, 1000);
      auto vmps1 = page_stack(num_bytes(1000));
      alloc.expect_commit(block1, 400);
      vmps1.resize(400);

      {
        alloc.expect_reserve(block2, 2000);
        auto vmps2 = page_stack(num_bytes(2000));
        alloc.expect_commit(block2, 300);
        vmps2.resize(300);

        alloc.expect_free(block1);
        vmps1 = std::move(vmps2);

        CHECK(alloc.reservations() == 1);
        CHECK(alloc.reserve_calls() == 2);
        CHECK(alloc.free_calls() == 1);
        CHECK(alloc.commit_calls() == 2);
        CHECK(alloc.decommit_calls() == 0);
        CHECK(vmps1.base() == block2);
        CHECK(vmps2.base() == nullptr);
        CHECK(vmps1.reserved_bytes() == 2000);
        CHECK(vmps2.reserved_bytes() == 0);
        CHECK(vmps1.committed_bytes() == 300);
        CHECK(vmps2.committed_bytes() == 0);
      }
      alloc.expect_free(block2);
    }
    CHECK(alloc.reservations() == 0);
    CHECK(alloc.reserve_calls() == 2);
    CHECK(alloc.free_calls() == 2);
    CHECK(alloc.commit_calls() == 2);
    CHECK(alloc.decommit_calls() == 0);
  }

  SECTION("self move assignment is a no-op")
  {
    char block[1000];
    alloc.expect_reserve(block, 1000);
    auto vmps = page_stack(num_bytes(1000));
    alloc.expect_commit(block, 400);
    vmps.resize(400);

#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wself-move"
#endif
    vmps = std::move(vmps);
#ifdef __clang__
#  pragma clang diagnostic pop
#endif

    CHECK(vmps.base() == block);
    CHECK(vmps.reserved_bytes() == 1000);
    CHECK(vmps.committed_bytes() == 400);

    CHECK(alloc.reservations() == 1);
    CHECK(alloc.reserve_calls() == 1);
    CHECK(alloc.free_calls() == 0);
    CHECK(alloc.commit_calls() == 1);
    CHECK(alloc.decommit_calls() == 0);
    alloc.expect_free(block);
  }

  SECTION("single resize() and inverse resize()")
  {
    {
      char block[1000];
      alloc.expect_reserve(block, 1000);
      auto vmps = page_stack(num_bytes(1000));
      alloc.expect_commit(block, 100);
      vmps.resize(100);
      CHECK(vmps.committed_bytes() == 100);
      CHECK(alloc.commit_calls() == 1);
      CHECK(alloc.decommit_calls() == 0);

      alloc.expect_decommit(block, 100);
      vmps.resize(0);
      CHECK(vmps.committed_bytes() == 0);
      CHECK(alloc.commit_calls() == 1);
      CHECK(alloc.decommit_calls() == 1);

      alloc.expect_free(block);
    }
    CHECK(alloc.reservations() == 0);
    CHECK(alloc.reserve_calls() == 1);
    CHECK(alloc.free_calls() == 1);
    CHECK(alloc.commit_calls() == 1);
    CHECK(alloc.decommit_calls() == 1);
  }
  SECTION("multiple growing and shrinking resize()")
  {
    {
      char block[1000];
      alloc.expect_reserve(block, 1000);
      auto vmps = page_stack(num_bytes(1000));
      alloc.expect_commit(block, 100);
      vmps.resize(100);
      CHECK(vmps.committed_bytes() == 100);
      CHECK(alloc.commit_calls() == 1);
      CHECK(alloc.decommit_calls() == 0);

      alloc.expect_commit(block + 100, 200);
      vmps.resize(300);
      CHECK(vmps.committed_bytes() == 300);
      CHECK(alloc.commit_calls() == 2);
      CHECK(alloc.decommit_calls() == 0);

      alloc.expect_decommit(block + 200, 100);
      vmps.resize(200);
      CHECK(vmps.committed_bytes() == 200);
      CHECK(alloc.commit_calls() == 2);
      CHECK(alloc.decommit_calls() == 1);

      alloc.expect_commit(block + 200, 200);
      vmps.resize(400);
      CHECK(vmps.committed_bytes() == 400);
      CHECK(alloc.commit_calls() == 3);
      CHECK(alloc.decommit_calls() == 1);

      alloc.expect_decommit(block + 300, 100);
      vmps.resize(300);
      CHECK(vmps.committed_bytes() == 300);
      CHECK(alloc.commit_calls() == 3);
      CHECK(alloc.decommit_calls() == 2);

      alloc.expect_free(block);
    }
    CHECK(alloc.reservations() == 0);
    CHECK(alloc.reserve_calls() == 1);
    CHECK(alloc.free_calls() == 1);
    CHECK(alloc.commit_calls() == 3);
    CHECK(alloc.decommit_calls() == 2);
  }
  SECTION("resize() amount is rounded up to page size")
  {
    {
      char block[1000];
      alloc.expect_reserve(block, 1000);
      auto vmps = page_stack(num_bytes(1000));
      vmps.resize(0);
      CHECK(vmps.committed_bytes() == 0);
      CHECK(alloc.commit_calls() == 0);
      CHECK(alloc.decommit_calls() == 0);

      alloc.expect_commit(block, 100);
      vmps.resize(1);
      CHECK(vmps.committed_bytes() == 100);
      CHECK(alloc.commit_calls() == 1);
      CHECK(alloc.decommit_calls() == 0);

      alloc.expect_commit(block + 100, 100);
      vmps.resize(199);
      CHECK(vmps.committed_bytes() == 200);
      CHECK(alloc.commit_calls() == 2);
      CHECK(alloc.decommit_calls() == 0);

      alloc.expect_commit(block + 200, 200);
      vmps.resize(301);
      CHECK(vmps.committed_bytes() == 400);
      CHECK(alloc.commit_calls() == 3);
      CHECK(alloc.decommit_calls() == 0);

      vmps.resize(399);
      CHECK(vmps.committed_bytes() == 400);
      CHECK(alloc.commit_calls() == 3);
      CHECK(alloc.decommit_calls() == 0);

      vmps.resize(305);
      CHECK(vmps.committed_bytes() == 400);
      CHECK(alloc.commit_calls() == 3);
      CHECK(alloc.decommit_calls() == 0);

      alloc.expect_free(block);
    }
    CHECK(alloc.reservations() == 0);
    CHECK(alloc.reserve_calls() == 1);
    CHECK(alloc.free_calls() == 1);
    CHECK(alloc.commit_calls() == 3);
    CHECK(alloc.decommit_calls() == 0);
  }

  SECTION("swap")
  {
    {
      char block1[1000];
      char block2[2000];
      alloc.expect_reserve(block1, 1000);
      auto vmps1 = page_stack(num_bytes(1000));
      alloc.expect_commit(block1, 400);
      vmps1.resize(400);

      {
        alloc.expect_reserve(block2, 2000);
        auto vmps2 = page_stack(num_bytes(2000));
        alloc.expect_commit(block2, 300);
        vmps2.resize(300);

        using std::swap;
        static_assert(noexcept(swap(vmps1, vmps2)), "page_stack::swap() is not noexcept");

        swap(vmps1, vmps2);
        CHECK(alloc.reservations() == 2);
        CHECK(alloc.reserve_calls() == 2);
        CHECK(alloc.free_calls() == 0);
        CHECK(alloc.commit_calls() == 2);
        CHECK(alloc.decommit_calls() == 0);
        CHECK(vmps1.base() == block2);
        CHECK(vmps2.base() == block1);
        CHECK(vmps1.reserved_bytes() == 2000);
        CHECK(vmps2.reserved_bytes() == 1000);
        CHECK(vmps1.committed_bytes() == 300);
        CHECK(vmps2.committed_bytes() == 400);

        alloc.expect_free(block1);
      }
      alloc.expect_free(block2);
    }
    CHECK(alloc.reservations() == 0);
    CHECK(alloc.reserve_calls() == 2);
    CHECK(alloc.free_calls() == 2);
    CHECK(alloc.commit_calls() == 2);
    CHECK(alloc.decommit_calls() == 0);
  }
}
