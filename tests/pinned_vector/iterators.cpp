//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "vmcontainer/pinned_vector.hpp"

#include "catch.hpp"

#include <type_traits>

using mknejp::vmcontainer::pinned_vector;

TEST_CASE("pinned_vector iterators compare equal for empty containers", "[pinned_vector][iterators]")
{
  auto test = [](pinned_vector<int>& v, pinned_vector<int> const& cv) {
    THEN("begin() and end() compare equal")
    {
      auto begin = v.begin();
      auto end = v.end();
      auto cbegin = cv.begin();
      auto cend = cv.end();

      static_assert(std::is_same<pinned_vector<int>::iterator, decltype(begin)>(), "");
      static_assert(std::is_same<pinned_vector<int>::iterator, decltype(end)>(), "");
      static_assert(std::is_same<pinned_vector<int>::const_iterator, decltype(cbegin)>(), "");
      static_assert(std::is_same<pinned_vector<int>::const_iterator, decltype(cend)>(), "");

      REQUIRE(begin == end);
      REQUIRE(cbegin == cend);
      REQUIRE(std::distance(begin, end) == 0);
      REQUIRE(std::distance(cbegin, cend) == 0);
    }
    THEN("cbegin() and cend() compare equal")
    {
      auto begin = v.cbegin();
      auto end = v.cend();
      auto cbegin = cv.cbegin();
      auto cend = cv.cend();

      static_assert(std::is_same<pinned_vector<int>::const_iterator, decltype(begin)>(), "");
      static_assert(std::is_same<pinned_vector<int>::const_iterator, decltype(end)>(), "");
      static_assert(std::is_same<pinned_vector<int>::const_iterator, decltype(cbegin)>(), "");
      static_assert(std::is_same<pinned_vector<int>::const_iterator, decltype(cend)>(), "");

      REQUIRE(begin == end);
      REQUIRE(cbegin == cend);
      REQUIRE(std::distance(begin, end) == 0);
      REQUIRE(std::distance(cbegin, cend) == 0);
    }
    THEN("rbegin() and rend() compare equal")
    {
      auto begin = v.rbegin();
      auto end = v.rend();
      auto cbegin = cv.rbegin();
      auto cend = cv.rend();

      static_assert(std::is_same<pinned_vector<int>::reverse_iterator, decltype(begin)>(), "");
      static_assert(std::is_same<pinned_vector<int>::reverse_iterator, decltype(end)>(), "");
      static_assert(std::is_same<pinned_vector<int>::const_reverse_iterator, decltype(cbegin)>(), "");
      static_assert(std::is_same<pinned_vector<int>::const_reverse_iterator, decltype(cend)>(), "");

      REQUIRE(begin == end);
      REQUIRE(cbegin == cend);
      REQUIRE(std::distance(begin, end) == 0);
      REQUIRE(std::distance(cbegin, cend) == 0);
    }
    THEN("crbegin() and crend() compare equal")
    {
      auto begin = v.crbegin();
      auto end = v.crend();
      auto cbegin = cv.crbegin();
      auto cend = cv.crend();

      static_assert(std::is_same<pinned_vector<int>::const_reverse_iterator, decltype(begin)>(), "");
      static_assert(std::is_same<pinned_vector<int>::const_reverse_iterator, decltype(end)>(), "");
      static_assert(std::is_same<pinned_vector<int>::const_reverse_iterator, decltype(cbegin)>(), "");
      static_assert(std::is_same<pinned_vector<int>::const_reverse_iterator, decltype(cend)>(), "");

      REQUIRE(begin == end);
      REQUIRE(cbegin == cend);
      REQUIRE(std::distance(begin, end) == 0);
      REQUIRE(std::distance(cbegin, cend) == 0);
    }
    THEN("mutable and const iterators compare equal")
    {
      REQUIRE(v.begin() == v.cbegin());
      REQUIRE(v.end() == v.cend());
      REQUIRE(v.rbegin() == v.crbegin());
      REQUIRE(v.rend() == v.crend());

      REQUIRE(cv.begin() == cv.cbegin());
      REQUIRE(cv.end() == cv.cend());
      REQUIRE(cv.rbegin() == cv.crbegin());
      REQUIRE(cv.rend() == cv.crend());
    }
  };

  GIVEN("an empty container")
  {
    auto v = pinned_vector<int>(1);
    auto const cv = pinned_vector<int>(1);
    test(v, cv);
  }
  GIVEN("a default-constructed container")
  {
    auto v = pinned_vector<int>();
    auto const cv = pinned_vector<int>();
    test(v, cv);
  }
  GIVEN("a moved-from container")
  {
    auto v = pinned_vector<int>(1);
    auto sink = std::move(v);
    test(v, pinned_vector<int>());
  }
}

TEST_CASE("pinned_vector singular iterators compare equal", "[pinned_vector][iterators]")
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
