//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "vmcontainer/pinned_vector.hpp"

#include <string>

using namespace mknejp::vmcontainer;

namespace
{
  struct regular
  {
  public:
    regular() = default;
    regular(regular const&) = default;
    regular(regular&&) = default;
    auto operator=(regular const&) -> regular& = default;
    auto operator=(regular &&) -> regular& = default;
  };

  struct movable_only
  {
  public:
    movable_only() = default;
    movable_only(movable_only const&) = delete;
    movable_only(movable_only&&) = default;
    auto operator=(movable_only const&) -> movable_only& = delete;
    auto operator=(movable_only &&) -> movable_only& = default;
  };

  struct immovable
  {
  public:
    immovable() = default;
    immovable(immovable const&) = delete;
    immovable(immovable&&) = delete;
    auto operator=(immovable const&) -> immovable& = delete;
    auto operator=(immovable &&) -> immovable& = delete;
  };
}

template<typename T>
static void required_functions()
{
  // ctor
  auto a = pinned_vector<T>(max_bytes(100));
  // move ctor
  auto b = std::move(a);
  // move assign
  a = std::move(b);

  // member functions which are always required otherwise the container is pointless
  a.clear();

  // swap
  swap(a, b); // ADL
  a.swap(b);

  static_assert(std::is_same<decltype(a.begin()), typename pinned_vector<T>::iterator>::value, "");
  static_assert(std::is_same<decltype(a.cbegin()), typename pinned_vector<T>::const_iterator>::value, "");
  static_assert(std::is_same<decltype(a.rbegin()), typename pinned_vector<T>::reverse_iterator>::value, "");
  static_assert(std::is_same<decltype(a.crbegin()), typename pinned_vector<T>::const_reverse_iterator>::value, "");

  static_assert(std::is_same<decltype(a.end()), typename pinned_vector<T>::iterator>::value, "");
  static_assert(std::is_same<decltype(a.cend()), typename pinned_vector<T>::const_iterator>::value, "");
  static_assert(std::is_same<decltype(a.rend()), typename pinned_vector<T>::reverse_iterator>::value, "");
  static_assert(std::is_same<decltype(a.crend()), typename pinned_vector<T>::const_reverse_iterator>::value, "");

  static_assert(std::is_same<decltype(a.emplace_back()), T&>::value, "");

  static_assert(std::is_same<decltype(a.front()), T&>::value, "");
  static_assert(std::is_same<decltype(a.back()), T&>::value, "");

  static_assert(std::is_same<decltype(a[0]), T&>::value, "");
  static_assert(std::is_same<decltype(a.at(0)), T&>::value, "");

  static_assert(std::is_same<decltype(a.data()), T*>::value, "");

  a.pop_back();

  a.reserve(typename pinned_vector<T>::size_type(1000));
  a.resize(typename pinned_vector<T>::size_type(2000));

  static_assert(std::is_same<decltype(a.empty()), bool>::value, "");
  static_assert(std::is_same<decltype(a.capacity()), typename pinned_vector<T>::size_type>::value, "");
  static_assert(std::is_same<decltype(a.size()), typename pinned_vector<T>::size_type>::value, "");
  static_assert(std::is_same<decltype(a.max_size()), typename pinned_vector<T>::size_type>::value, "");
  static_assert(std::is_same<decltype(a.page_size()), typename pinned_vector<T>::size_type>::value, "");

  const auto c = std::move(a);

  static_assert(std::is_same<decltype(c.begin()), typename pinned_vector<T>::const_iterator>::value, "");
  static_assert(std::is_same<decltype(c.cbegin()), typename pinned_vector<T>::const_iterator>::value, "");
  static_assert(std::is_same<decltype(c.rbegin()), typename pinned_vector<T>::const_reverse_iterator>::value, "");
  static_assert(std::is_same<decltype(c.crbegin()), typename pinned_vector<T>::const_reverse_iterator>::value, "");

  static_assert(std::is_same<decltype(c.end()), typename pinned_vector<T>::const_iterator>::value, "");
  static_assert(std::is_same<decltype(c.cend()), typename pinned_vector<T>::const_iterator>::value, "");
  static_assert(std::is_same<decltype(c.rend()), typename pinned_vector<T>::const_reverse_iterator>::value, "");
  static_assert(std::is_same<decltype(c.crend()), typename pinned_vector<T>::const_reverse_iterator>::value, "");

  static_assert(std::is_same<decltype(c.front()), T const&>::value, "");
  static_assert(std::is_same<decltype(c.back()), T const&>::value, "");

  static_assert(std::is_same<decltype(c[0]), T const&>::value, "");
  static_assert(std::is_same<decltype(c.at(0)), T const&>::value, "");

  static_assert(std::is_same<decltype(c.data()), T const*>::value, "");
}

template<typename T>
static void required_for_copyable()
{
  pinned_vector<T> a(max_bytes(100));
  // copy ctor
  auto b = a;
  // copy assign
  a = b;
}

template void required_functions<regular>();
template void required_functions<movable_only>();
template void required_functions<immovable>();

template void required_for_copyable<regular>();
