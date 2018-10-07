//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#pragma once
#include <cstddef>
#include <type_traits>
#include <utility>

namespace mknejp
{
  namespace vmcontainer
  {
    struct num_pages
    {
      std::size_t count;
    };
    struct num_elements
    {
      std::size_t count;
    };
    struct num_bytes
    {
      std::size_t count;
    };

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
      template<typename ForwardIt, typename T>
      auto uninitialized_fill_n(ForwardIt first, std::size_t count, T const& value) -> ForwardIt;
      template<typename InputIt, typename ForwardIt>
      auto uninitialized_copy(InputIt first, InputIt last, ForwardIt d_first) -> ForwardIt;
      template<typename InputIt, typename ForwardIt>
      auto uninitialized_move(InputIt first, InputIt last, ForwardIt d_first) -> ForwardIt;
      template<typename InputIt, typename ForwardIt>
      auto uninitialized_move_n(InputIt first, std::size_t count, ForwardIt d_first) -> std::pair<InputIt, ForwardIt>;
      template<typename ForwardIt>
      auto uninitialized_default_construct_n(ForwardIt first, std::size_t count) -> ForwardIt;
    }
  }
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
  for(; first != last; ++first)
  {
    destroy_at(std::addressof(*first));
  }
}

template<typename ForwardIt, typename T>
auto mknejp::vmcontainer::detail::uninitialized_fill_n(ForwardIt first, std::size_t count, T const& value) -> ForwardIt
{
  auto current = first;
  try
  {
    for(std::size_t i = 0; i < count; ++current, (void)++i)
    {
      construct_at(std::addressof(*current), value);
    }
  }
  catch(...)
  {
    destroy(first, current);
    throw;
  }
  return current;
}

template<typename InputIt, typename ForwardIt>
auto mknejp::vmcontainer::detail::uninitialized_copy(InputIt first, InputIt last, ForwardIt d_first) -> ForwardIt
{
  auto current = d_first;
  try
  {
    for(; first != last; ++first, (void)++current)
    {
      construct_at(std::addressof(*current), *first);
    }
  }
  catch(...)
  {
    destroy(d_first, current);
    throw;
  }
  return current;
}

template<typename InputIt, typename ForwardIt>
auto mknejp::vmcontainer::detail::uninitialized_move(InputIt first, InputIt last, ForwardIt d_first) -> ForwardIt
{
  auto current = d_first;
  try
  {
    for(; first != last; ++first, (void)++current)
    {
      construct_at(std::addressof(*current), std::move(*first));
    }
  }
  catch(...)
  {
    destroy(d_first, current);
    throw;
  }
  return current;
}

template<typename InputIt, typename ForwardIt>
auto mknejp::vmcontainer::detail::uninitialized_move_n(InputIt first, std::size_t count, ForwardIt d_first)
  -> std::pair<InputIt, ForwardIt>
{
  auto current = d_first;
  try
  {
    for(std::size_t i = 0; i < count; ++first, (void)++current)
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
auto mknejp::vmcontainer::detail::uninitialized_default_construct_n(ForwardIt first, std::size_t count) -> ForwardIt
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
