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
      struct virtual_memory_system;

      class virtual_memory_reservation;

      class virtual_memory_page_stack;

      namespace detail
      {
        using vmcontainer::detail::round_up;

        template<typename VirtualMemorySystem>
        class virtual_memory_reservation;

        template<typename VirtualMemorySystem>
        class virtual_memory_page_stack;
      }

    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// virtual_memory_system
//

struct mknejp::vmcontainer::vm::virtual_memory_system
{
  static auto reserve(std::size_t num_bytes) -> void* { return vm::reserve(num_bytes); }
  static auto free(void* offset, std::size_t num_bytes) -> void { return vm::free(offset, num_bytes); }
  static auto commit(void* offset, std::size_t num_bytes) -> void { return vm::commit(offset, num_bytes); }
  static auto decommit(void* offset, std::size_t num_bytes) -> void { return vm::decommit(offset, num_bytes); }

  static auto page_size() noexcept -> std::size_t { return vm::page_size(); }
};

///////////////////////////////////////////////////////////////////////////////
// virtual_memory_reservation
//

template<typename VirtualMemorySystem>
class mknejp::vmcontainer::vm::detail::virtual_memory_reservation
{
public:
  virtual_memory_reservation() = default;
  explicit virtual_memory_reservation(std::size_t num_bytes)
  {
    if(num_bytes > 0)
    {
      _reserved_bytes = round_up(num_bytes, VirtualMemoryTraits::page_size());
      _base = VirtualMemorySystem::reserve(_reserved_bytes);
    }
  }
  virtual_memory_reservation(virtual_memory_reservation const& other) = delete;
  virtual_memory_reservation(virtual_memory_reservation&& other) noexcept { swap(*this, other); }
  virtual_memory_reservation& operator=(virtual_memory_reservation const& other) = delete;
  virtual_memory_reservation& operator=(virtual_memory_reservation&& other) & noexcept
  {
    auto temp = std::move(other);
    swap(*this, temp);
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

class mknejp::vmcontainer::vm::virtual_memory_reservation : public detail::virtual_memory_reservation<virtual_memory_system>
{
  using detail::reservation<virtual_memory_system>::reservation;
};

///////////////////////////////////////////////////////////////////////////////
// virtual_memory_page_stack
//

template<typename VirtualMemorySystem>
class mknejp::vmcontainer::vm::detail::virtual_memory_page_stack
{
public:
  virtual_memory_page_stack() = default;
  explicit virtual_memory_page_stack(std::size_t num_bytes) : _reservation(num_bytes) {}
  virtual_memory_page_stack(virtual_memory_page_stack const& other) = delete;
  virtual_memory_page_stack(virtual_memory_page_stack&& other) noexcept { swap(*this, other); }
  auto operator=(virtual_memory_page_stack const& other) = delete;
  auto operator=(virtual_memory_page_stack&& other) & noexcept -> page_stack&
  {
    auto temp = std::move(other);
    swap(*this, temp);
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
  reservation<VirtualMemorySystem> _reservation;
  std::size_t _committed_bytes = 0;
  std::size_t _page_size = VirtualMemorySystem::page_size();
};

class mknejp::vmcontainer::vm::virtual_memory_page_stack : public detail::virtual_memory_page_stack<virtual_memory_system>
{
  using detail::virtual_memory_page_stack<virtual_memory_system>::page_stack;
};
