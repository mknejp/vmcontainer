//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "vmcontainer/detail.hpp"

#include <catch.hpp>

#include <type_traits>

using namespace mknejp::vmcontainer;
using namespace detail;

///////////////////////////////////////////////////////////////////////////////
// value_init_when_moved_from
//

TEST_CASE("detail::value_init_when_moved_from")
{
  static_assert(std::is_nothrow_move_constructible<value_init_when_moved_from<int>>::value, "");
  static_assert(std::is_nothrow_move_assignable<value_init_when_moved_from<int>>::value, "");

  GIVEN("a default constructed value_init_when_moved_from")
  {
    auto x = value_init_when_moved_from<int>();
    auto p = value_init_when_moved_from<int*>();
    THEN("its value compares equal to a value-initialized value_type")
    {
      REQUIRE(x.value == int{});
      REQUIRE(p.value == (int*){});
      REQUIRE(p.value == nullptr);
    }
  }

  GIVEN("a value_init_when_moved_from object initialized with a value")
  {
    auto x = value_init_when_moved_from<int>(5);

    THEN("its value compares equal to the same value") { REQUIRE(x.value == 5); }

    THEN("it implicitly converts to its value_type")
    {
      int i = x;
      REQUIRE(i == 5);
    }

    THEN("it can be implicitly assigned from its value_type")
    {
      x = 10;
      REQUIRE(x.value == 10);
    }

    WHEN("moved from via move-construction")
    {
      auto y = std::move(x);

      THEN("its value compares equal to a value-initialized value_type") { REQUIRE(x.value == int{}); }
      THEN("the newly constructed object contains the original value") { REQUIRE(y.value == 5); }
    }

    WHEN("moved from via move-assignment")
    {
      auto y = value_init_when_moved_from<int>();
      y = std::move(x);

      THEN("its value compares equal to a value-initialized value_type") { REQUIRE(x.value == int{}); }
      THEN("the assigned-to object contains the original value") { REQUIRE(y.value == 5); }
    }

    WHEN("copied via copy-construction")
    {
      auto y = x;

      THEN("its value doesn't change") { REQUIRE(x.value == 5); }
      THEN("the newly constructed object contains an equivalent value") { REQUIRE(y.value == 5); }
      THEN("the two objects compare equal") { REQUIRE(x == y); }
    }

    WHEN("copied from via copy-assignment")
    {
      auto y = value_init_when_moved_from<int>();
      y = x;

      THEN("its value doesn't change") { REQUIRE(x.value == 5); }
      THEN("the newly constructed object contains an equivalent value") { REQUIRE(y.value == 5); }
      THEN("the two objects compare equal") { REQUIRE(x == y); }
    }
  }
}
