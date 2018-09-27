//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#pragma once
#include "vmcontainer/detail.hpp"
#include "vmcontainer/vm.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <type_traits>

namespace mknejp
{
  namespace vmcontainer
  {
    template<typename T>
    class pinned_vector;

    namespace detail
    {
      template<typename T, typename VirtualMemoryPageStack>
      class pinned_vector_impl;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// pinned_vector_impl
//

template<typename T, typename VirtualMemoryPageStack>
class mknejp::vmcontainer::detail::pinned_vector_impl
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
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // Special members
  pinned_vector_impl() = default;
  explicit pinned_vector_impl(num_bytes max_size) : _storage(max_size) {}
  explicit pinned_vector_impl(num_elements max_size) : _storage(num_bytes{max_size.count * sizeof(T)}) {}
  explicit pinned_vector_impl(num_pages max_size) : _storage(max_size) {}

  pinned_vector_impl(pinned_vector_impl const& other) : _storage(num_bytes{other._storage.reserved_bytes()})
  {
    _storage.commit(other.size() * sizeof(T));
    _end = uninitialized_copy(other.cbegin(), other.cend(), data());
  }
  pinned_vector_impl(pinned_vector_impl&& other) = default;
  pinned_vector_impl& operator=(pinned_vector_impl const& other) &
  {
    if(this != std::addressof(other))
    {
      *this = pinned_vector_impl(other);
    }
    return *this;
  }
  pinned_vector_impl& operator=(pinned_vector_impl&& other) = default;

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

  auto crbegin() const noexcept -> const_reverse_iterator { return const_reverse_iterator(const_iterator(data())); }
  auto crend() const noexcept -> const_reverse_iterator { return const_reverse_iterator(const_iterator(_end)); }

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
      destroy(_end - delta, _end.value);
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
  value_init_when_moved_from<T*> _end = data();
};

///////////////////////////////////////////////////////////////////////////////
// pinned_vector
//

template<typename T>
class mknejp::vmcontainer::pinned_vector : public detail::pinned_vector_impl<T, vm::page_stack>
{
public:
  using detail::pinned_vector_impl<T, vm::page_stack>::pinned_vector_impl;

  friend void swap(pinned_vector& lhs, pinned_vector& rhs) noexcept { lhs.swap(rhs); }
};
