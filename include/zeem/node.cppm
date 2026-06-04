// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#include <algorithm>
#include <cassert>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iosfwd>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

export module zeem:node;

import :error;
import :version;

#define IN_MODULE_INTERFACE
#define ZEEM_EXPORT export
#define ZEEM_INLINE

#include "node.hpp"
