// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

export module zeem:xpath;

import :node;

#define IN_MODULE_INTERFACE
#define ZEEM_EXPORT export
#define ZEEM_INLINE

#include "xpath.hpp"
