// Copyright Maarten L. Hekkelman 2026
//
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
#include <experimental/type_traits>
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
