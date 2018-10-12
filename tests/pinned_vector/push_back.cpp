//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "pinned_vector_test.hpp"

#include "catch.hpp"

using namespace mknejp::vmcontainer;
using namespace vmcontainer_test;

TEST_CASE("pinned_vector::push_back()", "[pinned_vector][push_back]")
{
  auto v = pinned_vector<int>(num_elements{10});

  v.push_back(1);
  REQUIRE(v.size() == 1);
  REQUIRE(v[0] == 1);

  v.push_back(2);
  REQUIRE(v.size() == 2);
  REQUIRE(v[1] == 2);

  v.push_back(3);
  REQUIRE(v.size() == 3);
  REQUIRE(v[2] == 3);
}

TEST_CASE("pinned_vector::push_back() with CopyConstructible", "[pinned_vector][push_back]")
{
  struct CopyConstructible
  {
    explicit CopyConstructible(int x) : x(x) {}
    int x;
  };

  auto v = pinned_vector<CopyConstructible>(num_elements{10});

  auto x = CopyConstructible(1);

  v.push_back(x);
  REQUIRE(v.size() == 1);
  REQUIRE(v[0].x == 1);

  x.x = 2;
  v.push_back(x);
  REQUIRE(v.size() == 2);
  REQUIRE(v[1].x == 2);

  x.x = 3;
  v.push_back(x);
  REQUIRE(v.size() == 3);
  REQUIRE(v[2].x == 3);
}

TEST_CASE("pinned_vector::push_back() with MoveConstructible", "[pinned_vector][push_back]")
{
  struct MoveConstructible
  {
    explicit MoveConstructible(int x) : x(x) {}
    MoveConstructible(MoveConstructible const&) = delete;
    MoveConstructible(MoveConstructible&&) = default;
    int x;
  };

  auto v = pinned_vector<MoveConstructible>(num_elements{10});

  auto x = MoveConstructible(1);

  v.push_back(std::move(x));
  REQUIRE(v.size() == 1);
  REQUIRE(v[0].x == 1);

  x.x = 2;
  v.push_back(std::move(x));
  REQUIRE(v.size() == 2);
  REQUIRE(v[1].x == 2);

  x.x = 3;
  v.push_back(std::move(x));
  REQUIRE(v.size() == 3);
  REQUIRE(v[2].x == 3);
}

TEST_CASE("pinned_vector::push_back() has strong exception guarantee on throwing copy construction",
          "[pinned_vector][push_back]")
{
  struct may_throw_when_copied
  {
    may_throw_when_copied() = default;
    may_throw_when_copied(may_throw_when_copied const& other)
    {
      if(other.should_throw)
      {
        throw 1;
      }
    }
    bool should_throw = false;
  };

  auto v = pinned_vector<may_throw_when_copied>(num_elements{10});

  auto x = may_throw_when_copied();

  v.push_back(x);
  v.push_back(x);
  REQUIRE(v.size() == 2);
  x.should_throw = true;

  auto state = capture_value_state(v);
  REQUIRE_THROWS_AS(v.push_back(x), int);

  REQUIRE(capture_value_state(v) == state);
}

TEST_CASE("pinned_vector::push_back() has strong exception guarantee on throwing move construction",
          "[pinned_vector][push_back]")
{
  struct may_throw_when_copied
  {
    may_throw_when_copied() = default;
    may_throw_when_copied(may_throw_when_copied&& other)
    {
      if(other.should_throw)
      {
        throw 1;
      }
    }
    bool should_throw = false;
  };

  auto v = pinned_vector<may_throw_when_copied>(num_elements{10});

  auto x = may_throw_when_copied();

  v.push_back(std::move(x));
  v.push_back(std::move(x));
  REQUIRE(v.size() == 2);
  x.should_throw = true;

  auto state = capture_value_state(v);
  REQUIRE_THROWS_AS(v.push_back(std::move(x)), int);

  REQUIRE(capture_value_state(v) == state);
}

TEST_CASE("pinned_vector::push_back() has strong exception guarantee if committing new memory fails",
          "[pinned_vector][push_back]")
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

  v.push_back(1);
  v.push_back(2);
  v.push_back(3);
  v.push_back(4);
  REQUIRE(v.size() == 4);
  REQUIRE(v.capacity() == 4);

  alloc.expect_commit_and_fail(&page[0] + sizeof(page), sizeof(page));
  auto state = capture_value_state(v);
  REQUIRE_THROWS_AS(v.push_back(5), std::bad_alloc);

  REQUIRE(v.capacity() == 4);
  REQUIRE(capture_value_state(v) == state);
}
