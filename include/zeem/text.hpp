// Copyright (c) 2024 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

/**
 * \file
 * various definitions of data types and routines used to work with Unicode encoded text
 */

#ifndef ZEEM_CXX_MODULE
# include <string>
# include <string_view>
#endif

namespace zeem
{

/// some character classification routines

ZEEM_EXPORT bool is_name_start_char(char32_t uc);             ///< is the character a 'name_start_char'?
ZEEM_EXPORT bool is_name_char(char32_t uc);                   ///< is the character a 'name_char'?
ZEEM_EXPORT bool is_space(char32_t uc);                       ///< is the character a 'space'?
ZEEM_EXPORT bool is_valid_xml_1_0_char(char32_t uc);          ///< is the character a 'valid_xml_1_0_char'?
ZEEM_EXPORT bool is_valid_xml_1_1_char(char32_t uc);          ///< is the character a 'valid_xml_1_1_char'?
ZEEM_EXPORT bool is_valid_system_literal_char(char32_t uc);   ///< is the character a 'valid_system_literal_char'?
ZEEM_EXPORT bool is_valid_system_literal(std::string_view s); ///< is the character a 'valid_system_literal'?
ZEEM_EXPORT bool is_valid_public_id_char(char32_t uc);        ///< is the character a 'valid_public_id_char'?
ZEEM_EXPORT bool is_valid_public_id(std::string_view s);      ///< is the character a 'valid_public_id'?

/// \brief the (admittedly limited) set of supported text encodings in libzeem
///
/// these are the supported encodings. Perhaps we should extend this list a bit?
ZEEM_EXPORT enum class encoding_type {
	ASCII,   ///< 7-bit ascii
	UTF8,    ///< UTF-8
	UTF16BE, ///< UTF-16 Big Endian
	UTF16LE, ///< UTF 16 Little Endian
	ISO88591 ///< Default single byte encoding, is a subset of utf-8
};

/// \brief Append a single unicode character to an utf-8 string
ZEEM_EXPORT void append(std::string &s, char32_t uc);

/// \brief remove the last unicode character from an utf-8 string
ZEEM_EXPORT char32_t pop_back_char(std::string &s);

/// \brief return the first unicode and advance the pointer @a ptr from a string
ZEEM_EXPORT char32_t pop_front_char(std::string::const_iterator &ptr, std::string::const_iterator end);

ZEEM_EXPORT char32_t pop_front_char(std::string_view::const_iterator &ptr, std::string_view::const_iterator end);

/// \brief A simple implementation of trim, removing white space from start and end of \a s
ZEEM_EXPORT void trim(std::string &s);

} // namespace zeem
