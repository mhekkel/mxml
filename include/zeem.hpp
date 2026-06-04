// Copyright (c) 2024 Maarten L. Hekkelman
//
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

/** @file 
 * Main module definition for zeem.
*/

#define ZEEM_EXPORT
#define ZEEM_INLINE inline

// IWYU pragma: begin_exports
#include "zeem/doctype.hpp"
#include "zeem/document.hpp"
#include "zeem/error.hpp"
#include "zeem/node.hpp"
#include "zeem/parser.hpp"
#include "zeem/serialize.hpp"
#include "zeem/text.hpp"
#include "zeem/version.hpp"
#include "zeem/xpath.hpp"
// IWYU pragma: end_exports
