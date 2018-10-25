//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "pinned_vector_test.hpp"

#include "catch.hpp"

#include <forward_list>
#include <iterator>
#include <list>
#include <sstream>
#include <type_traits>
#include <vector>

using namespace mknejp::vmcontainer;
using namespace vmcontainer_test;

static auto round_up(std::size_t bytes, std::size_t page_size)
{
  return ((bytes + page_size - 1) / page_size) * page_size;
}

static_assert(std::is_nothrow_default_constructible<pinned_vector<int>>::value, "");
static_assert(std::is_nothrow_move_constructible<pinned_vector<int>>::value, "");
static_assert(std::is_nothrow_move_assignable<pinned_vector<int>>::value, "");

TEST_CASE("a default constructed pinned_vector is empty", "[pinned_vector][special]")
{
  auto v = pinned_vector<int>();
  CHECK(v.empty() == true);
  CHECK(v.size() == 0);
  CHECK(v.max_size() == 0);
  CHECK(v.capacity() == 0);
}

TEST_CASE("pinned_vector construction creates appropriate max_size", "[pinned_vector][special]")
{
  SECTION("num_elements")
  {
    auto v = pinned_vector<int>(max_elements(12345));

    auto page_size = vm::system_default::page_size();
    // rounded up to page size
    REQUIRE(page_size > 0);
    CAPTURE(page_size);
    auto max_size = round_up(12345 * sizeof(int), page_size) / sizeof(int);

    CHECK(v.max_size() == max_size);
  }
  SECTION("num_bytes")
  {
    auto v = pinned_vector<int>(max_bytes(12345));

    auto page_size = vm::system_default::page_size();
    // rounded up to page size
    REQUIRE(page_size > 0);
    CAPTURE(page_size);
    auto max_size = round_up(12345, page_size) / sizeof(int);

    CHECK(v.max_size() == max_size);
  }
  SECTION("num_pages")
  {
    auto v = pinned_vector<int>(max_pages(10));

    auto page_size = vm::system_default::page_size();
    REQUIRE(page_size > 0);
    CAPTURE(page_size);
    auto max_size = 10 * page_size / sizeof(int);

    CHECK(v.max_size() == max_size);
  }
}

TEST_CASE("pinned_vector construction from an initializer_list", "[pinned_vector][special]")
{
  auto init = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  auto v = pinned_vector<int>(max_elements(init.size()), init);

  CHECK(v.size() == init.size());
  CHECK(v.empty() == false);
  CHECK(std::equal(v.begin(), v.end(), init.begin(), init.end()));
}

TEST_CASE("pinned_vector construction from an iterator pair", "[pinned_vector][special]")
{
  auto test = [](auto first, auto last, auto expected) {
    auto v = pinned_vector<int>(max_elements(expected.size()), first, last);
    CHECK(v.size() == expected.size());
    CHECK(v.empty() == false);
    CHECK(std::equal(v.begin(), v.end(), begin(expected), end(expected)));
  };
  auto const expected = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  SECTION("input iterator")
  {
    auto init = std::istringstream("0 1 2 3 4 5 6 7 8 9");
    test(std::istream_iterator<int>(init), std::istream_iterator<int>(), expected);
  }
  SECTION("forward iterator")
  {
    auto init = std::forward_list<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    test(begin(init), end(init), expected);
  }
  SECTION("bidirectional iterator")
  {
    auto init = std::list<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    test(begin(init), end(init), expected);
  }
  SECTION("random access iterator")
  {
    auto init = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    test(begin(init), end(init), expected);
  }
}

TEST_CASE("pinned_vector construction from a count and value", "[pinned_vector][special]")
{
  auto v = pinned_vector<int>(max_elements(10), 10, 5);

  CHECK(v.size() == 10);
  CHECK(v.empty() == false);
  CHECK(std::distance(v.begin(), v.end()) == 10);
  CHECK(std::all_of(v.begin(), v.end(), [](int x) { return x == 5; }));
}

TEST_CASE("pinned_vector construction from a count uses only default constructor", "[pinned_vector][special]")
{
  static std::vector<void*> constructed;

  struct default_constructible
  {
    default_constructible() { constructed.push_back(this); }
    default_constructible(default_constructible const&) = delete;
    default_constructible& operator=(default_constructible const&) = delete;
  };

  auto v = pinned_vector<default_constructible>(max_elements(10), 10);

  CHECK(constructed.size() == 10);
  CHECK(v.size() == 10);
  CHECK(v.empty() == false);

  for(std::size_t i = 0; i < v.size(); ++i)
  {
    CAPTURE(i);
    REQUIRE(constructed[i] == &v[i]);
  }
}

TEST_CASE("pinned_vector constructed with elements has capacity rounded up to page size", "[pinned_vector][special]")
{
  SECTION("count and value")
  {
    auto v = pinned_vector<int>(max_elements(12345), 50, 1);
    CAPTURE(v.page_size());
    CHECK(v.capacity() == round_up(50 * sizeof(int), v.page_size()) / sizeof(int));
  }
  SECTION("count")
  {
    auto v = pinned_vector<int>(max_elements(12345), 1234);
    CAPTURE(v.page_size());
    CHECK(v.capacity() == round_up(1234 * sizeof(int), v.page_size()) / sizeof(int));
  }
  SECTION("initializer_list")
  {
    auto v = pinned_vector<int>(max_elements(12345), {1, 2, 3, 4, 5, 6});
    CAPTURE(v.page_size());
    CHECK(v.capacity() == round_up(6 * sizeof(int), v.page_size()) / sizeof(int));
  }
  SECTION("iterator pair")
  {
    auto init = {1, 2, 3};
    auto v = pinned_vector<int>(max_elements(12345), begin(init), end(init));
    CAPTURE(v.page_size());
    CHECK(v.capacity() == round_up(3 * sizeof(int), v.page_size()) / sizeof(int));
  }
}

TEST_CASE("pinned_vector copy construction", "[pinned_vector][special]")
{
  auto a = pinned_vector<int>(max_elements(10), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto b = a;

  CHECK(a.size() == b.size());
  CHECK(a.empty() == b.empty());
  CHECK(std::equal(a.begin(), a.end(), b.begin(), b.end()));
}

TEST_CASE("pinned_vector copy assignment", "[pinned_vector][special]")
{
  auto a = pinned_vector<int>(max_elements(10), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto b = pinned_vector<int>();
  b = a;

  CHECK(a.size() == b.size());
  CHECK(a.empty() == b.empty());
  CHECK(std::equal(a.begin(), a.end(), b.begin(), b.end()));
}

TEST_CASE("pinned_vector move construction", "[pinned_vector][special]")
{
  auto a = pinned_vector<int>(max_elements(10), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto a_state = capture_value_state(a);

  auto b = std::move(a);

  REQUIRE(a.size() == 0);
  REQUIRE(a.empty() == true);
  REQUIRE(capture_value_state(b) == a_state);
}

TEST_CASE("pinned_vector move assignment", "[pinned_vector][special]")
{
  auto a = pinned_vector<int>(max_elements(10), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto a_state = capture_value_state(a);

  auto b = pinned_vector<int>();
  b = std::move(a);

  REQUIRE(a.size() == 0);
  REQUIRE(a.empty() == true);
  REQUIRE(capture_value_state(b) == a_state);
}

TEST_CASE("pinned_vector assignment operator with initializer_list", "[pinned_vector][special]")
{
  auto v = pinned_vector<int>(max_elements(10), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

  v = {10, 11, 12, 13, 14};

  CHECK(v.size() == 5);
  CHECK(v.empty() == false);
  CHECK(v[0] == 10);
  CHECK(v[1] == 11);
  CHECK(v[2] == 12);
  CHECK(v[3] == 13);
  CHECK(v[4] == 14);
}

TEST_CASE("pinned_vector::swap()", "[pinned_vector][special]")
{
  auto init_a = {1, 2, 3, 4, 5};
  auto init_b = {6, 7, 8, 9};

  auto a = pinned_vector<int>(max_elements(5), init_a);
  auto b = pinned_vector<int>(max_elements(4), init_b);

  auto a_state = capture_value_state(a);
  auto b_state = capture_value_state(b);

  REQUIRE(a.size() == 5);
  REQUIRE(b.size() == 4);

  SECTION("free swap()")
  {
    swap(a, b);
    static_assert(noexcept(swap(a, b)), "pinned_vector swap() is noexcept");
  }
  SECTION("member swap()")
  {
    a.swap(b);
    static_assert(noexcept(a.swap(b)), "pinned_vector swap() is noexcept");
  }

  REQUIRE(capture_value_state(a) == b_state);
  REQUIRE(capture_value_state(b) == a_state);

  REQUIRE(std::equal(a.begin(), a.end(), begin(init_b), end(init_b)));
  REQUIRE(std::equal(b.begin(), b.end(), begin(init_a), end(init_a)));
}
