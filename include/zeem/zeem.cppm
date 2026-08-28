// Copyright Maarten L. Hekkelman 2026
// SPDX-License-Identifier: BSD-2-Clause

module;

#include <cassert>
#include <cstdint>

// import std;

#include <algorithm>
#include <cassert>
#include <charconv>
#include <chrono>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <istream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <source_location>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#if ZEEM_USE_DATE_H
# include <date/date.h>
# include <date/tz.h>
#endif

export module zeem;

#define ZEEM_EXPORT export
#define ZEEM_INLINE

#if defined(_WIN32) && defined(ZEEM_SHARED_BUILD)
# define ZEEM_API __declspec(dllexport)
#else
# define ZEEM_API
#endif

// clang-format off
#include "detail/charconv.hpp"
#include "doctype.hpp"
#include "version.hpp"
#include "error.hpp"
#include "text.hpp"
#include "node.hpp"
#include "xpath.hpp"
#include "parser.hpp"
#include "document.hpp"
#include "serialize.hpp"
// clang-format on
