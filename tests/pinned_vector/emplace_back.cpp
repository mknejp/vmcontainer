//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "pinned_vector_test.hpp"

#include "catch.hpp"

using namespace mknejp::vmcontainer;
using namespace vmcontainer_test;

TEST_CASE("pinned_vector::emplace_back()", "[pinned_vector][emplace_back]")
{
  auto v = pinned_vector<int>(num_elements{10});

  int& a = v.emplace_back(1);
  REQUIRE(v.size() == 1);
  REQUIRE(std::addressof(a) == std::addressof(v.back()));
  REQUIRE(v[0] == 1);
  REQUIRE(v.end() - v.begin() == 1);

  int& b = v.emplace_back(2);
  REQUIRE(v.size() == 2);
  REQUIRE(std::addressof(b) == std::addressof(v.back()));
  REQUIRE(v[1] == 2);
  REQUIRE(v.end() - v.begin() == 2);

  int& c = v.emplace_back(3);
  REQUIRE(v.size() == 3);
  REQUIRE(std::addressof(c) == std::addressof(v.back()));
  REQUIRE(v[2] == 3);
  REQUIRE(v.end() - v.begin() == 3);
}

TEST_CASE("pinned_vector::emplace_back() with CopyConstructible", "[pinned_vector][emplace_back]")
{
  struct CopyConstructible
  {
    explicit CopyConstructible(int x) : x(x) {}
    int x;
  };

  auto v = pinned_vector<CopyConstructible>(num_elements{10});

  auto x = CopyConstructible(1);

  CopyConstructible& a = v.emplace_back(x);
  REQUIRE(v.size() == 1);
  REQUIRE(std::addressof(a) == std::addressof(v.back()));
  REQUIRE(v[0].x == 1);
  REQUIRE(v.end() - v.begin() == 1);

  x.x = 2;
  CopyConstructible& b = v.emplace_back(x);
  REQUIRE(v.size() == 2);
  REQUIRE(std::addressof(b) == std::addressof(v.back()));
  REQUIRE(v[1].x == 2);
  REQUIRE(v.end() - v.begin() == 2);

  x.x = 3;
  CopyConstructible& c = v.emplace_back(x);
  REQUIRE(v.size() == 3);
  REQUIRE(std::addressof(c) == std::addressof(v.back()));
  REQUIRE(v[2].x == 3);
  REQUIRE(v.end() - v.begin() == 3);
}

TEST_CASE("pinned_vector::emplace_back() with MoveConstructible", "[pinned_vector][emplace_back]")
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

  MoveConstructible& a = v.emplace_back(std::move(x));
  REQUIRE(v.size() == 1);
  REQUIRE(std::addressof(a) == std::addressof(v.back()));
  REQUIRE(v[0].x == 1);
  REQUIRE(v.end() - v.begin() == 1);

  x.x = 2;
  MoveConstructible& b = v.emplace_back(std::move(x));
  REQUIRE(v.size() == 2);
  REQUIRE(std::addressof(b) == std::addressof(v.back()));
  REQUIRE(v[1].x == 2);
  REQUIRE(v.end() - v.begin() == 2);

  x.x = 3;
  MoveConstructible& c = v.emplace_back(std::move(x));
  REQUIRE(v.size() == 3);
  REQUIRE(std::addressof(c) == std::addressof(v.back()));
  REQUIRE(v[2].x == 3);
  REQUIRE(v.end() - v.begin() == 3);
}

TEST_CASE("pinned_vector::emplace_back() with DefaultConstructible", "[pinned_vector][emplace_back]")
{
  static int counter = 1;

  struct DefaultConstructible
  {
    int x = counter++;
  };

  auto v = pinned_vector<DefaultConstructible>(num_elements{10});

  DefaultConstructible& a = v.emplace_back();
  REQUIRE(v.size() == 1);
  REQUIRE(std::addressof(a) == std::addressof(v.back()));
  REQUIRE(v[0].x == 1);
  REQUIRE(v.end() - v.begin() == 1);

  DefaultConstructible& b = v.emplace_back();
  REQUIRE(v.size() == 2);
  REQUIRE(std::addressof(b) == std::addressof(v.back()));
  REQUIRE(v[1].x == 2);
  REQUIRE(v.end() - v.begin() == 2);

  DefaultConstructible& c = v.emplace_back();
  REQUIRE(v.size() == 3);
  REQUIRE(std::addressof(c) == std::addressof(v.back()));
  REQUIRE(v[2].x == 3);
  REQUIRE(v.end() - v.begin() == 3);
}

namespace
{
  struct check_emplace_arg_forwarding
  {
    template<typename L, typename R, typename CL, typename CR, typename VL, typename VR, typename CVL, typename CVR>
    check_emplace_arg_forwarding(L&& l, R&& r, CL&& cl, CR&& cr, VL&& vl, VR&& vr, CVL&& cvl, CVR&& cvr)
    {
      static_assert(std::is_same<decltype(l), int&>::value, "");
      static_assert(std::is_same<decltype(r), int&&>::value, "");
      static_assert(std::is_same<decltype(cl), int const&>::value, "");
      static_assert(std::is_same<decltype(cr), int const&&>::value, "");
      static_assert(std::is_same<decltype(vl), int volatile&>::value, "");
      static_assert(std::is_same<decltype(vr), int volatile&&>::value, "");
      static_assert(std::is_same<decltype(cvl), int const volatile&>::value, "");
      static_assert(std::is_same<decltype(cvr), int const volatile&&>::value, "");
      REQUIRE(l == 1);
      REQUIRE(r == 2);
      REQUIRE(cl == 3);
      REQUIRE(cr == 4);
      REQUIRE(vl == 5);
      REQUIRE(vr == 6);
      REQUIRE(cvl == 7);
      REQUIRE(cvr == 8);
    }
  };
}

TEST_CASE("pinned_vector::emplace_back() forwards arguments to value constructor", "[pinned_vector][emplace_back]")
{
  auto v = pinned_vector<check_emplace_arg_forwarding>(num_elements{10});

  auto a = 1;
  auto b = 2;
  auto const c = 3;
  auto const d = 4;
  auto volatile e = 5;
  auto volatile f = 6;
  auto const volatile g = 7;
  auto const volatile h = 8;
  v.emplace_back(a, std::move(b), c, std::move(d), e, std::move(f), g, std::move(h));
}

TEST_CASE("pinned_vector::emplace_back() with NotMoveConstructibleNotCopyConstructible",
          "[pinned_vector][emplace_back]")
{
  struct NotMoveConstructibleNotCopyConstructible
  {
    explicit NotMoveConstructibleNotCopyConstructible(int x) : x(x) {}
    NotMoveConstructibleNotCopyConstructible(NotMoveConstructibleNotCopyConstructible const&) = delete;
    NotMoveConstructibleNotCopyConstructible(NotMoveConstructibleNotCopyConstructible&&) = delete;
    int x;
  };

  auto v = pinned_vector<NotMoveConstructibleNotCopyConstructible>(num_elements{10});

  NotMoveConstructibleNotCopyConstructible& a = v.emplace_back(1);
  REQUIRE(v.size() == 1);
  REQUIRE(std::addressof(a) == std::addressof(v.back()));
  REQUIRE(v[0].x == 1);
  REQUIRE(v.end() - v.begin() == 1);

  NotMoveConstructibleNotCopyConstructible& b = v.emplace_back(2);
  REQUIRE(v.size() == 2);
  REQUIRE(std::addressof(b) == std::addressof(v.back()));
  REQUIRE(v[1].x == 2);
  REQUIRE(v.end() - v.begin() == 2);

  NotMoveConstructibleNotCopyConstructible& c = v.emplace_back(3);
  REQUIRE(v.size() == 3);
  REQUIRE(std::addressof(c) == std::addressof(v.back()));
  REQUIRE(v[2].x == 3);
  REQUIRE(v.end() - v.begin() == 3);
}

TEST_CASE("pinned_vector::emplace_back() has strong exception guarantee on throwing copy construction",
          "[pinned_vector][emplace_back]")
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

  v.emplace_back(x);
  v.emplace_back(x);
  REQUIRE(v.size() == 2);

  auto state = capture_value_state(v);
  x.should_throw = true;
  REQUIRE_THROWS_AS(v.emplace_back(x), int);

  REQUIRE(capture_value_state(v) == state);
}

TEST_CASE("pinned_vector::emplace_back() has strong exception guarantee on throwing move construction",
          "[pinned_vector][emplace_back]")
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

  v.emplace_back(std::move(x));
  v.emplace_back(std::move(x));
  REQUIRE(v.size() == 2);

  auto state = capture_value_state(v);
  x.should_throw = true;
  REQUIRE_THROWS_AS(v.emplace_back(std::move(x)), int);

  REQUIRE(capture_value_state(v) == state);
}

TEST_CASE("pinned_vector::emplace_back() has strong exception guarantee on throwing construction",
          "[pinned_vector][emplace_back]")
{
  static bool should_throw = false;

  struct may_throw_when_constructed
  {
    may_throw_when_constructed()
    {
      if(should_throw)
      {
        throw 1;
      }
    }
  };

  auto v = pinned_vector<may_throw_when_constructed>(num_elements{10});

  v.emplace_back();
  v.emplace_back();
  REQUIRE(v.size() == 2);

  auto state = capture_value_state(v);
  should_throw = true;
  REQUIRE_THROWS_AS(v.emplace_back(), int);

  REQUIRE(capture_value_state(v) == state);
}

TEST_CASE("pinned_vector::emplace_back() has strong exception guarantee if committing new memory fails",
          "[pinned_vector][emplace_back]")
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

  v.emplace_back(1);
  v.emplace_back(2);
  v.emplace_back(3);
  v.emplace_back(4);
  REQUIRE(v.size() == 4);
  REQUIRE(v.capacity() == 4);

  alloc.expect_commit_and_fail(&page[0] + sizeof(page), sizeof(page));
  auto state = capture_value_state(v);
  REQUIRE_THROWS_AS(v.emplace_back(5), std::bad_alloc);

  REQUIRE(v.capacity() == 4);
  REQUIRE(capture_value_state(v) == state);
}
