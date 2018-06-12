//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "pinned_vector.hpp"

#include "catch.hpp"

#include <functional>
#include <type_traits>

static_assert(
  std::is_base_of<mknejp::detail::_pinned_vector::virtual_memory_reservation<>, mknejp::virtual_memory_reservation>(),
  "");

namespace
{
  template<typename Tag>
  struct mock_allocator
  {
    static std::function<auto(std::size_t)->void*> reserve;
    static std::function<auto(void*)->void> free;
    static std::function<auto(void*, std::size_t)->void*> commit;
    static std::function<auto(void*, std::size_t)->void*> decommit;
    static std::function<auto()->size_t> page_size;
  };

  template<typename Tag>
  std::function<auto(std::size_t)->void*> mock_allocator<Tag>::reserve;
  template<typename Tag>
  std::function<auto(void*)->void> mock_allocator<Tag>::free;
  template<typename Tag>
  std::function<auto(void*, std::size_t)->void*> mock_allocator<Tag>::commit;
  template<typename Tag>
  std::function<auto(void*, std::size_t)->void*> mock_allocator<Tag>::decommit;
  template<typename Tag>
  std::function<auto()->size_t> mock_allocator<Tag>::page_size;
}

TEST_CASE("virtual_memory_reservation")
{
  struct Tag;

  auto reserved = std::size_t(0);
  auto freed = 0;
  char base[100];

  auto set_reserve_base = [&](void* reserve_base) {
    mock_allocator<Tag>::reserve = [&, reserve_base](std::size_t num_bytes) {
      reserved += num_bytes;
      return reserve_base;
    };
  };
  auto set_free_base = [&](void* reserve_base) {
    mock_allocator<Tag>::free = [&, reserve_base](void* p) {
      REQUIRE(reserve_base == p);
      ++freed;
    };
  };

  set_reserve_base(base);
  set_free_base(base);

  SECTION("ctor/dtor reserve/free virtual memory")
  {
    {
      auto vmr = mknejp::detail::_pinned_vector::virtual_memory_reservation<mock_allocator<Tag>>(100);
      REQUIRE(reserved == 100);
      REQUIRE(freed == 0);
      REQUIRE(vmr.base() == base);
      REQUIRE(vmr.reserved_bytes() == 100);
    }
    REQUIRE(freed == 1);
  }

  SECTION("copy construction duplicates virtual memory reservaton")
  {
    {
      auto vmr1 = mknejp::detail::_pinned_vector::virtual_memory_reservation<mock_allocator<Tag>>(100);
      REQUIRE(reserved == 100);

      {
        // Give the copy a new reservation
        char base2[100];
        set_reserve_base(base2);
        set_free_base(base2);

        auto vmr2 = vmr1;
        REQUIRE(reserved == 200);
        REQUIRE(vmr2.base() == base2);
        REQUIRE(vmr2.reserved_bytes() == 100);
      }
      // Ensure the original is freeing the old reservation
      set_free_base(base);
    }
    REQUIRE(freed == 2);
  }

  SECTION("copy assignment duplicates virtual memory reservaton")
  {
    {
      auto vmr1 = mknejp::detail::_pinned_vector::virtual_memory_reservation<mock_allocator<Tag>>(100);
      REQUIRE(reserved == 100);

      // Give the copy a new reservation
      char base2[100];
      {
        auto vmr2 = mknejp::detail::_pinned_vector::virtual_memory_reservation<mock_allocator<Tag>>(200);
        REQUIRE(freed == 0);
        REQUIRE(reserved == 300);

        // vmr1 gets a new reservation but must free the previous one
        set_reserve_base(base2);

        vmr1 = vmr2;
        REQUIRE(freed == 1);
        REQUIRE(reserved == 500);
        REQUIRE(vmr1.base() == base2);
        REQUIRE(vmr1.reserved_bytes() == 200);
      }
      // vmr1 now frees the new reservation
      set_free_base(base2);
    }
    REQUIRE(freed == 3);
  }

  SECTION("move construction does not affect reservations")
  {
    {
      auto vmr1 = mknejp::detail::_pinned_vector::virtual_memory_reservation<mock_allocator<Tag>>(100);
      REQUIRE(reserved == 100);
      REQUIRE(freed == 0);

      auto vmr2 = std::move(vmr1);
      REQUIRE(reserved == 100);
      REQUIRE(freed == 0);
      REQUIRE(vmr1.base() == nullptr);
      REQUIRE(vmr1.reserved_bytes() == 0);
      REQUIRE(vmr2.base() == base);
      REQUIRE(vmr2.reserved_bytes() == 100);

      set_free_base(base);
    }
    REQUIRE(freed == 1);
  }

  SECTION("move assignment does not affect reservations")
  {
    {
      auto vmr1 = mknejp::detail::_pinned_vector::virtual_memory_reservation<mock_allocator<Tag>>(100);
      REQUIRE(reserved == 100);
      REQUIRE(freed == 0);
      auto vmr2 = mknejp::detail::_pinned_vector::virtual_memory_reservation<mock_allocator<Tag>>(200);
      REQUIRE(reserved == 300);
      REQUIRE(freed == 0);
      vmr1 = std::move(vmr2);
      REQUIRE(freed == 1);
      REQUIRE(reserved == 300);
      REQUIRE(vmr1.base() == base);
      REQUIRE(vmr1.reserved_bytes() == 200);
      REQUIRE(vmr2.base() == nullptr);
      REQUIRE(vmr2.reserved_bytes() == 0);
    }
    REQUIRE(freed == 2);
  }
}
