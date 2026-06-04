// Copyright Maarten L. Hekkelman 2026 
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#if __has_include(<experimental/type_traits>)
# include <experimental/type_traits>
#endif
#include <charconv>
#include <type_traits>
#include <utility>

export module zeem:charconv;

#define IN_MODULE_INTERFACE
#define ZEEM_EXPORT export
#define ZEEM_INLINE

#include "charconv.hpp"
