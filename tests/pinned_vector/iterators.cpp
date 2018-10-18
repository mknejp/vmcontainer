//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "vmcontainer/pinned_vector.hpp"

#include "catch.hpp"

#include <type_traits>

using namespace mknejp::vmcontainer;

TEST_CASE("pinned_vector iterators compare equal for empty containers", "[pinned_vector][iterator]")
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
    auto v = pinned_vector<int>(max_elements(1));
    test(v);
  }
  GIVEN("a default-constructed container")
  {
    auto v = pinned_vector<int>();
    test(v);
  }
  GIVEN("a moved-from container")
  {
    auto v = pinned_vector<int>(max_elements(1));
    auto _ = std::move(v);
    test(v);
  }
}

TEST_CASE("pinned_vector container iteration", "[pinned_vector][iterator]")
{
  GIVEN("a non-empty container")
  {
    auto init = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto v = pinned_vector<int>(max_elements(10), init);
    auto const& cv = v;

    THEN("begin() and end() iterate through all values")
    {
      CHECK(std::equal(v.begin(), v.end(), begin(init), end(init)));
      CHECK(std::equal(v.cbegin(), v.cend(), begin(init), end(init)));
      CHECK(std::equal(cv.begin(), cv.end(), begin(init), end(init)));
      CHECK(std::equal(cv.cbegin(), cv.cend(), begin(init), end(init)));
    }

    THEN("rbegin() and rend() iterate through all values in reverse order")
    {
      CHECK(std::equal(v.rbegin(), v.rend(), rbegin(init), rend(init)));
      CHECK(std::equal(v.crbegin(), v.crend(), rbegin(init), rend(init)));
      CHECK(std::equal(cv.rbegin(), cv.rend(), rbegin(init), rend(init)));
      CHECK(std::equal(cv.crbegin(), cv.crend(), rbegin(init), rend(init)));
    }
  }
}

TEST_CASE("pinned_vector singular iterators compare equal", "[pinned_vector][iterator]")
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
