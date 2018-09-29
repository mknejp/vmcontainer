//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "vmcontainer/pinned_vector.hpp"

#include "catch.hpp"

#include <algorithm>
#include <memory>
#include <type_traits>

using namespace mknejp::vmcontainer;

SCENARIO("pinned_vector is a contiguous container", "[pinned_vector]")
{
  auto check = [](auto const& container) {
    THEN("their elements are laid out in contiguous memory")
    {
      using container_t = std::decay_t<decltype(container)>;
      for(int i = 0; i < container.size(); ++i)
      {
        CAPTURE(i);
        auto const& x = *(std::addressof(*container.begin()) + i);
        REQUIRE(x == *(container.begin() + typename container_t::difference_type(i)));
        REQUIRE(x == *(container.data() + i));
      }
    }
  };

  GIVEN("an empty container") { check(pinned_vector<int>()); }
  GIVEN("a small container") { check(pinned_vector<int>(num_elements{10}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9})); }
  GIVEN("a large container spanning multiple pages")
  {
    auto v = pinned_vector<int>(num_pages{5});
    std::generate_n(std::back_inserter(v), v.max_size(), [i = 0]() mutable { return i++; });
    check(v);
  }
}
