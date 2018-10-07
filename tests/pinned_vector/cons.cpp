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

TEST_CASE("a default constructed pinned_vector is empty", "[pinned_vector][cons]")
{
  auto v = pinned_vector<int>();
  CHECK(v.empty() == true);
  CHECK(v.size() == 0);
  CHECK(v.max_size() == 0);
  CHECK(v.capacity() == 0);
}

TEST_CASE("pinned_vector construction creates appropriate max_size", "[pinned_vector][cons]")
{
  SECTION("num_elements")
  {
    auto v = pinned_vector<int>(num_elements{12345});

    auto page_size = vm::default_vm_traits::page_size();
    // rounded up to page size
    REQUIRE(page_size > 0);
    CAPTURE(page_size);
    auto max_size = (((sizeof(int) * 12345 + page_size - 1) / page_size) * page_size) / sizeof(int);

    CHECK(v.capacity() == 0);
    CHECK(v.size() == 0);
    CHECK(v.empty() == true);
    CHECK(v.max_size() == max_size);
    CHECK(v.page_size() == page_size);
  }
  SECTION("num_bytes")
  {
    auto v = pinned_vector<int>(num_bytes{12345});

    auto page_size = vm::default_vm_traits::page_size();
    // rounded up to page size
    REQUIRE(page_size > 0);
    CAPTURE(page_size);
    auto max_size = (((12345 + page_size - 1) / page_size) * page_size) / sizeof(int);

    CHECK(v.capacity() == 0);
    CHECK(v.size() == 0);
    CHECK(v.empty() == true);
    CHECK(v.max_size() == max_size);
    CHECK(v.page_size() == page_size);
  }
  SECTION("num_pages")
  {
    auto v = pinned_vector<int>(num_pages{10});

    auto page_size = vm::default_vm_traits::page_size();
    REQUIRE(page_size > 0);
    CAPTURE(page_size);
    auto max_size = 10 * page_size / sizeof(int);

    CHECK(v.capacity() == 0);
    CHECK(v.size() == 0);
    CHECK(v.empty() == true);
    CHECK(v.max_size() == max_size);
    CHECK(v.page_size() == page_size);
  }
}

TEST_CASE("pinned_vector construction from an initializer_list", "[pinned_vector][cons]")
{
  auto init = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  auto v = pinned_vector<int>(num_elements{init.size()}, init);

  CHECK(v.size() == init.size());
  CHECK(v.empty() == false);
  CHECK(std::equal(v.begin(), v.end(), init.begin(), init.end()));
}

TEST_CASE("pinned_vector construction from an iterator pair", "[pinned_vector][cons]")
{
  auto test = [](auto first, auto last, auto expected) {
    auto v = pinned_vector<int>(num_elements{expected.size()}, first, last);
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

TEST_CASE("pinned_vector construction from a count and value", "[pinned_vector][cons]")
{
  auto v = pinned_vector<int>(num_elements{10}, 10, 5);

  CHECK(v.size() == 10);
  CHECK(v.empty() == false);
  CHECK(std::distance(v.begin(), v.end()) == 10);
  CHECK(std::all_of(v.begin(), v.end(), [](int x) { return x == 5; }));
}

TEST_CASE("pinned_vector construction from a count", "[pinned_vector][cons]")
{
  auto v = pinned_vector<int>(num_elements{10}, 10);

  CHECK(v.size() == 10);
  CHECK(v.empty() == false);
  CHECK(std::distance(v.begin(), v.end()) == 10);
  CHECK(std::all_of(v.begin(), v.end(), [](int x) { return x == 0; }));
}

TEST_CASE("pinned_vector copy construction", "[pinned_vector][cons]")
{
  auto a = pinned_vector<int>(num_elements{10}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto b = a;

  CHECK(a.size() == b.size());
  CHECK(a.empty() == b.empty());
  CHECK(std::equal(a.begin(), a.end(), b.begin(), b.end()));
}

TEST_CASE("pinned_vector copy assignment", "[pinned_vector][cons]")
{
  auto a = pinned_vector<int>(num_elements{10}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto b = pinned_vector<int>();
  b = a;

  CHECK(a.size() == b.size());
  CHECK(a.empty() == b.empty());
  CHECK(std::equal(a.begin(), a.end(), b.begin(), b.end()));
}

TEST_CASE("pinned_vector move construction", "[pinned_vector][cons]")
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

TEST_CASE("pinned_vector move assignment", "[pinned_vector][cons]")
{
  auto a = pinned_vector<int>(num_elements{10}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto first = a.begin();
  auto last = a.end();

  auto b = pinned_vector<int>();
  b = std::move(a);

  CHECK(a.size() == 0);
  CHECK(b.size() == 10);
  CHECK(a.empty() == true);
  CHECK(b.empty() == false);
  CHECK(b.begin() == first);
  CHECK(b.end() == last);
}

TEST_CASE("pinned_vector assign() with an initializer_list", "[pinned_vector][cons]")
{
  auto v = pinned_vector<int>(num_elements{10}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

  auto replacement = {10, 11, 12, 13, 14};
  v.assign(replacement);

  CHECK(v.size() == 5);
  CHECK(v.empty() == false);
  CHECK(std::equal(v.begin(), v.end(), begin(replacement), end(replacement)));
}

TEST_CASE("pinned_vector assign() with an iterator pair", "[pinned_vector][cons]")
{
  auto test = [](auto first, auto last, auto expected) {
    auto v = pinned_vector<int>(num_elements{10}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

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

TEST_CASE("pinned_vector assign() with a count and value", "[pinned_vector][cons]")
{
  auto v = pinned_vector<int>(num_elements{10}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

  v.assign(5, 6);
  CHECK(v.size() == 5);
  CHECK(v.empty() == false);
  CHECK(std::distance(v.begin(), v.end()) == 5);
  CHECK(std::all_of(v.begin(), v.end(), [](int x) { return x == 6; }));
}

TEST_CASE("pinned_vector assignment operator with initializer_list", "[pinned_vector][cons]")
{
  auto v = pinned_vector<int>(num_elements{10}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

  v = {10, 11, 12, 13, 14};

  CHECK(v.size() == 5);
  CHECK(v.empty() == false);
  CHECK(v[0] == 10);
  CHECK(v[1] == 11);
  CHECK(v[2] == 12);
  CHECK(v[3] == 13);
  CHECK(v[4] == 14);
}
