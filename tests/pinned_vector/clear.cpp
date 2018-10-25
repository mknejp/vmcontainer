//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "pinned_vector_test.hpp"

#include "catch.hpp"

#include <type_traits>
#include <vector>

using namespace mknejp::vmcontainer;
using namespace vmcontainer_test;

TEST_CASE("pinned_vector::clear() empties the container", "[pinned_vector][clear]")
{
  auto vec = pinned_vector<int>(max_elements(10), {1, 2, 3, 4, 5, 6, 7, 8, 9, 0});

  REQUIRE(vec.size() == 10);

  static_assert(noexcept(vec.clear()), "");
  vec.clear();

  REQUIRE(vec.size() == 0);
  REQUIRE(vec.empty() == true);
  REQUIRE(vec.begin() == vec.end());
}

TEST_CASE("pinned_vector::clear() does not change capacity", "[pinned_vector][clear]")
{
  auto vec = pinned_vector<int>(max_elements(10), {1, 2, 3, 4, 5, 6, 7, 8, 9, 0});

  auto const capacity = vec.capacity();

  vec.clear();

  REQUIRE(vec.capacity() == capacity);
}

TEST_CASE("pinned_vector::clear() destroys its elements", "[pinned_vector][clear]")
{
  static std::vector<std::uintptr_t> constructed;
  static std::vector<std::uintptr_t> destroyed;

  struct tracker
  {
    tracker() { constructed.push_back(reinterpret_cast<std::uintptr_t>(this)); }
    ~tracker() { destroyed.push_back(reinterpret_cast<std::uintptr_t>(this)); }
  };

  auto vec = pinned_vector<tracker>(max_elements(10), 10);

  REQUIRE(vec.size() == 10);
  REQUIRE(constructed.size() == 10);

  vec.clear();
  REQUIRE(std::equal(constructed.begin(), constructed.end(), destroyed.begin(), destroyed.end()));
}
