//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "pinned_vector.hpp"

#ifdef WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#error BOOO
#endif

#include <cassert>
#include <stdexcept>
#include <system_error>

auto mknejp::detail::_pinned_vector::virtual_memory_allocator::reserve(std::size_t num_bytes) -> void*
{
  assert(num_bytes > 0);

#ifdef WIN32
  auto const offset = ::VirtualAlloc(nullptr, num_bytes, MEM_RESERVE, PAGE_NOACCESS);
  if(offset == nullptr)
  {
    auto const err = ::GetLastError();
    throw std::system_error(std::error_code(err, std::system_category()), "virtual memory reservation failed");
  }
  return offset;
#endif
}

auto mknejp::detail::_pinned_vector::virtual_memory_allocator::free(void* offset) -> void
{
#ifdef WIN32
  assert(::VirtualFree(offset, 0, MEM_RELEASE) != 0);
#endif
}

auto mknejp::detail::_pinned_vector::virtual_memory_allocator::commit(void* offset, std::size_t num_bytes) -> void
{
  assert(num_bytes > 0);

#ifdef WIN32
  auto const result = ::VirtualAlloc(offset, num_bytes, MEM_COMMIT, PAGE_READWRITE);
  if(result == nullptr)
  {
    throw std::bad_alloc();
  }
#endif
}

auto mknejp::detail::_pinned_vector::virtual_memory_allocator::decommit(void* offset, std::size_t num_bytes) -> void
{
#ifdef WIN32
  assert(::VirtualFree(offset, 0, MEM_DECOMMIT) != 0);
#endif
}

auto mknejp::detail::_pinned_vector::virtual_memory_allocator::page_size() noexcept -> std::size_t
{
#ifdef WIN32
  static auto const size = [] {
    auto info = SYSTEM_INFO{};
    ::GetSystemInfo(&info);
    return static_cast<std::size_t>(info.dwPageSize);
  }();
  return size;
#endif
}
