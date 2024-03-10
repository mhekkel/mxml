/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Maarten L. Hekkelman
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

/**
 * \file
 * various definitions of data types and routines used to work with Unicode encoded text
 */

#include <string>

namespace mxml
{

/// some character classification routines

bool is_name_start_char(char32_t uc);               ///< is the character a 'name_start_char'?
bool is_name_char(char32_t uc);                     ///< is the character a 'name_char'?
bool is_valid_xml_1_0_char(char32_t uc);            ///< is the character a 'valid_xml_1_0_char'?
bool is_valid_xml_1_1_char(char32_t uc);            ///< is the character a 'valid_xml_1_1_char'?
bool is_valid_system_literal_char(char32_t uc);     ///< is the character a 'valid_system_literal_char'?
bool is_valid_system_literal(const std::string &s); ///< is the character a 'valid_system_literal'?
bool is_valid_public_id_char(char32_t uc);          ///< is the character a 'valid_public_id_char'?
bool is_valid_public_id(const std::string &s);      ///< is the character a 'valid_public_id'?

/// \brief the (admittedly limited) set of supported text encodings in libmxml
///
/// these are the supported encodings. Perhaps we should extend this list a bit?
enum class encoding_type {
	ASCII,   ///< 7-bit ascii
	UTF8,    ///< UTF-8
	UTF16BE, ///< UTF-16 Big Endian
	UTF16LE, ///< UTF 16 Little Endian
	ISO88591 ///< Default single byte encoding, is a subset of utf-8
};

/// \brief Append a single unicode character to an utf-8 string
void append(std::string &s, char32_t uc);

/// \brief remove the last unicode character from an utf-8 string
char32_t pop_back_char(std::string &s);

/// \brief return the first unicode and advance the pointer @a ptr from a string
char32_t pop_front_char(std::string::const_iterator &ptr, std::string::const_iterator end);

/// \brief A simple implementation of trim, removing white space from start and end of \a s
void trim(std::string &s);

} // namespace mxml
