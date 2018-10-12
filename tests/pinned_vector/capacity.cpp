//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "pinned_vector_test.hpp"

#include "catch.hpp"

using namespace mknejp::vmcontainer;
using namespace vmcontainer_test;

static auto round_up = [](std::size_t bytes, std::size_t page_size) {
  return ((bytes + page_size - 1) / page_size) * page_size;
};

TEST_CASE("pinned_vector::page_size() returns the system page size", "[pinned_vector][capacity]")
{
  auto v = pinned_vector<int>();
  CHECK(v.page_size() == vm::default_vm_traits::page_size());
}

TEST_CASE("pinned_vector::reserve() grows the capacity", "[pinned_vector][capacity]")
{
  auto v = pinned_vector<int>(num_pages{10});
  auto page_size = v.page_size();
  REQUIRE(page_size > 0);
  CAPTURE(page_size);

  auto test = [&v, page_size](std::size_t n) {
    CAPTURE(n);
    auto new_cap = n * page_size / sizeof(int);
    v.reserve(new_cap);
    CHECK(v.capacity() == new_cap);
  };

  test(1);
  test(2);
  test(3);
  test(4);
}

TEST_CASE("pinned_vector::reserve() grows in multiples of page size", "[pinned_vector][capacity]")
{
  auto v = pinned_vector<int>(num_pages{10});
  auto page_size = v.page_size();
  REQUIRE(page_size > 0);
  CAPTURE(page_size);

  auto test = [&v, page_size](std::size_t n) {
    CAPTURE(n);
    v.reserve(n);
    auto expected = round_up(n * sizeof(int), page_size) / sizeof(int);
    CHECK(v.capacity() == expected);
  };
  test(1);
  test(page_size / sizeof(int) + 1);
}

TEST_CASE("pinned_vector::reserve() does not reduce capacity", "[pinned_vector][capacity]")
{
  auto v = pinned_vector<int>(num_pages{2});
  auto page_size = v.page_size();
  REQUIRE(page_size > 0);
  CAPTURE(page_size);

  v.reserve(2 * page_size / sizeof(int));
  auto old_capacity = v.capacity();

  v.reserve(1);
  CHECK(v.capacity() == old_capacity);
}

TEST_CASE("pinned_vector::reserve() has strong exception guarantee if committing new memory fails",
          "[pinned_vector][capacity]")
{
  struct tag
  {};
  auto alloc = tracking_allocator<tag>();
  using traits = pinned_vector_test_traits<decltype(alloc)>;

  char page[4 * sizeof(int)];
  alloc.set_page_size(sizeof(page));
  alloc.expect_reserve(page, 2 * sizeof(page));
  alloc.expect_commit(page, sizeof(page));
  alloc.expect_free(page);

  auto v = pinned_vector<int, traits>(num_pages{2});
  REQUIRE(v.max_size() == 8);
  REQUIRE(v.size() == 0);

  v.push_back(1);
  v.push_back(2);
  REQUIRE(v.size() == 2);
  REQUIRE(v.capacity() == 4);

  alloc.expect_commit_and_fail(&page[0] + sizeof(page), sizeof(page));
  auto state = capture_value_state(v);
  REQUIRE_THROWS_AS(v.reserve(5), std::bad_alloc);
  REQUIRE(v.capacity() == 4);
  REQUIRE(capture_value_state(v) == state);
}

TEST_CASE("pinned_vector::shrink_to_fit() reduces capacity to current size rounded up to page size",
          "[pinned_vector][capacity]")
{
  auto v = pinned_vector<int>(num_pages{2}, {1});
  auto page_size = v.page_size();
  REQUIRE(page_size > 0);
  CAPTURE(page_size);

  v.reserve(2 * page_size / sizeof(int));
  CHECK(v.capacity() == 2 * page_size / sizeof(int));

  v.shrink_to_fit();
  CHECK(v.capacity() == 1 * page_size / sizeof(int));

  v.clear();
  v.shrink_to_fit();
  CHECK(v.capacity() == 0);
}

TEST_CASE("pinned_vector::empty()", "[pinned_vector][capacity]")
{
  auto v = pinned_vector<int>(num_elements{10});
  REQUIRE(v.empty() == true);
  v.push_back(1);
  REQUIRE(v.empty() == false);
  v.clear();
  REQUIRE(v.empty() == true);
}

TEST_CASE("pinned_vector capacity grows geometrically according to growth_factor", "[pinned_vector][capacity]")
{
  auto test = [](auto factor) {
    struct traits
    {
      using page_stack = pinned_vector_traits::page_stack;
      using growth_factor = decltype(factor);
    };

    auto v = pinned_vector<int, traits>(num_pages{100});
    auto page_size = v.page_size();
    REQUIRE(page_size > 0);
    CAPTURE(page_size);

    auto next_cap = [page_size](auto old_cap) {
      return (old_cap == 0
                ? page_size
                : round_up(old_cap * sizeof(int) * traits::growth_factor::num / traits::growth_factor::den, page_size))
             / sizeof(int);
    };

    auto old_cap = v.capacity();
    std::fill_n(std::back_inserter(v), page_size / sizeof(int), 1);
    REQUIRE(v.capacity() == next_cap(old_cap));

    for(int i = 0; i < 5; ++i)
    {
      old_cap = v.capacity();
      std::fill_n(std::back_inserter(v), v.capacity() - v.size() + 1, 1);
      REQUIRE(v.capacity() == next_cap(old_cap));
    }
  };
  SECTION("default growth factor 2x")
  {
    static_assert(std::is_same<pinned_vector_traits::growth_factor, std::ratio<2, 1>>::value,
                  "default growth factor is not 2x");
    test(pinned_vector_traits::growth_factor());
  }
  SECTION("custom growth factor 1.5x") { test(std::ratio<3, 2>()); }
}

TEST_CASE("pinned_vector::resize() without a default value", "[pinned_vector][capacity]")
{
  auto v = pinned_vector<int>(num_elements{12345});
  REQUIRE(v.size() == 0);

  v.resize(10);
  REQUIRE(v.size() == 10);
  REQUIRE(v.capacity() >= 10);

  v.resize(20);
  REQUIRE(v.size() == 20);
  REQUIRE(v.capacity() >= 20);

  v.resize(15);
  REQUIRE(v.size() == 15);
  REQUIRE(v.capacity() >= 20);
}

TEST_CASE("pinned_vector::resize() with a default value", "[pinned_vector][capacity]")
{
  auto v = pinned_vector<int>(num_elements{12345});
  REQUIRE(v.size() == 0);

  v.resize(10);
  REQUIRE(v.size() == 10);
  REQUIRE(v.capacity() >= 10);
  REQUIRE(std::all_of(v.begin(), v.end(), [](int x) { return x == 0; }));

  v.resize(20, 1);
  REQUIRE(v.size() == 20);
  REQUIRE(v.capacity() >= 20);
  REQUIRE(std::all_of(v.begin(), v.begin() + 10, [](int x) { return x == 0; }));
  REQUIRE(std::all_of(v.begin() + 10, v.begin() + 20, [](int x) { return x == 1; }));

  v.resize(15, 2);
  REQUIRE(v.size() == 15);
  REQUIRE(v.capacity() >= 15);
  REQUIRE(std::all_of(v.begin(), v.begin() + 10, [](int x) { return x == 0; }));
  REQUIRE(std::all_of(v.begin() + 10, v.begin() + 15, [](int x) { return x == 1; }));

  v.resize(30, 3);
  REQUIRE(v.size() == 30);
  REQUIRE(v.capacity() >= 30);
  REQUIRE(std::all_of(v.begin(), v.begin() + 10, [](int x) { return x == 0; }));
  REQUIRE(std::all_of(v.begin() + 10, v.begin() + 15, [](int x) { return x == 1; }));
  REQUIRE(std::all_of(v.begin() + 15, v.begin() + 30, [](int x) { return x == 3; }));
}

TEST_CASE("pinned_vector::resize() has strong exception guarantee if committing new memory fails",
          "[pinned_vector][capacity]")
{
  struct tag
  {};
  auto alloc = tracking_allocator<tag>();
  using traits = pinned_vector_test_traits<decltype(alloc)>;

  char page[4 * sizeof(int)];
  alloc.set_page_size(sizeof(page));
  alloc.expect_reserve(page, 2 * sizeof(page));
  alloc.expect_commit(page, sizeof(page));
  alloc.expect_free(page);

  auto v = pinned_vector<int, traits>(num_pages{2});
  REQUIRE(v.max_size() == 8);
  REQUIRE(v.size() == 0);

  v.resize(1);
  REQUIRE(v.size() == 1);
  REQUIRE(v.capacity() == 4);

  alloc.expect_commit_and_fail(&page[0] + sizeof(page), sizeof(page));
  auto state = capture_value_state(v);
  REQUIRE_THROWS_AS(v.resize(5), std::bad_alloc);
  REQUIRE(v.capacity() == 4);
  REQUIRE(capture_value_state(v) == state);
}
