//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "vmcontainer/pinned_vector.hpp"

#include "catch.hpp"

#include <type_traits>

using namespace mknejp::vmcontainer;

TEST_CASE("pinned_vector::at()", "[pinned_vector][access]")
{
  auto v = pinned_vector<int>(max_elements(10), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto const& cv = v;

  GIVEN("an in-range index")
  {
    auto i = 5;
    THEN("the returned reference refers to the element at that index")
    {
      auto& r = v.at(i);
      auto& cr = cv.at(i);

      CHECK(std::addressof(r) == v.data() + i);
      CHECK(std::addressof(cr) == v.data() + i);

      static_assert(std::is_same<decltype(r), int&>::value, "");
      static_assert(std::is_same<decltype(cr), int const&>::value, "");
    }
  }

  GIVEN("an out-of-range index")
  {
    auto i = v.size();
    THEN("an exception of type std::out_of_range is thrown")
    {
      CHECK_THROWS_AS(v.at(i), std::out_of_range);
      CHECK_THROWS_AS(cv.at(i), std::out_of_range);
    }
  }
}

TEST_CASE("pinned_vector::operator[]()", "[pinned_vector][access]")
{
  auto v = pinned_vector<int>(max_elements(10), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto const& cv = v;

  GIVEN("an in-range index")
  {
    auto i = 5;
    THEN("the returned reference refers to the element at that index")
    {
      auto& r = v[i];
      auto& cr = cv[i];

      CHECK(std::addressof(r) == v.data() + i);
      CHECK(std::addressof(cr) == v.data() + i);

      static_assert(std::is_same<decltype(r), int&>::value, "");
      static_assert(std::is_same<decltype(cr), int const&>::value, "");
    }
  }
}

TEST_CASE("pinned_vector::front()", "[pinned_vector][access]")
{
  auto v = pinned_vector<int>(max_elements(10), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto const& cv = v;

  auto& r = v.front();
  auto& cr = cv.front();

  CHECK(std::addressof(r) == v.data());
  CHECK(std::addressof(cr) == v.data());

  static_assert(std::is_same<decltype(r), int&>::value, "");
  static_assert(std::is_same<decltype(cr), int const&>::value, "");
}

TEST_CASE("pinned_vector::back()", "[pinned_vector][access]")
{
  auto v = pinned_vector<int>(max_elements(10), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto const& cv = v;

  auto& r = v.back();
  auto& cr = cv.back();

  CHECK(std::addressof(r) == v.data() + 9);
  CHECK(std::addressof(cr) == v.data() + 9);

  static_assert(std::is_same<decltype(r), int&>::value, "");
  static_assert(std::is_same<decltype(cr), int const&>::value, "");
}

TEST_CASE("pinned_vector::data()", "[pinned_vector][access]")
{
  auto v = pinned_vector<int>(max_elements(10), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto const& cv = v;

  auto p = v.data();
  auto cp = cv.data();

  static_assert(std::is_same<decltype(p), int*>::value, "");
  static_assert(std::is_same<decltype(cp), int const*>::value, "");

  CHECK(p != nullptr);
  CHECK(cp != nullptr);
  CHECK(p == cp);
}
