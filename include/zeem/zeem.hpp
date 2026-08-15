// Copyright (c) 2024-2026 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

/** @file
 * Main module definition for zeem.
 */

#ifndef ZEEM_CXX_MODULE
/** \brief Expands to nothing in the non-module build; marks exported symbols in the C++20 module build. */
# define ZEEM_EXPORT
/** \brief Expands to inline in the non-module build; marks exported inline functions in the C++20 module build. */
# define ZEEM_INLINE inline

// IWYU pragma: begin_exports
# include "doctype.hpp"
# include "document.hpp"
# include "error.hpp"
# include "node.hpp"
# include "parser.hpp"
# include "serialize.hpp"
# include "text.hpp"
# include "version.hpp"
# include "xpath.hpp"
// IWYU pragma: end_exports

#endif