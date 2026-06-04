// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#include <functional>
#include <istream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module zeem:parser;

import :error;
import :text;
import :version;

#define IN_MODULE_INTERFACE
#define ZEEM_EXPORT export
#define ZEEM_INLINE

#include "parser.hpp"
