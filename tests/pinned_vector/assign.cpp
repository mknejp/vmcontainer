//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "vmcontainer/pinned_vector.hpp"

#include "catch.hpp"

#include <forward_list>
#include <iterator>
#include <list>
#include <sstream>

using namespace mknejp::vmcontainer;

TEST_CASE("pinned_vector::assign() with an iterator pair", "[pinned_vector][cons]")
{
  auto test = [](auto first, auto last, auto expected) {
    auto v = pinned_vector<int>(max_elements(10), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

    v.assign(first, last);
    CHECK(v.size() == expected.size());
    CHECK(v.empty() == false);
    CHECK(std::equal(v.begin(), v.end(), begin(expected), end(expected)));
  };
  auto const expected = {10, 11, 12, 13, 14};
  SECTION("input iterator")
  {
    auto init = std::istringstream("10 11 12 13 14");
    test(std::istream_iterator<int>(init), std::istream_iterator<int>(), expected);
  }
  SECTION("forward iterator")
  {
    auto init = std::forward_list<int>{10, 11, 12, 13, 14};
    test(begin(init), end(init), expected);
  }
  SECTION("bidirectional iterator")
  {
    auto init = std::list<int>{10, 11, 12, 13, 14};
    test(begin(init), end(init), expected);
  }
  SECTION("random access iterator")
  {
    auto init = {10, 11, 12, 13, 14};
    test(begin(init), end(init), expected);
  }
}

TEST_CASE("pinned_vector::assign() with a count and value", "[pinned_vector][cons]")
{
  auto v = pinned_vector<int>(max_elements(10), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

  v.assign(5, 6);
  CHECK(v.size() == 5);
  CHECK(v.empty() == false);
  CHECK(std::distance(v.begin(), v.end()) == 5);
  CHECK(std::all_of(v.begin(), v.end(), [](int x) { return x == 6; }));
}

TEST_CASE("pinned_vector::assign() with an initializer_list", "[pinned_vector][cons]")
{
  auto v = pinned_vector<int>(max_elements(10), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

  auto replacement = {10, 11, 12, 13, 14};
  v.assign(replacement);

  CHECK(v.size() == 5);
  CHECK(v.empty() == false);
  CHECK(std::equal(v.begin(), v.end(), begin(replacement), end(replacement)));
}
