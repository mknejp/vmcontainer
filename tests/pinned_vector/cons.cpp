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
#include <type_traits>

using namespace mknejp::vmcontainer;

static_assert(std::is_nothrow_default_constructible<pinned_vector<int>>::value, "");
static_assert(std::is_nothrow_move_constructible<pinned_vector<int>>::value, "");
static_assert(std::is_nothrow_move_assignable<pinned_vector<int>>::value, "");

TEST_CASE("pinned_vector.cons")
{
  SECTION("a default cosntructed container")
  {
    auto v = pinned_vector<int>();

    CHECK(v.empty() == true);
    CHECK(v.size() == 0);
    CHECK(v.max_size() == 0);
    CHECK(v.capacity() == 0);
  }

  SECTION("construction from list_initializer")
  {
    auto init = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto v = pinned_vector<int>(num_elements{init.size()}, init);

    CHECK(v.size() == init.size());
    CHECK(v.empty() == false);
    CHECK(std::equal(v.begin(), v.end(), init.begin(), init.end()));
  }

  SECTION("construction from iterator pair")
  {
    auto test = [](std::size_t n, auto first, auto last, auto expected_first, auto expected_last) {
      auto v = pinned_vector<int>(num_elements{n}, first, last);
      CHECK(v.size() == n);
      CHECK(v.empty() == false);
      CHECK(std::equal(v.begin(), v.end(), expected_first, expected_last));
    };
    auto const expected = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    SECTION("input iterator")
    {
      auto init = std::istringstream("0 1 2 3 4 5 6 7 8 9");
      test(10, std::istream_iterator<int>(init), std::istream_iterator<int>(), expected.begin(), expected.end());
    }
    SECTION("forward iterator")
    {
      auto init = std::forward_list<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      test(10, init.begin(), init.end(), expected.begin(), expected.end());
    }
    SECTION("bidirectional iterator")
    {
      auto init = std::list<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      test(10, init.begin(), init.end(), expected.begin(), expected.end());
    }
    SECTION("random access iterator")
    {
      auto init = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      test(10, init.begin(), init.end(), expected.begin(), expected.end());
    }
  }

  SECTION("construction from count and value")
  {
    auto v = pinned_vector<int>(num_elements{10}, 10, 5);

    CHECK(v.size() == 10);
    CHECK(v.empty() == false);
    CHECK(std::distance(v.begin(), v.end()) == 10);
    CHECK(std::all_of(v.begin(), v.end(), [](int x) { return x == 5; }));
  }

  SECTION("construction from count")
  {
    auto v = pinned_vector<int>(num_elements{10}, 10);

    CHECK(v.size() == 10);
    CHECK(v.empty() == false);
    CHECK(std::distance(v.begin(), v.end()) == 10);
    CHECK(std::all_of(v.begin(), v.end(), [](int x) { return x == 0; }));
  }

  SECTION("copy construction")
  {
    auto a = pinned_vector<int>(num_elements{10}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    auto b = a;

    CHECK(a.size() == b.size());
    CHECK(a.empty() == b.empty());
    CHECK(std::equal(a.begin(), a.end(), b.begin(), b.end()));
  }

  SECTION("move construction")
  {
    auto a = pinned_vector<int>(num_elements{10}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    auto first = a.begin();
    auto last = a.end();

    auto b = std::move(a);

    CHECK(a.size() == 0);
    CHECK(b.size() == 10);
    CHECK(a.empty() == true);
    CHECK(b.empty() == false);
    CHECK(b.begin() == first);
    CHECK(b.end() == last);
  }
}