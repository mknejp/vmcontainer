//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#pragma once
#include <cassert>
#include <cstddef>

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <type_traits>

namespace mknejp
{
  template<typename T>
  class pinned_vector;

  class virtual_memory_reservation;

  class virtual_memory_page_stack;

  namespace detail
  {
    namespace _pinned_vector
    {
      struct virtual_memory_system;

      template<typename VirtualMemorySystem = virtual_memory_system>
      class virtual_memory_reservation;

      template<typename VirtualMemorySystem = virtual_memory_system>
      class virtual_memory_page_stack;

      template<typename T, typename VirtualMemoryPageStack = virtual_memory_page_stack<>>
      class pinned_vector_impl;

      constexpr auto round_up(std::size_t num_bytes, std::size_t page_size) noexcept -> std::size_t;

      template<typename T, typename... Args>
      auto construct_at(T* p, Args&&... args) -> T*;
      // C++17 algorithms
      template<class T, class U = T>
      auto exchange(T& obj, U&& new_value) -> T;
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
// algorithms
//

constexpr auto mknejp::detail::_pinned_vector::round_up(std::size_t num_bytes, std::size_t page_size) noexcept
  -> std::size_t
{
  return ((num_bytes + page_size - 1) / page_size) * page_size;
}

template<class T, class U>
auto mknejp::detail::_pinned_vector::exchange(T& obj, U&& new_value) -> T
{
  auto old = std::move(obj);
  obj = std::forward<U>(new_value);
  return old;
}

template<typename T, typename... Args>
auto mknejp::detail::_pinned_vector::construct_at(T* p, Args&&... args) -> T*
{
  return ::new(static_cast<void*>(p)) T(std::forward<Args>(args)...);
}

template<typename T>
auto mknejp::detail::_pinned_vector::destroy_at(T* p) -> void
{
  p->~T();
}

template<typename ForwardIt>
auto mknejp::detail::_pinned_vector::destroy(ForwardIt first, ForwardIt last) -> void
{
  for(; first != last; ++first)
  {
    destroy_at(std::addressof(*first));
  }
}

template<typename ForwardIt, typename T>
auto mknejp::detail::_pinned_vector::uninitialized_fill_n(ForwardIt first, std::size_t count, T const& value)
  -> ForwardIt
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
auto mknejp::detail::_pinned_vector::uninitialized_copy(InputIt first, InputIt last, ForwardIt d_first) -> ForwardIt
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
auto mknejp::detail::_pinned_vector::uninitialized_move(InputIt first, InputIt last, ForwardIt d_first) -> ForwardIt
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
auto mknejp::detail::_pinned_vector::uninitialized_move_n(InputIt first, std::size_t count, ForwardIt d_first)
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
auto mknejp::detail::_pinned_vector::uninitialized_default_construct_n(ForwardIt first, std::size_t count) -> ForwardIt
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

///////////////////////////////////////////////////////////////////////////////
// virtual_memory_system
//

struct mknejp::detail::_pinned_vector::virtual_memory_system
{
  static auto reserve(std::size_t num_bytes) -> void*;
  static auto free(void* offset, std::size_t num_bytes) -> void;
  static auto commit(void* offset, std::size_t num_bytes) -> void;
  static auto decommit(void* offset, std::size_t num_bytes) -> void;

  static auto page_size() noexcept -> std::size_t;
};

///////////////////////////////////////////////////////////////////////////////
// virtual_memory_reservation
//

template<typename VirtualMemorySystem>
class mknejp::detail::_pinned_vector::virtual_memory_reservation
{
public:
  virtual_memory_reservation() = default;
  explicit virtual_memory_reservation(std::size_t num_bytes)
  {
    if(num_bytes > 0)
    {
      _reserved_bytes = round_up(num_bytes, VirtualMemorySystem::page_size());
      _base = VirtualMemorySystem::reserve(_reserved_bytes);
    }
  }
  virtual_memory_reservation(virtual_memory_reservation const& other) = delete;
  virtual_memory_reservation(virtual_memory_reservation&& other) noexcept
    : _base(exchange(other._base, nullptr)), _reserved_bytes(exchange(other._reserved_bytes, 0))
  {
  }
  auto operator=(virtual_memory_reservation const& other) = delete;
  auto operator=(virtual_memory_reservation&& other) & noexcept -> virtual_memory_reservation&
  {
    auto temp = std::move(*this);
    swap(*this, other);
    return *this;
  }
  ~virtual_memory_reservation()
  {
    if(_base)
    {
      VirtualMemorySystem::free(base(), reserved_bytes());
    }
  }

  friend void swap(virtual_memory_reservation& lhs, virtual_memory_reservation& rhs) noexcept
  {
    std::swap(lhs._base, rhs._base);
    std::swap(lhs._reserved_bytes, rhs._reserved_bytes);
  }

  auto base() const noexcept -> void* { return _base; }
  auto reserved_bytes() const noexcept -> std::size_t { return _reserved_bytes; }

private:
  void* _base = nullptr; // The starting address of the reserved address space
  std::size_t _reserved_bytes = 0; // Total size of reserved address space in bytes
};

class mknejp::virtual_memory_reservation : detail::_pinned_vector::virtual_memory_reservation<>
{
  using detail::_pinned_vector::virtual_memory_reservation<>::virtual_memory_reservation;
};

///////////////////////////////////////////////////////////////////////////////
// virtual_memory_page_stack
//

template<typename VirtualMemorySystem>
class mknejp::detail::_pinned_vector::virtual_memory_page_stack
{
public:
  virtual_memory_page_stack() = default;
  explicit virtual_memory_page_stack(std::size_t num_bytes) : _reservation(num_bytes) {}
  virtual_memory_page_stack(virtual_memory_page_stack const& other) = delete;
  virtual_memory_page_stack(virtual_memory_page_stack&& other) noexcept
    : _reservation(std::move(other._reservation))
    , _committed_bytes(exchange(other._committed_bytes, 0))
    , _page_size(exchange(other._page_size, 0))
  {
  }
  auto operator=(virtual_memory_page_stack const& other) = delete;
  auto operator=(virtual_memory_page_stack&& other) & noexcept -> virtual_memory_page_stack&
  {
    auto temp = std::move(*this);
    swap(*this, other);
    return *this;
  }
  ~virtual_memory_page_stack()
  {
    if(committed_bytes() > 0)
    {
      VirtualMemorySystem::decommit(base(), committed_bytes());
    }
  }

  auto commit(std::size_t bytes) -> void
  {
    if(bytes > 0)
    {
      auto const new_committed = round_up(committed_bytes() + bytes, page_size());
      assert(new_committed <= reserved_bytes());
      VirtualMemorySystem::commit(static_cast<char*>(base()) + committed_bytes(), new_committed - committed_bytes());
      _committed_bytes = new_committed;
    }
  }

  auto decommit(std::size_t bytes) -> void
  {
    if(bytes > 0)
    {
      assert(bytes <= committed_bytes());
      auto const new_committed = round_up(committed_bytes() - bytes, page_size());
      if(new_committed < committed_bytes())
      {
        VirtualMemorySystem::decommit(static_cast<char*>(base()) + new_committed, committed_bytes() - new_committed);
        _committed_bytes = new_committed;
      }
    }
  }

  auto resize(std::size_t new_size) -> void
  {
    assert(new_size <= reserved_bytes());
    if(new_size < committed_bytes())
    {
      decomit(committed_bytes() - new_size);
    }
    else if(new_size > committed_bytes())
    {
      commit(new_size - committed_bytes());
    }
  }

  auto base() const noexcept -> void* { return _reservation.base(); }
  auto committed_bytes() const noexcept -> std::size_t { return _committed_bytes; }
  auto reserved_bytes() const noexcept -> std::size_t { return _reservation.reserved_bytes(); }
  auto page_size() const noexcept -> std::size_t { return _page_size; }

  friend void swap(virtual_memory_page_stack& lhs, virtual_memory_page_stack& rhs) noexcept
  {
    using std::swap;
    swap(lhs._reservation, rhs._reservation);
    swap(lhs._committed_bytes, rhs._committed_bytes);
    swap(lhs._page_size, rhs._page_size);
  }

private:
  using Reservation = mknejp::detail::_pinned_vector::virtual_memory_reservation<VirtualMemorySystem>;
  Reservation _reservation;
  std::size_t _committed_bytes = 0;
  std::size_t _page_size = VirtualMemorySystem::page_size();
};

class mknejp::virtual_memory_page_stack : detail::_pinned_vector::virtual_memory_page_stack<>
{
  using detail::_pinned_vector::virtual_memory_page_stack<>::virtual_memory_page_stack;
};

///////////////////////////////////////////////////////////////////////////////
// pinned_vector_impl
//

template<typename T, typename VirtualMemoryPageStack>
class mknejp::detail::_pinned_vector::pinned_vector_impl
{
public:
  static_assert(std::is_destructible<T>::value, "value_type must satisfy Destructible concept");

  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = T&;
  using const_reference = T const&;
  using pointer = T*;
  using const_pointer = T const*;

  using iterator = T*;
  using const_iterator = T const*;
  using reverse_iterator = std::reverse_iterator<T*>;
  using const_reverse_iterator = std::reverse_iterator<T const*>;

  // Special members
  pinned_vector_impl() = default;
  explicit pinned_vector_impl(size_type max_size) : _storage(max_size * sizeof(T)) {}

  pinned_vector_impl(pinned_vector_impl const& other) : _storage(other._storage.reserved_bytes())
  {
    _storage.commit(other.size() * sizeof(T));
    _end = uninitialized_copy(other.cbegin(), other.cend(), data());
  }
  pinned_vector_impl(pinned_vector_impl&& other) noexcept
    : _storage(std::move(other._storage)), _end(_pinned_vector::exchange(other._end, nullptr))
  {
  }
  auto operator=(pinned_vector_impl const& other) & -> pinned_vector_impl&
  {
    if(this != std::addressof(other))
    {
      *this = pinned_vector_impl(other);
    }
    return *this;
  }
  auto operator=(pinned_vector_impl&& other) & noexcept -> pinned_vector_impl&
  {
    auto temp = std::move(*this);
    swap(other);
    return *this;
  }

  ~pinned_vector_impl() { clear(); }

  // Element access
  auto at(size_type pos) -> T&
  {
    if(pos >= size())
    {
      throw std::out_of_range("index out of range");
    }
    return data()[pos];
  }
  auto at(size_type pos) const -> T const&
  {
    if(pos >= size())
    {
      throw std::out_of_range("index out of range");
    }
    return data()[pos];
  }

  auto operator[](size_type pos) -> T&
  {
    assert(pos < size());
    return data()[pos];
  }
  auto operator[](size_type pos) const -> T const&
  {
    assert(pos < size());
    return data()[pos];
  }

  auto front() -> T&
  {
    assert(size() > 0);
    return (*this)[0];
  }
  auto front() const -> T const&
  {
    assert(size() > 0);
    return (*this)[0];
  }

  auto back() -> T&
  {
    assert(size() > 0);
    return (*this)[size() - 1];
  }
  auto back() const -> T const&
  {
    assert(size() > 0);
    return (*this)[size() - 1];
  }

  auto data() noexcept -> T* { return static_cast<T*>(_storage.base()); }
  auto data() const noexcept -> T const* { return static_cast<T const*>(_storage.base()); }

  // Iterators

  auto begin() noexcept -> iterator { return iterator(data()); }
  auto end() noexcept -> iterator { return iterator(_end); }

  auto begin() const noexcept -> const_iterator { return cbegin(); }
  auto end() const noexcept -> const_iterator { return cbegin(); }

  auto cbegin() const noexcept -> const_iterator { return const_iterator(data()); }
  auto cend() const noexcept -> const_iterator { return const_iterator(_end); }

  auto rbegin() noexcept -> reverse_iterator { return reverse_iterator(iterator(data())); }
  auto rend() noexcept -> reverse_iterator { return reverse_iterator(iterator(_end)); }

  auto rbegin() const noexcept -> const_reverse_iterator { return const_reverse_iterator(const_iterator(data())); }
  auto rend() const noexcept -> const_reverse_iterator { return const_reverse_iterator(const_iterator(_end)); }

  auto rcbegin() const noexcept -> const_reverse_iterator { return const_reverse_iterator(const_iterator(data())); }
  auto rcend() const noexcept -> const_reverse_iterator { return const_reverse_iterator(const_iterator(_end)); }

  // Capacity

  auto empty() const noexcept -> bool { return begin() == end(); }
  auto size() const noexcept -> size_type { return static_cast<size_type>(_end - data()); }
  auto max_size() const noexcept -> size_type { return _storage.reserved_bytes() / sizeof(T); }
  auto reserve(size_type new_cap) -> void
  {
    assert(new_cap <= max_size());
    if(new_cap > capacity())
    {
      _storage.commit(new_cap * sizeof(T) - _storage.committed_bytes());
    }
  }
  auto capacity() const noexcept -> size_type { return _storage.committed_bytes() / sizeof(T); }
  auto shrink_to_fit() -> void
  {
    if(capacity() > size())
    {
      _storage.decommit((capacity() - size()) * sizeof(T));
    }
  }
  auto page_size() const noexcept -> std::size_t { return _storage.page_size(); }

  // Modifiers

  auto clear() noexcept -> void
  {
    destroy(begin(), end());
    _end = data();
  }
  auto insert(const_iterator pos, T const& value) ->
    typename std::enable_if<std::is_copy_constructible<T&>::value, iterator>::type
  {
    assert(is_valid_last_iterator(pos));
    return iterator(std::addressof(emplace(pos, value)));
  }
  template<typename U = T>
  auto insert(const_iterator pos, T&& value) ->
    typename std::enable_if<std::is_move_constructible<U>::value, iterator>::type
  {
    assert(is_valid_last_iterator(pos));
    return iterator(std::addressof(emplace(pos, std::move(value))));
  }
  auto insert(const_iterator pos, size_type count, const T& value) -> iterator
  {
    assert(is_valid_last_iterator(pos));
    return range_insert_impl(pos, count, [&value](T* first, T* last) { std::fill(first, last, value); });
  }

  template<typename InputIter>
  auto insert(const_iterator pos, InputIter first, InputIter last) -> typename std::enable_if<
    std::is_base_of<std::input_iterator_tag, typename std::iterator_traits<InputIter>::iterator_category>::value,
    iterator>::type
  {
    assert(is_valid_last_iterator(pos));
    return insert(pos, first, last, typename std::iterator_traits<InputIter>::iterator_category());
  }

private:
  template<typename InputIter>
  auto insert(const_iterator pos, InputIter first, InputIter last, std::input_iterator_tag) -> iterator
  {
    for(auto it = pos; first != last; ++first, ++it)
    {
      insert(pos, *first);
    }
    return pos;
  }

  template<typename InputIter>
  auto insert(const_iterator pos, InputIter first, InputIter last, std::forward_iterator_tag) -> iterator
  {
    return range_insert_impl(pos, static_cast<size_type>(std::distance(first, last)), [&](T* d_first, T* d_last) {
      std::copy(first, last, d_first);
    });
  }

  template<typename F>
  auto range_insert_impl(const_iterator pos, size_type count, F fill) -> iterator
  {
    auto* const p = to_pointer(pos);
    if(count > 0)
    {
      grow_if_necessary(count);
      if(p != _end)
      {
        uninitialized_move(_end - count, _end, _end);
        auto const rest = _end - p - count;
        std::move_backward(p, p + rest, _end - rest);
      }
      fill(p, p + count);
      _end += count;
    }
    return iterator(p);
  }

public:
  auto insert(const_iterator pos, std::initializer_list<T> ilist) -> iterator
  {
    assert(is_valid_last_iterator(pos));
    return range_insert_impl(
      pos, ilist.size(), [&](T* d_first, T* d_last) { std::copy(ilist.begin(), ilist.end(), d_first); });
  }
  template<typename... Args>
  auto emplace(const_iterator pos, Args&&... args) ->
    typename std::enable_if<std::is_constructible<T, Args&&...>::value && std::is_move_constructible<T>::value
                              && std::is_move_assignable<T>::value,
                            T&>::type
  {
    assert(is_valid_last_iterator(pos));
    grow_if_necessary(1);
    auto* p = to_pointer(pos);
    if(p != _end)
    {
      uninitialized_move(_end - 1, _end, _end);
      std::move_backward(p, _end - 1, p + 1);
      destroy_at(p);
    }
    auto* x = construct_at(p, std::forward<Args>(args)...);
    ++_end;
    return *x;
  }
  auto erase(const_iterator pos) -> iterator
  {
    assert(is_valid_iterator(pos));
    std::move(to_iterator(pos) + 1, end(), to_iterator(pos));
    destroy_at(--_end);
    return pos;
  }
  auto erase(const_iterator first, const_iterator last) -> iterator
  {
    assert(is_valid_last_iterator(last));
    assert(first <= last);
    std::move(to_iterator(last), end(), to_iterator(first));
    destroy(to_iterator(last), end());
    _end = to_pointer(last);
    return last;
  }
  template<typename U = T>
  auto push_back(T const& value) -> typename std::enable_if<std::is_copy_constructible<U>::value, T&>::type
  {
    return emplace_back(value);
  }
  template<typename U = T>
  auto push_back(T&& value) -> typename std::enable_if<std::is_move_constructible<U>::value, T&>::type
  {
    return emplace_back(std::move(value));
  }
  template<typename... Args>
  auto emplace_back(Args&&... args) -> typename std::enable_if<std::is_constructible<T, Args&&...>::value, T&>::type
  {
    grow_if_necessary(1);
    construct_at(_end, std::forward<Args>(args)...);
    return *_end++;
  }
  auto pop_back() -> void
  {
    assert(!empty());
    destroy_at(--_end);
  }
  auto resize(size_type count) -> void
  {
    if(count > size())
    {
      auto const delta = count - size();
      reserve(count);
      uninitialized_default_construct_n(_end, delta);
      _end += delta;
    }
    else if(count < size())
    {
      auto const delta = size() - count;
      destroy(_end - delta, _end);
      _end -= delta;
      shrink_to_fit();
    }
  }
  auto resize(size_type count, T const& value) -> void
  {
    auto const old_size = size();
    if(count > old_size)
    {
      reserve(count);
      auto const delta = count - old_size;
      uninitialized_fill_n(_end, _end + delta, value);
      _end += delta;
    }
    else if(count < old_size)
    {
      auto const delta = old_size - count;
      destroy(_end - delta, _end);
      _end -= delta;
      shrink_to_fit();
    }
  }
  auto swap(pinned_vector_impl& other) noexcept -> void
  {
    using std::swap;
    swap(_storage, other._storage);
    swap(_end, other._end);
  }

  template<typename U = T, typename = decltype(std::declval<U const&>() == std::declval<U const&>())>
  friend auto operator==(pinned_vector<T> const& lhs, pinned_vector<T> const& rhs) -> bool
  {
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
  }
  template<typename U = T, typename = decltype(std::declval<U const&>() == std::declval<U const&>())>
  friend auto operator!=(pinned_vector<T> const& lhs, pinned_vector<T> const& rhs) -> bool
  {
    return !(lhs == rhs);
  }
  template<typename U = T, typename = decltype(std::declval<U const&>() < std::declval<U const&>())>
  friend auto operator<(pinned_vector<T> const& lhs, pinned_vector<T> const& rhs) -> bool
  {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
  }
  template<typename U = T, typename = decltype(std::declval<U const&>() < std::declval<U const&>())>
  friend auto operator>(pinned_vector<T> const& lhs, pinned_vector<T> const& rhs) -> bool
  {
    return rhs < lhs;
  }
  template<typename U = T, typename = decltype(std::declval<U const&>() < std::declval<U const&>())>
  friend auto operator<=(pinned_vector<T> const& lhs, pinned_vector<T> const& rhs) -> bool
  {
    return !(rhs < lhs);
  }
  template<typename U = T, typename = decltype(std::declval<U const&>() < std::declval<U const&>())>
  friend auto operator>=(pinned_vector<T> const& lhs, pinned_vector<T> const& rhs) -> bool
  {
    return !(lhs < rhs);
  }

private:
  static constexpr std::size_t growth_factor = 2;

  auto grow_if_necessary(std::size_t n) -> void
  {
    auto const new_size = size() + n;
    if(new_size > capacity())
    {
      reserve(std::max(std::min(capacity() * growth_factor, max_size()), new_size));
    }
  }

  auto is_valid_iterator(const_iterator it) const noexcept -> bool
  {
    return to_pointer(it) >= data() && to_pointer(it) < _end;
  }
  auto is_valid_last_iterator(const_iterator it) const noexcept -> bool
  {
    return to_pointer(it) >= data() && to_pointer(it) <= _end;
  }

  auto to_iterator(const_iterator it) noexcept -> iterator { return iterator(to_pointer(it)); }
  auto to_pointer(const_iterator it) noexcept -> iterator { return data() + (it - cbegin()); }

  VirtualMemoryPageStack _storage;
  T* _end = data();
};

///////////////////////////////////////////////////////////////////////////////
// pinned_vector
//

template<typename T>
class mknejp::pinned_vector : public detail::_pinned_vector::pinned_vector_impl<T>
{
public:
  using detail::_pinned_vector::pinned_vector_impl<T>::pinned_vector_impl;

  friend void swap(pinned_vector& lhs, pinned_vector& rhs) noexcept { lhs.swap(rhs); }
};
