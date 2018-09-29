//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "vmcontainer/pinned_vector.hpp"

#include "catch.hpp"

#include <type_traits>

using namespace mknejp::vmcontainer;

TEST_CASE("pinned_vector.iterators")
{
  SECTION("iterators compare equal for empty containers")
  {
    auto test = [](pinned_vector<int>& v) {
      THEN("begin() and end() compare equal")
      {
        auto begin = v.begin();
        auto end = v.end();

        REQUIRE(begin == end);
        REQUIRE(std::distance(begin, end) == 0);
      }
      THEN("rbegin() and rend() compare equal")
      {
        auto begin = v.rbegin();
        auto end = v.rend();

        REQUIRE(begin == end);
        REQUIRE(std::distance(begin, end) == 0);
      }
      THEN("mutable and const iterators compare equal")
      {
        auto const& cv = v;

        REQUIRE(v.begin() == v.cbegin());
        REQUIRE(v.end() == v.cend());
        REQUIRE(v.rbegin() == v.crbegin());
        REQUIRE(v.rend() == v.crend());

        REQUIRE(cv.begin() == v.begin());
        REQUIRE(cv.end() == v.end());
        REQUIRE(cv.rbegin() == v.rbegin());
        REQUIRE(cv.rend() == v.rend());

        REQUIRE(cv.begin() == v.cbegin());
        REQUIRE(cv.end() == v.cend());
        REQUIRE(cv.rbegin() == v.crbegin());
        REQUIRE(cv.rend() == v.crend());

        REQUIRE(cv.begin() == cv.cbegin());
        REQUIRE(cv.end() == cv.cend());
        REQUIRE(cv.rbegin() == cv.crbegin());
        REQUIRE(cv.rend() == cv.crend());
      }
    };

    GIVEN("an empty container")
    {
      auto v = pinned_vector<int>(num_elements{1});
      test(v);
    }
    GIVEN("a default-constructed container")
    {
      auto v = pinned_vector<int>();
      test(v);
    }
    GIVEN("a moved-from container")
    {
      auto v = pinned_vector<int>(num_elements{1});
      auto _ = std::move(v);
      test(v);
    }
  }

  SECTION("singular iterators compare equal")
  {
    GIVEN("default constructed iterators")
    {
      auto i1 = pinned_vector<int>::iterator();
      auto i2 = pinned_vector<int>::iterator();
      auto i3 = i2;
      auto c1 = pinned_vector<int>::const_iterator();

      THEN("they compare equal")
      {
        REQUIRE(i1 == i2);
        REQUIRE(i2 == i3);
        REQUIRE(i1 == c1);
        REQUIRE(c1 == i1);

        REQUIRE(!(i1 != i2));
        REQUIRE(!(i1 != c1));
        REQUIRE(!(c1 != i1));

        REQUIRE(!(i1 < i2));
        REQUIRE(!(i1 < c1));
        REQUIRE(!(c1 < i1));

        REQUIRE(!(i1 > i2));
        REQUIRE(!(i1 > c1));
        REQUIRE(!(c1 > i1));

        REQUIRE(i1 <= i2);
        REQUIRE(i1 <= c1);
        REQUIRE(c1 <= i1);

        REQUIRE(i1 >= i2);
        REQUIRE(i1 >= c1);
        REQUIRE(c1 >= i1);

        REQUIRE((i1 - i2) == 0);
        REQUIRE((i1 - c1) == 0);
        REQUIRE((c1 - i1) == 0);
      }
    }
  }
}
