// Copyright Maarten L. Hekkelman 2026
// SPDX-License-Identifier: BSD-2-Clause

module;

#include <cassert>
#include <cstdint>

import std;

export module zeem;

#define ZEEM_EXPORT export
#define ZEEM_INLINE

#include "detail/charconv.hpp"
#include "doctype.hpp"
#include "version.hpp"
#include "error.hpp"
#include "text.hpp"
#include "node.hpp"
#include "parser.hpp"
#include "document.hpp"
#include "serialize.hpp"
#include "xpath.hpp"
