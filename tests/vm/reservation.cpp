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

static_assert(std::is_base_of<detail::reservation<vm::default_vm_traits>, vm::reservation>(), "");

static_assert(std::is_nothrow_move_constructible<vm::reservation>::value, "");
static_assert(std::is_nothrow_move_assignable<vm::reservation>::value, "");

TEST_CASE("vm/reservation")
{
  struct Tag
  {};
  using virtual_memory_system_stub = pinned_vector_test::virtual_memory_system_stub<Tag>;
  auto alloc = pinned_vector_test::tracking_allocator<Tag>();

  virtual_memory_system_stub::page_size = [] { return 100; };

  using reservation = detail::reservation<virtual_memory_system_stub>;

  SECTION("default constructed has no reservation")
  {
    {
      char block[100];
      alloc.expect_reserve(block, 100);
      auto vmr = reservation();
      CHECK(vmr.base() == nullptr);
      CHECK(vmr.reserved_bytes() == 0);
      alloc.expect_free(block);
    }
    CHECK(alloc.reservations() == 0);
    CHECK(alloc.reserve_calls() == 0);
    CHECK(alloc.free_calls() == 0);
  }

  SECTION("ctor/dtor reserve/free virtual memory")
  {
    {
      char block[100];
      alloc.expect_reserve(block, 100);
      auto vmr = reservation(100);
      CHECK(alloc.reservations() == 1);
      CHECK(alloc.reserve_calls() == 1);
      CHECK(alloc.free_calls() == 0);
      CHECK(vmr.base() == block);
      CHECK(vmr.reserved_bytes() == 100);
      alloc.expect_free(block);
    }
    CHECK(alloc.reservations() == 0);
    CHECK(alloc.reserve_calls() == 1);
    CHECK(alloc.free_calls() == 1);
  }

  SECTION("round up reservation to page size")
  {
    char block[100];
    alloc.expect_reserve(block, 100);
    auto vmr = reservation(1);
    CHECK(vmr.base() == block);
    CHECK(vmr.reserved_bytes() == 100);
    alloc.expect_free(block);
  }

  SECTION("move construction")
  {
    {
      char block[100];
      alloc.expect_reserve(block, 100);
      auto vmr1 = reservation(100);

      auto vmr2 = std::move(vmr1);
      CHECK(alloc.reservations() == 1);
      CHECK(alloc.reserve_calls() == 1);
      CHECK(alloc.free_calls() == 0);
      CHECK(vmr1.base() == nullptr);
      CHECK(vmr1.reserved_bytes() == 0);
      CHECK(vmr2.base() == block);
      CHECK(vmr2.reserved_bytes() == 100);
      alloc.expect_free(block);
    }
    CHECK(alloc.reservations() == 0);
    CHECK(alloc.reserve_calls() == 1);
    CHECK(alloc.free_calls() == 1);
  }

  SECTION("move assignment")
  {
    {
      char block1[100];
      char block2[200];
      alloc.expect_reserve(block1, 100);
      auto vmr1 = reservation(100);

      alloc.expect_free(block1);
      alloc.expect_reserve(block2, 200);
      auto vmr2 = reservation(200);

      vmr1 = std::move(vmr2);
      CHECK(alloc.reservations() == 1);
      CHECK(alloc.reserve_calls() == 2);
      CHECK(alloc.free_calls() == 1);
      CHECK(vmr1.base() == block2);
      CHECK(vmr1.reserved_bytes() == 200);
      CHECK(vmr2.base() == nullptr);
      CHECK(vmr2.reserved_bytes() == 0);
      alloc.expect_free(block2);
    }
    CHECK(alloc.reservations() == 0);
    CHECK(alloc.reserve_calls() == 2);
    CHECK(alloc.free_calls() == 2);
  }

  SECTION("self move assignment is a no-op")
  {
    char block[100];
    alloc.expect_reserve(block, 100);
    auto vmr1 = reservation(100);

#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wself-move"
#endif
    vmr1 = std::move(vmr1);
#ifdef __clang__
#  pragma clang diagnostic pop
#endif

    CHECK(vmr1.base() == block);
    CHECK(vmr1.reserved_bytes() == 100);

    CHECK(alloc.reservations() == 1);
    CHECK(alloc.reserve_calls() == 1);
    CHECK(alloc.free_calls() == 0);
    alloc.expect_free(block);
  }

  SECTION("swap")
  {
    {
      char block1[100];
      char block2[200];
      alloc.expect_reserve(block1, 100);
      auto vmr1 = reservation(100);
      {
        alloc.expect_reserve(block2, 200);
        auto vmr2 = reservation(200);

        using std::swap;
        static_assert(noexcept(swap(vmr1, vmr2)), "reservation::swap() is not noexcept");

        swap(vmr1, vmr2);
        CHECK(alloc.reservations() == 2);
        CHECK(alloc.reserve_calls() == 2);
        CHECK(alloc.free_calls() == 0);
        CHECK(vmr1.base() == block2);
        CHECK(vmr2.base() == block1);
        CHECK(vmr1.reserved_bytes() == 200);
        CHECK(vmr2.reserved_bytes() == 100);
        alloc.expect_free(block1);
      }
      alloc.expect_free(block2);
    }
    CHECK(alloc.reservations() == 0);
    CHECK(alloc.reserve_calls() == 2);
    CHECK(alloc.free_calls() == 2);
  }
}
