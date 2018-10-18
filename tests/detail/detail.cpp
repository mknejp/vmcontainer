//
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//

#include "vmcontainer/detail.hpp"

#include <catch.hpp>

#include <type_traits>

using namespace mknejp::vmcontainer;

///////////////////////////////////////////////////////////////////////////////
// reservation_size_t
//

static_assert(num_bytes(5).num_bytes(1000) == 5, "");
static_assert(num_pages(5).num_bytes(1000) == 5 * 1000, "");

///////////////////////////////////////////////////////////////////////////////
// max_size_t
//

static_assert(max_elements(5).scaled_for_type<int>().num_bytes(1000) == 5 * sizeof(int), "");
static_assert(max_bytes(5).scaled_for_type<int>().num_bytes(1000) == 5, "");
static_assert(max_pages(5).scaled_for_type<int>().num_bytes(1000) == 5 * 1000, "");
