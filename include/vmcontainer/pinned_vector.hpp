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
#include <ratio>
#include <type_traits>

namespace mknejp
{
  namespace vmcontainer
  {
    struct pinned_vector_traits;

    template<typename T, typename Traits = pinned_vector_traits>
    class pinned_vector;
  }
}

///////////////////////////////////////////////////////////////////////////////
// pinned_vector_traits
//

struct mknejp::vmcontainer::pinned_vector_traits
{
  using page_stack = vm::page_stack;
  using growth_factor = std::ratio<2, 1>;
};

///////////////////////////////////////////////////////////////////////////////
// pinned_vector
//

template<typename T, typename Traits>
class mknejp::vmcontainer::pinned_vector
{
public:
  static_assert(std::is_destructible<T>::value, "value_type must satisfy Destructible concept");
  static_assert(std::ratio_greater<typename Traits::growth_factor, std::ratio<1, 1>>::value,
                "growth factor must be greater than 1");

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

  // constructors
  pinned_vector() = default;
  explicit pinned_vector(max_size_t max_size) : _storage(max_size.scaled_for_type<T>()) {}

  pinned_vector(max_size_t max_size, std::initializer_list<T> init) : pinned_vector(max_size) { insert(end(), init); }

  template<typename InputIter,
           typename = typename std::enable_if<
             std::is_base_of<std::input_iterator_tag,
                             typename std::iterator_traits<InputIter>::iterator_category>::value>::type>
  // requires InputIterator<InputIter>
  pinned_vector(max_size_t max_size, InputIter first, InputIter last) : pinned_vector(max_size)
  {
    insert(end(), first, last);
  }

  pinned_vector(max_size_t max_size, size_type count, T const& value) : pinned_vector(max_size)
  {
    insert(end(), count, value);
  }

  template<typename U = T, typename = typename std::enable_if<std::is_default_constructible<U>::value>::type>
  pinned_vector(max_size_t max_size, size_type count) : pinned_vector(max_size)
  {
    reserve(count);
    _end = detail::uninitialized_default_construct_n(data(), count);
  }

  // Special members
  pinned_vector(pinned_vector const& other) : _storage(num_bytes(other._storage.reserved_bytes()))
  {
    _storage.commit(other.size() * sizeof(T));
    _end = detail::uninitialized_copy(other.cbegin(), other.cend(), data());
  }
  pinned_vector(pinned_vector&& other) = default;
  pinned_vector& operator=(pinned_vector const& other) &
  {
    if(this != std::addressof(other))
    {
      *this = pinned_vector(other);
    }
    return *this;
  }
  pinned_vector& operator=(pinned_vector&& other) & noexcept
  {
    auto temp = std::move(other);
    swap(temp);
    return *this;
  }
  pinned_vector& operator=(std::initializer_list<T> init) &
  {
    assign(init);
    return *this;
  }

  ~pinned_vector() { clear(); }

  // Assign
  auto assign(std::size_t count, T const& value) -> void
  {
    clear();
    insert(end(), count, value);
  }
  auto assign(std::initializer_list<T> init) -> void { assign(init.begin(), init.end()); }
  template<typename InputIter>
  // requires InputIterator<InputIter>
  auto assign(InputIter first, InputIter last) -> typename std::enable_if<
    std::is_base_of<std::input_iterator_tag, typename std::iterator_traits<InputIter>::iterator_category>::value,
    void>::type
  {
    clear();
    insert(end(), first, last, typename std::iterator_traits<InputIter>::iterator_category());
  }

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
  auto end() const noexcept -> const_iterator { return cend(); }

  auto cbegin() const noexcept -> const_iterator { return const_iterator(data()); }
  auto cend() const noexcept -> const_iterator { return const_iterator(_end); }

  auto rbegin() noexcept -> reverse_iterator { return reverse_iterator(end()); }
  auto rend() noexcept -> reverse_iterator { return reverse_iterator(begin()); }

  auto rbegin() const noexcept -> const_reverse_iterator { return const_reverse_iterator(cend()); }
  auto rend() const noexcept -> const_reverse_iterator { return const_reverse_iterator(cbegin()); }

  auto crbegin() const noexcept -> const_reverse_iterator { return const_reverse_iterator(cend()); }
  auto crend() const noexcept -> const_reverse_iterator { return const_reverse_iterator(cbegin()); }

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
      _storage.decommit(_storage.committed_bytes() - size() * sizeof(T));
    }
  }
  auto page_size() const noexcept -> std::size_t { return _storage.page_size(); }

  // Modifiers

  auto clear() noexcept -> void
  {
    detail::destroy(begin(), end());
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
      insert(it, *first);
    }
    return to_iterator(pos);
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
        detail::uninitialized_move(_end - count, _end.value, _end.value);
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
      detail::uninitialized_move(_end - 1, _end.value, _end.value);
      std::move_backward(p, _end - 1, p + 1);
      detail::destroy_at(p);
    }
    auto* x = detail::construct_at(p, std::forward<Args>(args)...);
    ++_end;
    return *x;
  }
  auto erase(const_iterator pos) -> iterator
  {
    assert(is_valid_iterator(pos));
    std::move(to_iterator(pos) + 1, end(), to_iterator(pos));
    detail::destroy_at(--_end);
    return pos;
  }
  auto erase(const_iterator first, const_iterator last) -> iterator
  {
    assert(is_valid_last_iterator(last));
    assert(first <= last);
    std::move(to_iterator(last), end(), to_iterator(first));
    detail::destroy(to_iterator(last), end());
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
    detail::construct_at(_end.value, std::forward<Args>(args)...);
    return *_end++;
  }
  auto pop_back() -> void
  {
    assert(!empty());
    detail::destroy_at(--_end);
  }
  auto resize(size_type count) -> void
  {
    if(count > size())
    {
      auto const delta = count - size();
      reserve(count);
      detail::uninitialized_default_construct_n(_end, delta);
      _end += delta;
    }
    else if(count < size())
    {
      auto const delta = size() - count;
      detail::destroy(_end - delta, _end.value);
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
      detail::uninitialized_fill_n(_end, count, value);
      _end += delta;
    }
    else if(count < old_size)
    {
      auto const delta = old_size - count;
      detail::destroy(_end - delta, _end.value);
      _end -= delta;
      shrink_to_fit();
    }
  }
  auto swap(pinned_vector& other) noexcept -> void
  {
    using std::swap;
    swap(_storage, other._storage);
    swap(_end, other._end);
  }

private:
  auto grow_if_necessary(std::size_t n) -> void
  {
    assert(max_size() - size() >= n);
    auto const new_size = size() + n;
    if(new_size > capacity())
    {
      auto const new_cap = capacity() * Traits::growth_factor::num / Traits::growth_factor::den;
      reserve(std::min(max_size(), std::max(new_cap, new_size)));
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
  auto to_pointer(const_iterator it) const noexcept -> T const* { return data() + (it - cbegin()); }
  auto to_pointer(const_iterator it) noexcept -> T* { return data() + (it - cbegin()); }

  typename Traits::page_stack _storage;
  detail::value_init_when_moved_from<T*> _end = data();
};

namespace mknejp
{
  namespace vmcontainer
  {
    template<typename T, typename Traits>
    auto swap(pinned_vector<T, Traits>& lhs, pinned_vector<T, Traits>& rhs) noexcept -> void
    {
      lhs.swap(rhs);
    }

    template<typename T, typename Traits, typename = decltype(std::declval<T const&>() == std::declval<T const&>())>
    auto operator==(pinned_vector<T, Traits> const& lhs, pinned_vector<T, Traits> const& rhs) -> bool
    {
      return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
    template<typename T, typename Traits, typename = decltype(std::declval<T const&>() == std::declval<T const&>())>
    auto operator!=(pinned_vector<T, Traits> const& lhs, pinned_vector<T, Traits> const& rhs) -> bool
    {
      return !(lhs == rhs);
    }
    template<typename T, typename Traits, typename = decltype(std::declval<T const&>() == std::declval<T const&>())>
    auto operator<(pinned_vector<T, Traits> const& lhs, pinned_vector<T, Traits> const& rhs) -> bool
    {
      return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
    template<typename T, typename Traits, typename = decltype(std::declval<T const&>() == std::declval<T const&>())>
    auto operator>(pinned_vector<T, Traits> const& lhs, pinned_vector<T, Traits> const& rhs) -> bool
    {
      return rhs < lhs;
    }
    template<typename T, typename Traits, typename = decltype(std::declval<T const&>() == std::declval<T const&>())>
    auto operator<=(pinned_vector<T, Traits> const& lhs, pinned_vector<T, Traits> const& rhs) -> bool
    {
      return !(rhs < lhs);
    }
    template<typename T, typename Traits, typename = decltype(std::declval<T const&>() == std::declval<T const&>())>
    auto operator>=(pinned_vector<T, Traits> const& lhs, pinned_vector<T, Traits> const& rhs) -> bool
    {
      return !(lhs < rhs);
    }
  }
}
