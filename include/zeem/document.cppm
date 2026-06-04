// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

export module zeem:document;

#define IN_MODULE_INTERFACE
#define ZEEM_EXPORT export
#define ZEEM_INLINE

import :node;
import :parser;
import :version;
import :text;

#include "document.hpp"
