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
      class commit_stack;
      class page_stack;
    }
    namespace detail
    {
      template<typename VirtualMemorySystem>
      class reservation;

      template<typename VirtualMemorySystem>
      class page_stack;

      template<typename VirtualMemorySystem>
      class commit_stack;
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

  static auto page_size() noexcept -> std::size_t { return _page_size; }

private:
  static std::size_t const _page_size;
};

///////////////////////////////////////////////////////////////////////////////
// reservation
//

template<typename VirtualMemorySystem>
class mknejp::vmcontainer::detail::reservation
{
public:
  reservation() = default;
  explicit reservation(reservation_size_t reserved_bytes)
  {
    auto num_bytes = reserved_bytes.num_bytes(VirtualMemorySystem::page_size());
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
// commit_stack
//

template<typename VirtualMemorySystem>
class mknejp::vmcontainer::detail::commit_stack
{
public:
  commit_stack() = default;
  explicit commit_stack(reservation_size_t reserved_bytes) : _reservation(reserved_bytes) {}
  explicit commit_stack(reservation<VirtualMemorySystem> reservation) : _reservation(std::move(reservation)) {}

  auto commit(std::size_t new_bytes) -> std::size_t
  {
    new_bytes = round_up(new_bytes, page_size());
    if(new_bytes > committed_bytes())
    {
      VirtualMemorySystem::commit(static_cast<char*>(base()) + committed_bytes(), new_bytes - committed_bytes());
    }
    else if(new_bytes < committed_bytes())
    {
      VirtualMemorySystem::decommit(static_cast<char*>(base()) + new_bytes, committed_bytes() - new_bytes);
    }
    _committed_bytes = new_bytes;
    return committed_bytes();
  }

  auto base() const noexcept -> void* { return _reservation.base(); }
  auto committed_bytes() const noexcept -> std::size_t { return _committed_bytes; }
  auto reserved_bytes() const noexcept -> std::size_t { return _reservation.reserved_bytes(); }
  auto page_size() const noexcept -> std::size_t { return VirtualMemorySystem::page_size(); }

private:
  reservation<VirtualMemorySystem> _reservation;
  value_init_when_moved_from<std::size_t> _committed_bytes = 0;
};

class mknejp::vmcontainer::vm::commit_stack : public detail::commit_stack<system_default>
{
  using detail::commit_stack<system_default>::commit_stack;
};

///////////////////////////////////////////////////////////////////////////////
// page_stack
//

template<typename VirtualMemorySystem>
class mknejp::vmcontainer::detail::page_stack
{
public:
  using reservation_type = reservation<VirtualMemorySystem>;

  page_stack() = default;
  explicit page_stack(reservation_size_t reserved_bytes) : _reservation(reserved_bytes) {}

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

  auto reservation() const & noexcept -> reservation_type const& { return _reservation; }
  auto reservation() && noexcept -> reservation_type&& { return std::move(_reservation); }

private:
  reservation_type _reservation;
  value_init_when_moved_from<std::size_t> _committed_bytes = 0;
};

class mknejp::vmcontainer::vm::page_stack : public detail::page_stack<system_default>
{
  using detail::page_stack<system_default>::page_stack;
};
