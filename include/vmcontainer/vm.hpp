//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#pragma once
#include "vmcontainer/detail.hpp"

#include <cassert>
#include <cstddef>
#include <memory>

namespace mknejp
{
  namespace vmcontainer
  {
    namespace vm
    {
      struct system_default;

      class reservation;

      class page_stack;
    }
    namespace detail
    {
      template<typename VirtualMemorySystem>
      class reservation;

      template<typename VirtualMemorySystem>
      class page_stack;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// default_vm_traits
//

struct mknejp::vmcontainer::vm::system_default
{
  static auto reserve(std::size_t num_bytes) -> void*;
  static auto free(void* offset, std::size_t num_bytes) -> void;
  static auto commit(void* offset, std::size_t num_bytes) -> void;
  static auto decommit(void* offset, std::size_t num_bytes) -> void;

  static auto page_size() noexcept -> std::size_t;
};

///////////////////////////////////////////////////////////////////////////////
// reservation
//

template<typename VirtualMemorySystem>
class mknejp::vmcontainer::detail::reservation
{
public:
  reservation() = default;
  explicit reservation(std::size_t num_bytes)
  {
    if(num_bytes > 0)
    {
      num_bytes = round_up(num_bytes, VirtualMemorySystem::page_size());
      _reservation.reset(VirtualMemorySystem::reserve(num_bytes));
      _reservation.get_deleter().reserved_bytes = num_bytes;
    }
  }

  auto base() const noexcept -> void* { return _reservation.get(); }
  auto reserved_bytes() const noexcept -> std::size_t { return _reservation.get_deleter().reserved_bytes; }

private:
  struct deleter
  {
    auto operator()(void* p) const -> void { VirtualMemorySystem::free(p, reserved_bytes); }
    value_init_when_moved_from<std::size_t> reserved_bytes = 0;
  };

  std::unique_ptr<void, deleter> _reservation;
};

class mknejp::vmcontainer::vm::reservation : public detail::reservation<system_default>
{
  using detail::reservation<system_default>::reservation;
};

///////////////////////////////////////////////////////////////////////////////
// page_stack
//

template<typename VirtualMemorySystem>
class mknejp::vmcontainer::detail::page_stack
{
public:
  page_stack() = default;
  explicit page_stack(reservation_size_t reserved_bytes)
    : _reservation(reserved_bytes.num_bytes(VirtualMemorySystem::page_size()))
  {}

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
  auto page_size() const noexcept -> std::size_t { return VirtualMemorySystem::page_size(); }

private:
  reservation<VirtualMemorySystem> _reservation;
  value_init_when_moved_from<std::size_t> _committed_bytes = 0;
};

class mknejp::vmcontainer::vm::page_stack : public detail::page_stack<system_default>
{
  using detail::page_stack<system_default>::page_stack;
};
