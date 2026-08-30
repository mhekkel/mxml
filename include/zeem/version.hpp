// Copyright (c) 2024-2026 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

/**
 * \file
 * File containing the version_type struct
 */

#ifndef ZEEM_CXX_MODULE
# include "zeem/export.hpp"
# include <compare>
# include <cstdint>
#endif

namespace zeem
{

// --------------------------------------------------------------------

/**
 * @brief struct for the XML version
 *
 */

struct version_type
{
	uint8_t major; ///< major, usually 1
	uint8_t minor; ///< minor, usually 0 or 1

	/// \brief Compare two versions, comparing major first and then minor
	constexpr auto operator<=>(const version_type &) const = default;
};

} // namespace zeem