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
#elif __APPLE__
#include <sys/mman.h>
#include <unistd.h>
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
#elif __APPLE__
  auto const offset = ::mmap(nullptr, num_bytes, PROT_NONE, MAP_ANON | MAP_PRIVATE, 0, 0);
  if(offset == MAP_FAILED)
  {
    throw std::system_error(std::error_code(errno, std::system_category()), "virtual memory reservation failed");
  }
  return offset;
#endif
}

auto mknejp::detail::_pinned_vector::virtual_memory_allocator::free(void* offset, std::size_t num_bytes) -> void
{
#ifdef WIN32
  assert(::VirtualFree(offset, num_bytes, MEM_RELEASE) != 0);
#elif __APPLE__
  assert(::munmap(offset, num_bytes) == 0);
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
#elif __APPLE__
  auto const result = ::mprotect(offset, num_bytes, PROT_READ | PROT_WRITE);
  if(result != 0)
  {
    throw std::bad_alloc();
  }
#endif
}

auto mknejp::detail::_pinned_vector::virtual_memory_allocator::decommit(void* offset, std::size_t num_bytes) -> void
{
#ifdef WIN32
  assert(::VirtualFree(offset, 0, MEM_DECOMMIT) != 0);
#elif __APPLE__
  assert(::madvise(offset, num_bytes, MADV_DONTNEED) != 0);
  assert(::mprotect(offset, num_bytes, PROT_NONE) == 0);
#endif
}

auto mknejp::detail::_pinned_vector::virtual_memory_allocator::page_size() noexcept -> std::size_t
{
#ifdef WIN32
  auto info = SYSTEM_INFO{};
  ::GetSystemInfo(&info);
  return static_cast<std::size_t>(info.dwPageSize);
#elif __APPLE__
  return static_cast<std::size_t>(::getpagesize());
#endif
}
