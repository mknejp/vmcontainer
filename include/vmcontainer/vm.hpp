//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#pragma once
#include "vmcontainer/detail.hpp"

#include <cassert>
#include <cstddef>

namespace mknejp
{
  namespace vmcontainer
  {
    namespace vm
    {
      auto reserve(std::size_t num_bytes) -> void*;
      auto free(void* offset, std::size_t num_bytes) -> void;
      auto commit(void* offset, std::size_t num_bytes) -> void;
      auto decommit(void* offset, std::size_t num_bytes) -> void;

      auto page_size() noexcept -> std::size_t;

      struct default_vm_traits;

      class reservation;

      class page_stack;

      namespace detail
      {
        using vmcontainer::detail::round_up;

        template<typename VirtualMemoryTraits>
        class reservation;

        template<typename VirtualMemoryTraits>
        class page_stack;
      }

    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// default_vm_traits
//

struct mknejp::vmcontainer::vm::default_vm_traits
{
  static auto reserve(std::size_t num_bytes) -> void* { return vm::reserve(num_bytes); }
  static auto free(void* offset, std::size_t num_bytes) -> void { return vm::free(offset, num_bytes); }
  static auto commit(void* offset, std::size_t num_bytes) -> void { return vm::commit(offset, num_bytes); }
  static auto decommit(void* offset, std::size_t num_bytes) -> void { return vm::decommit(offset, num_bytes); }

  static auto page_size() noexcept -> std::size_t { return vm::page_size(); }
};

///////////////////////////////////////////////////////////////////////////////
// reservation
//

template<typename VirtualMemoryTraits>
class mknejp::vmcontainer::vm::detail::reservation
{
public:
  reservation() = default;
  explicit reservation(std::size_t num_bytes)
  {
    if(num_bytes > 0)
    {
      _reserved_bytes = round_up(num_bytes, VirtualMemoryTraits::page_size());
      _base = VirtualMemoryTraits::reserve(_reserved_bytes);
    }
  }
  reservation(reservation const& other) = delete;
  reservation(reservation&& other) noexcept { swap(*this, other); }
  reservation& operator=(reservation const& other) = delete;
  reservation& operator=(reservation&& other) & noexcept
  {
    auto temp = std::move(other);
    swap(*this, temp);
    return *this;
  }
  ~reservation()
  {
    if(_base)
    {
      VirtualMemoryTraits::free(base(), reserved_bytes());
    }
  }

  friend void swap(reservation& lhs, reservation& rhs) noexcept
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

class mknejp::vmcontainer::vm::reservation : public detail::reservation<default_vm_traits>
{
  using detail::reservation<default_vm_traits>::reservation;
};

///////////////////////////////////////////////////////////////////////////////
// page_stack
//

template<typename VirtualMemoryTraits>
class mknejp::vmcontainer::vm::detail::page_stack
{
public:
  page_stack() = default;
  explicit page_stack(std::size_t num_bytes) : _reservation(num_bytes) {}
  page_stack(page_stack const& other) = delete;
  page_stack(page_stack&& other) noexcept { swap(*this, other); }
  auto operator=(page_stack const& other) = delete;
  auto operator=(page_stack&& other) & noexcept -> page_stack&
  {
    auto temp = std::move(other);
    swap(*this, temp);
    return *this;
  }
  ~page_stack()
  {
    if(committed_bytes() > 0)
    {
      VirtualMemoryTraits::decommit(base(), committed_bytes());
    }
  }

  auto commit(std::size_t bytes) -> void
  {
    if(bytes > 0)
    {
      auto const new_committed = round_up(committed_bytes() + bytes, page_size());
      assert(new_committed <= reserved_bytes());
      VirtualMemoryTraits::commit(static_cast<char*>(base()) + committed_bytes(), new_committed - committed_bytes());
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
        VirtualMemoryTraits::decommit(static_cast<char*>(base()) + new_committed, committed_bytes() - new_committed);
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

  friend void swap(page_stack& lhs, page_stack& rhs) noexcept
  {
    using std::swap;
    swap(lhs._reservation, rhs._reservation);
    swap(lhs._committed_bytes, rhs._committed_bytes);
    swap(lhs._page_size, rhs._page_size);
  }

private:
  reservation<VirtualMemoryTraits> _reservation;
  std::size_t _committed_bytes = 0;
  std::size_t _page_size = VirtualMemoryTraits::page_size();
};

class mknejp::vmcontainer::vm::page_stack : public detail::page_stack<default_vm_traits>
{
  using detail::page_stack<default_vm_traits>::page_stack;
};
