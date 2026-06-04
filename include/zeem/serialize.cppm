// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#if ZEEM_USE_DATE_H
# include <date/date.h>
# include <date/tz.h>
#endif

#include <algorithm>
#include <charconv>
#include <chrono>
#include <map>
#include <optional>
#include <regex>
#include <source_location>
#include <string>
#include <system_error>

export module zeem:serialize;

import :node;
import :charconv;

#define IN_MODULE_INTERFACE
#define ZEEM_EXPORT export
#define ZEEM_INLINE

#include "serialize.hpp"
