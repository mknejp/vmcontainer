//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "vmcontainer/pinned_vector.hpp"

#include "catch.hpp"

using namespace mknejp::vmcontainer;

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

TEST_CASE("pinned_vector capacity grows geometrically", "[pinned_vector][capacity]")
{
  auto v = pinned_vector<int>(num_pages{10});
  auto page_size = v.page_size();
  REQUIRE(page_size > 0);
  CAPTURE(page_size);

  auto const ints_per_page = page_size / sizeof(int);

  std::fill_n(std::back_inserter(v), 1 * ints_per_page, 1);
  REQUIRE(v.capacity() == 1 * ints_per_page);

  std::fill_n(std::back_inserter(v), 1 * ints_per_page, 1);
  REQUIRE(v.capacity() == 2 * ints_per_page);

  std::fill_n(std::back_inserter(v), 1 * ints_per_page, 1);
  REQUIRE(v.capacity() == 4 * ints_per_page);

  std::fill_n(std::back_inserter(v), 2 * ints_per_page, 1);
  REQUIRE(v.capacity() == 8 * ints_per_page);
}

TEST_CASE("pinned_vector::resize()", "[pinned_vector][capacity]")
{
  SECTION("resize() without a default value")
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

  SECTION("resize() with a default value")
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
}
