//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "vmcontainer/vm.hpp"

#ifdef WIN32
#  define NOMINMAX
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <sys/mman.h>
#  include <unistd.h>
#endif

#include <cassert>
#include <stdexcept>
#include <system_error>

auto mknejp::vmcontainer::vm::system_default::reserve(std::size_t num_bytes) -> void*
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
#else
  auto const offset = ::mmap(nullptr, num_bytes, PROT_NONE, MAP_ANON | MAP_PRIVATE, 0, 0);
  if(offset == MAP_FAILED)
  {
    throw std::system_error(std::error_code(errno, std::system_category()), "virtual memory reservation failed");
  }
  return offset;
#endif
}

auto mknejp::vmcontainer::vm::system_default::free(void* offset, std::size_t num_bytes) -> void
{
#ifdef WIN32
  auto const result = ::VirtualFree(offset, 0, MEM_RELEASE);
  (void)result;
  assert(result != 0);
#else
  auto const result = ::munmap(offset, num_bytes);
  (void)result;
  assert(result == 0);
#endif
}

auto mknejp::vmcontainer::vm::system_default::commit(void* offset, std::size_t num_bytes) -> void
{
  assert(num_bytes > 0);

#ifdef WIN32
  auto const result = ::VirtualAlloc(offset, num_bytes, MEM_COMMIT, PAGE_READWRITE);
  if(result == nullptr)
  {
    throw std::bad_alloc();
  }
#else
  auto const result = ::mprotect(offset, num_bytes, PROT_READ | PROT_WRITE);
  if(result != 0)
  {
    throw std::bad_alloc();
  }
#endif
}

auto mknejp::vmcontainer::vm::system_default::decommit(void* offset, std::size_t num_bytes) -> void
{
#ifdef WIN32
  auto const result = ::VirtualFree(offset, num_bytes, MEM_DECOMMIT);
  (void)result;
  assert(result != 0);
#else
  auto const result1 = ::madvise(offset, num_bytes, MADV_DONTNEED);
  (void)result1;
  assert(result1 == 0);
  auto const result2 = ::mprotect(offset, num_bytes, PROT_NONE);
  (void)result2;
  assert(result2 == 0);
#endif
}

std::size_t const mknejp::vmcontainer::vm::system_default::_page_size =
#ifdef WIN32
  []() {
    auto info = SYSTEM_INFO{};
    ::GetSystemInfo(&info);
    return static_cast<std::size_t>(info.dwPageSize);
  }();
#else
  static_cast<std::size_t>(::getpagesize());
#endif
