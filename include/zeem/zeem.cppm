// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#if __has_include(<experimental/type_traits>)
# include <experimental/type_traits>
#endif
#include <algorithm>
#include <cassert>
#include <charconv>
#include <chrono>
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
#include <regex>
#include <source_location>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

export module zeem;

#define IN_MODULE_INTERFACE
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
