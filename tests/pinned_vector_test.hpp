//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "vmcontainer/pinned_vector.hpp"

#include "allocator_mocks.hpp"

namespace vmcontainer_test
{
  template<typename Alloc>
  struct pinned_vector_test_traits
  {
    using commit_stack = mknejp::vmcontainer::detail::commit_stack<typename Alloc::vm_stub>;
    using growth_factor = typename mknejp::vmcontainer::pinned_vector_traits::growth_factor;
  };

  template<typename T, typename Traits>
  struct pinned_vector_value_state
  {
    using container = typename mknejp::vmcontainer::pinned_vector<T, Traits>;

    pinned_vector_value_state(container const& c)
      : begin(c.begin()), end(c.end()), data(c.data()), size(c.size()), max_size(c.max_size()), empty(c.empty())
    {}

    typename container::const_iterator begin;
    typename container::const_iterator end;
    T const* data;
    typename container::size_type size;
    typename container::size_type max_size;
    bool empty;

    auto operator==(pinned_vector_value_state const& other) const -> bool
    {
      return begin == other.begin && end == other.end && data == other.data && size == other.size
             && max_size == other.max_size && empty == other.empty;
    }
  };

  template<typename T, typename Traits>
  auto capture_value_state(typename mknejp::vmcontainer::pinned_vector<T, Traits> const& c)
  {
    return pinned_vector_value_state<T, Traits>(c);
  }
}
