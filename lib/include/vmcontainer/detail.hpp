//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace mknejp
{
  namespace vmcontainer
  {
    class max_size_t;
    class reservation_size_t;

    constexpr auto max_elements(std::size_t n) noexcept -> max_size_t;
    constexpr auto max_bytes(std::size_t n) noexcept -> max_size_t;
    constexpr auto max_pages(std::size_t n) noexcept -> max_size_t;

    constexpr auto num_bytes(std::size_t n) noexcept -> reservation_size_t;
    constexpr auto num_pages(std::size_t n) noexcept -> reservation_size_t;

    namespace detail
    {
      // simple utility type that stores a value of type T and when moved from assigns to it a value-initialized object.
      template<typename T>
      struct value_init_when_moved_from;

      constexpr auto round_up(std::size_t num_bytes, std::size_t page_size) noexcept -> std::size_t;

      template<typename T, typename... Args>
      auto construct_at(T* p, Args&&... args) -> T*;
      // C++17 algorithms
      template<typename T>
      auto destroy_at(T* p) -> void;
      template<typename ForwardIt>
      auto destroy(ForwardIt first, ForwardIt last) -> void;
      template<typename InputIt, typename ForwardIt>
      auto uninitialized_move(InputIt first, InputIt last, ForwardIt d_first) -> ForwardIt;
      template<typename InputIt, typename ForwardIt>
      auto uninitialized_move_n(InputIt first, std::size_t count, ForwardIt d_first) -> std::pair<InputIt, ForwardIt>;
      template<typename ForwardIt>
      auto uninitialized_value_construct_n(ForwardIt first, std::size_t count) -> ForwardIt;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// reservation_size_t
//

class mknejp::vmcontainer::reservation_size_t
{
public:
  constexpr auto num_bytes(std::size_t page_size) const noexcept
  {
    switch(_unit)
    {
      case unit::bytes:
        return _count;
      case unit::pages:
        return _count * page_size;
    }
    // TODO: mark as unreachable
    // assert(false);
    return std::size_t(0);
  }

private:
  friend constexpr auto num_bytes(std::size_t n) noexcept -> reservation_size_t;
  friend constexpr auto num_pages(std::size_t n) noexcept -> reservation_size_t;

  enum class unit
  {
    pages,
    bytes,
  };

  constexpr reservation_size_t(unit unit, std::size_t count) noexcept : _unit(unit), _count(count) {}

  unit _unit = unit::bytes;
  std::size_t _count = 0;
};

constexpr auto mknejp::vmcontainer::num_bytes(std::size_t n) noexcept -> reservation_size_t
{
  return {reservation_size_t::unit::bytes, n};
}
constexpr auto mknejp::vmcontainer::num_pages(std::size_t n) noexcept -> reservation_size_t
{
  return {reservation_size_t::unit::pages, n};
}

///////////////////////////////////////////////////////////////////////////////
// max_size_t
//

class mknejp::vmcontainer::max_size_t
{
public:
  template<typename T>
  constexpr auto scaled_for_type() const noexcept -> reservation_size_t
  {
    switch(_unit)
    {
      case unit::elements:
        return num_bytes(sizeof(T) * _count);
      case unit::bytes:
        return num_bytes(_count);
      case unit::pages:
        return num_pages(_count);
    }
    // TODO: mark as unreachable
    // assert(false);
    return num_pages(0);
  }

private:
  friend constexpr auto max_elements(std::size_t n) noexcept -> max_size_t;
  friend constexpr auto max_bytes(std::size_t n) noexcept -> max_size_t;
  friend constexpr auto max_pages(std::size_t n) noexcept -> max_size_t;

  enum class unit
  {
    elements,
    pages,
    bytes,
  };

  constexpr max_size_t(unit unit, std::size_t count) noexcept : _unit(unit), _count(count) {}

  unit _unit = unit::bytes;
  std::size_t _count = 0;
};

constexpr auto mknejp::vmcontainer::max_elements(std::size_t n) noexcept -> max_size_t
{
  return {max_size_t::unit::elements, n};
}
constexpr auto mknejp::vmcontainer::max_bytes(std::size_t n) noexcept -> max_size_t
{
  return {max_size_t::unit::bytes, n};
}
constexpr auto mknejp::vmcontainer::max_pages(std::size_t n) noexcept -> max_size_t
{
  return {max_size_t::unit::pages, n};
}

///////////////////////////////////////////////////////////////////////////////
// value_init_when_moved_from
//

template<typename T>
struct mknejp::vmcontainer::detail::value_init_when_moved_from
{
  static_assert(std::is_trivial<T>::value, "");

  using value_type = T;

  value_init_when_moved_from() = default;
  /*implicit*/ value_init_when_moved_from(T value) noexcept : value(value) {}
  value_init_when_moved_from(value_init_when_moved_from const& other) = default;
  value_init_when_moved_from(value_init_when_moved_from&& other) noexcept : value(other.value) { other.value = T{}; }
  value_init_when_moved_from& operator=(value_init_when_moved_from const& other) & = default;
  value_init_when_moved_from& operator=(value_init_when_moved_from&& other) & noexcept
  {
    auto temp = other.value;
    other.value = T{};
    value = temp;
    return *this;
  }
  T value = {};

  /*implicit*/ operator T&() & noexcept { return value; }
  /*implicit*/ operator T const&() const& noexcept { return value; }
  /*implicit*/ operator T &&() && noexcept { return value; }
};

///////////////////////////////////////////////////////////////////////////////
// algorithms
//

constexpr auto mknejp::vmcontainer::detail::round_up(std::size_t num_bytes, std::size_t page_size) noexcept
  -> std::size_t
{
  return ((num_bytes + page_size - 1) / page_size) * page_size;
}

template<typename T, typename... Args>
auto mknejp::vmcontainer::detail::construct_at(T* p, Args&&... args) -> T*
{
  return ::new(static_cast<void*>(p)) T(std::forward<Args>(args)...);
}

template<typename T>
auto mknejp::vmcontainer::detail::destroy_at(T* p) -> void
{
  p->~T();
}

template<typename ForwardIt>
auto mknejp::vmcontainer::detail::destroy(ForwardIt first, ForwardIt last) -> void
{
  std::for_each(first, last, [](auto& x) { destroy_at(std::addressof(x)); });
}

template<typename InputIt, typename ForwardIt>
auto mknejp::vmcontainer::detail::uninitialized_move(InputIt first, InputIt last, ForwardIt d_first) -> ForwardIt
{
  return std::uninitialized_copy(std::make_move_iterator(first), std::make_move_iterator(last), d_first);
}

template<typename InputIt, typename ForwardIt>
auto mknejp::vmcontainer::detail::uninitialized_move_n(InputIt first, std::size_t count, ForwardIt d_first)
  -> std::pair<InputIt, ForwardIt>
{
  auto current = d_first;
  try
  {
    for(std::size_t i = 0; i < count; ++first, (void)++current, ++i)
    {
      construct_at(std::addressof(*current), std::move(*first));
    }
  }
  catch(...)
  {
    destroy(d_first, current);
    throw;
  }
  return {first, current};
}

template<typename ForwardIt>
auto mknejp::vmcontainer::detail::uninitialized_value_construct_n(ForwardIt first, std::size_t count) -> ForwardIt
{
  auto current = first;
  try
  {
    for(std::size_t i = 0; i < count; ++i, (void)++current)
    {
      construct_at(std::addressof(*current));
    }
  }
  catch(...)
  {
    destroy(first, current);
    throw;
  }
  return current;
}
