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

  template<typename T>
  auto swap(pinned_vector<T>& lhs, pinned_vector<T>& rhs) noexcept -> void;

  namespace detail
  {
    namespace _pinned_vector
    {
      template<typename T, typename VirtualAllocator>
      class pinned_vector_impl;

      struct virtual_allocator;

      constexpr auto pad_to_page_size(std::size_t num_bytes, std::size_t page_size) noexcept -> std::size_t;

      // Same as std::move_backwards but does construction instead of assignment
      template<typename T>
      T* move_construct_backwards(T* first, T* last, T* d_last);
      template<typename T, typename... Args>
      auto construct_at(T* p, Args&&... args) -> T*;
      // C++17 algorithms
      template<typename T>
      auto destroy_at(T* p) -> void;
      template<typename ForwardIter>
      auto destroy(ForwardIter first, ForwardIter last) -> void;
      template<typename ForwardIter, typename T>
      auto uninitialized_fill_n(ForwardIter first, std::size_t count, T const& value) -> ForwardIter;
      template<typename InputIter, typename ForwardIter>
      auto uninitialized_move(InputIter first, InputIter last, ForwardIter d_first) -> ForwardIter;
      template<typename InputIter, typename ForwardIter>
      auto uninitialized_move_n(InputIter first, std::size_t count, ForwardIter d_first) -> ForwardIter;
      template<typename ForwardIter>
      auto uninitialized_default_construct_n(ForwardIter first, std::size_t count) -> ForwardIter;
    }
  }
}

struct mknejp::detail::_pinned_vector::virtual_allocator
{
  static auto reserve(std::size_t num_bytes) -> void*;
  static auto free(void* offset) -> void;
  static auto commit(void* offset, std::size_t num_bytes) -> void;
  static auto decommit(void* offset, std::size_t num_bytes) -> void;

  static auto page_size() noexcept -> std::size_t;
};

constexpr auto mknejp::detail::_pinned_vector::pad_to_page_size(std::size_t num_bytes, std::size_t page_size) noexcept -> std::size_t
{
  auto const num_pages = (num_bytes + page_size - 1) / page_size;
  return num_pages * page_size;
}

template<typename T>
T* mknejp::detail::_pinned_vector::move_construct_backwards(T* first, T* last, T* d_last)
{
  assert(first < last);
  while(first != last)
  {
    new(static_cast<void*>(--d_last)) T(std::move(*--last));
  }
  return d_last;
}

template<typename T, typename... Args>
auto mknejp::detail::_pinned_vector::construct_at(T* p, Args&&... args) -> T*
{
  return new(static_cast<void*>(p)) T(std::forward<Args>(args)...);
}

template<typename T>
auto mknejp::detail::_pinned_vector::destroy_at(T* p) -> void
{
  p->~T();
}

template<typename ForwardIter>
auto mknejp::detail::_pinned_vector::destroy(ForwardIter first, ForwardIter last) -> void
{
  for(; first != last; ++first)
  {
    destroy_at(std::addressof(*first));
  }
}

template<typename ForwardIter, typename T>
auto mknejp::detail::_pinned_vector::uninitialized_fill_n(ForwardIter first, std::size_t count, T const& value) -> ForwardIter
{
  for(std::size_t i = 0; i < count; ++first, (void)++i)
  {
    new(static_cast<void*>(std::addressof(*first))) typename std::iterator_traits<ForwardIter>::value_type(value);
  }
  return first;
}

template<typename InputIter, typename ForwardIter>
auto mknejp::detail::_pinned_vector::uninitialized_move(InputIter first, InputIter last, ForwardIter d_first) -> ForwardIter
{
  for(; first != last; ++first, (void)++d_first)
  {
    new(static_cast<void*>(std::addressof(*d_first))) typename std::iterator_traits<ForwardIter>::value_type(std::move(*first));
  }
  return d_first;
}

template<typename InputIter, typename ForwardIter>
auto mknejp::detail::_pinned_vector::uninitialized_move_n(InputIter first, std::size_t count, ForwardIter d_first) -> ForwardIter
{
  for(std::size_t i = 0; i < count; ++first, (void)++d_first)
  {
    new(static_cast<void*>(std::addressof(*d_first))) typename std::iterator_traits<ForwardIter>::value_type(std::move(*first));
  }
  return d_first;
}

template<typename ForwardIter>
auto mknejp::detail::_pinned_vector::uninitialized_default_construct_n(ForwardIter first, std::size_t count) -> ForwardIter
{
  for(std::size_t i = 0; i < count; ++i, (void)++first)
  {
    construct_at(std::addressof(*first));
  }
}

template<typename T, typename VirtualAllocator>
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
  explicit pinned_vector_impl(size_type max_size)
  {
    assert(max_size > 0);
    _reserved = detail::_pinned_vector::pad_to_page_size(max_size * sizeof(T), page_size());
    _base = _end = reinterpret_cast<T*>(VirtualAllocator::reserve(_reserved));
  }

  pinned_vector_impl(pinned_vector_impl const&) = delete;
  pinned_vector_impl(pinned_vector_impl&& other) noexcept { swap(other); }
  auto operator=(pinned_vector_impl const&) -> pinned_vector_impl& = delete;
  auto operator=(pinned_vector_impl&& other) noexcept -> pinned_vector_impl&
  {
    auto temp = std::move(*this);
    swap(other);
    return *this;
  }

  ~pinned_vector_impl()
  {
    clear();
    if(_reserved > 0)
    {
      VirtualAllocator::free(_base);
    }
  }

  // Element access
  auto at(size_type pos) -> T&
  {
    if(pos >= size())
    {
      throw std::out_of_range("index out of range");
    }
    return _base[pos];
  }
  auto at(size_type pos) const -> T const&
  {
    if(pos >= size())
    {
      throw std::out_of_range("index out of range");
    }
    return _base[pos];
  }

  auto operator[](size_type pos) -> T&
  {
    assert(pos < size());
    return _base[pos];
  }
  auto operator[](size_type pos) const -> T const&
  {
    assert(pos < size());
    return _base[pos];
  }

  auto front() -> T& { return (*this)[0]; }
  auto front() const -> T const& { return (*this)[0]; }

  auto back() -> T& { return (*this)[size() - 1]; }
  auto back() const -> T const& { return (*this)[size() - 1]; }

  auto data() noexcept -> T* { return _base; }
  auto data() const noexcept -> T const* { return _base; }

  // Iterators

  auto begin() noexcept -> iterator { return iterator(_base); }
  auto end() noexcept -> iterator { return iterator(_end); }

  auto begin() const noexcept -> const_iterator { return cbegin(); }
  auto end() const noexcept -> const_iterator { return cbegin(); }

  auto cbegin() const noexcept -> const_iterator { return const_iterator(_base); }
  auto cend() const noexcept -> const_iterator { return const_iterator(_end); }

  auto rbegin() noexcept -> reverse_iterator { return reverse_iterator(iterator(_base)); }
  auto rend() noexcept -> reverse_iterator { return reverse_iterator(iterator(_end)); }

  auto rbegin() const noexcept -> reverse_iterator { return const_reverse_iterator(const_iterator(_base)); }
  auto rend() const noexcept -> reverse_iterator { return const_reverse_iterator(const_iterator(_end)); }

  auto rcbegin() const noexcept -> reverse_iterator { return const_reverse_iterator(const_iterator(_base)); }
  auto rcend() const noexcept -> reverse_iterator { return const_reverse_iterator(const_iterator(_end)); }

  // Capacity

  auto empty() const noexcept -> bool { return _committed == 0; }
  auto size() const noexcept -> size_type { return static_cast<size_type>(_end - _base); }
  auto max_size() const noexcept -> size_type { return _reserved / sizeof(T); }
  auto reserve(size_type new_cap) -> void
  {
    assert(new_cap <= max_size());
    auto const new_committed = pad_to_page_size(new_cap * sizeof(T), page_size());
    if(new_committed > _committed)
    {
      VirtualAllocator::commit(reinterpret_cast<void*>(reinterpret_cast<char*>(_base) + _committed), new_committed - _committed);
      _committed = new_committed;
    }
  }
  auto capacity() const noexcept -> size_type { return _committed / sizeof(T); }
  auto shrink_to_fit(size_type new_cap) -> void
  {
    auto const new_committed = pad_to_page_size(size() * sizeof(T), page_size());
    if(new_committed < _committed)
    {
      VirtualAllocator::decommit(reinterpret_cast<void*>(reinterpret_cast<char*>(_base) + new_committed), _committed - new_committed);
      _committed = new_committed;
    }
  }
  auto page_size() const noexcept -> std::size_t { return _page_size; }

  // Modifiers

  auto clear() noexcept -> void
  {
    destroy(_base, _end);
    _end = _base;
  }
  auto insert(const_iterator pos, T const& value) -> typename std::enable_if<std::is_copy_constructible<T&>::value, iterator>::type
  {
    return iterator(std::addressof(emplace(pos, value)));
  }
  auto insert(const_iterator pos, T&& value) -> typename std::enable_if<std::is_move_constructible<T>::value, iterator>::type
  {
    return iterator(std::addressof(emplace(pos, std::move(value))));
  }
  auto insert(const_iterator pos, size_type count, const T& value) -> iterator
  {
    return range_insert_impl(pos, count, [&value](T* first, T* last) { std::fill(first, last, value); });
  }

  template<typename InputIter>
  auto insert(const_iterator pos, InputIter first, InputIter last) ->
    typename std::enable_if<std::is_base_of<std::input_iterator_tag, typename std::iterator_traits<InputIter>::iterator_category>::value, iterator>::type
  {
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
    return range_insert_impl(pos, static_cast<size_type>(std::distance(first, last)), [](T* d_first, T* d_last) { std::copy(first, last, d_first); });
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
        std::move_backwards(p, p + rest, _end - rest);
      }
      fill(p, p + count);
      _end += count;
    }
    return iterator(p);
  }

public:
  auto insert(const_iterator pos, std::initializer_list<T> ilist) -> iterator
  {
    return range_insert_impl(pos, ilist.size(), [](T* d_first, T* d_last) { std::copy(ilist.begin(), ilist.end(), d_first); });
  }
  template<typename... Args>
  auto emplace(const_iterator pos, Args&&... args) ->
    typename std::enable_if<std::is_constructible<T, Args&&...>::value && std::is_move_constructible<T>::value && std::is_move_assignable<T>::value, T&>::type
  {
    grow_if_necessary(1);
    auto* p = to_pointer(pos);
    if(p != _end)
    {
      uninitialized_move(_end - 1, _end, _end);
      std::move_backwards(p, _end - 1, p + 1);
      destroy_at(p);
    }
    auto* x = construct_at(p, std::forward<Args>(args)...);
    ++_end;
    return *x;
  }
  auto erase(const_iterator pos) -> iterator
  {
    std::move(to_iterator(pos) + 1, end(), to_iterator(pos));
    destroy_at(--_end);
    return pos;
  }
  auto erase(const_iterator first, const_iterator last) -> iterator
  {
    std::move(to_iterator(last), end(), to_iterator(first));
    destroy(to_iterator(last), end());
    _end = to_pointer(last);
    return last;
  }
  auto push_back(T const& value) -> typename std::enable_if<std::is_copy_constructible<T>::value, T&>::type { return emplace_back(value); }
  auto push_back(T&& value) -> typename std::enable_if<std::is_move_constructible<T>::value, T&>::type { return emplace_back(std::move(value)); }
  template<typename... Args>
  auto emplace_back(Args&&... args) -> typename std::enable_if<std::is_constructible<T, Args&&...>::value, T&>::type
  {
    grow_if_necessary(1);
    construct_at(_end, std::forward<Args>(args)...);
    return *_end++;
  }
  auto pop_back() -> void
  {
    assert(_base != _end);
    destroy_at(--_end);
  }
  auto resize(size_type count) -> void
  {
    if(count > size())
    {
      auto const delta = count - size();
      reserve(count);
      uninitialized_default_construct_n(_end, _end + delta);
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
      uninitialized_default_construct_n(_end, _end + delta, value);
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
  void swap(pinned_vector_impl& other) noexcept
  {
    std::swap(_base, other._base);
    std::swap(_end, other._end);
    std::swap(_committed, other._committed);
    std::swap(_reserved, other._reserved);
    std::swap(_page_size, other._page_size);
  }

  friend auto operator==(pinned_vector<T> const& lhs, pinned_vector<T> const& rhs) -> bool
  {
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
  }
  friend auto operator!=(pinned_vector<T> const& lhs, pinned_vector<T> const& rhs) -> bool
  {
    // TODO: is std::mismatch more efficient than inverting operator== ?
    return !(lhs == rhs);
  }
  friend auto operator<(pinned_vector<T> const& lhs, pinned_vector<T> const& rhs) -> bool
  {
    return std::lexicogaphical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
  }
  friend auto operator>(pinned_vector<T> const& lhs, pinned_vector<T> const& rhs) -> bool { return rhs < lhs; }
  friend auto operator<=(pinned_vector<T> const& lhs, pinned_vector<T> const& rhs) -> bool { return !(rhs < lhs); }
  friend auto operator>=(pinned_vector<T> const& lhs, pinned_vector<T> const& rhs) -> bool { return !(lhs < rhs); }

private:
  static constexpr std::size_t growth_factor = 2;

  auto grow_if_necessary(std::size_t n) -> void
  {
    auto const new_size = size() + n;
    if(new_size > capacity())
    {
      reserve(std::max(capacity() * growth_factor, new_size));
    }
  }

  static auto to_iterator(const_iterator it) noexcept -> iterator { return iterator(to_pointer(it)); }
  static auto to_pointer(const_iterator it) noexcept -> iterator { return _base + (it - cbegin()); }

  T* _base = nullptr; // The starting address of the reserved address space
  T* _end = nullptr; // The past-the-end address of the last value
  std::size_t _committed = 0; // Total size of committed pages
  std::size_t _reserved = 0; // Total size of reserved address space
  std::size_t _page_size = VirtualAllocator::page_size(); // Size in bytes of one page
};

template<typename T>
class mknejp::pinned_vector : public detail::_pinned_vector::pinned_vector_impl<T, detail::_pinned_vector::virtual_allocator>
{
public:
  using pinned_vector_impl::pinned_vector_impl;
};

// Non-member functions

template<typename T>
auto mknejp::swap(pinned_vector<T>& lhs, pinned_vector<T>& rhs) noexcept -> void
{
  lhs.swap(rhs);
}
