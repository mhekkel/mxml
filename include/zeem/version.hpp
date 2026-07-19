// Copyright (c) 2024 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

/**
 * \file
 * File containing the version_type struct
 */

#ifndef ZEEM_CXX_MODULE
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

	constexpr auto operator<=>(const version_type &) const = default;
};

} // namespace zeem