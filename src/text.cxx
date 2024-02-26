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

module;

#include <string>

module mxml;

import :error;
import :text;

namespace mxml
{

// some very basic code to check the class of scanned characters

bool is_name_start_char(char32_t uc)
{
	return uc == L':' or
	       (uc >= L'A' and uc <= L'Z') or
	       uc == L'_' or
	       (uc >= L'a' and uc <= L'z') or
	       (uc >= 0x0C0 and uc <= 0x0D6) or
	       (uc >= 0x0D8 and uc <= 0x0F6) or
	       (uc >= 0x0F8 and uc <= 0x02FF) or
	       (uc >= 0x0370 and uc <= 0x037D) or
	       (uc >= 0x037F and uc <= 0x01FFF) or
	       (uc >= 0x0200C and uc <= 0x0200D) or
	       (uc >= 0x02070 and uc <= 0x0218F) or
	       (uc >= 0x02C00 and uc <= 0x02FEF) or
	       (uc >= 0x03001 and uc <= 0x0D7FF) or
	       (uc >= 0x0F900 and uc <= 0x0FDCF) or
	       (uc >= 0x0FDF0 and uc <= 0x0FFFD) or
	       (uc >= 0x010000 and uc <= 0x0EFFFF);
}

bool is_name_char(char32_t uc)
{
	return uc == '-' or
	       uc == '.' or
	       (uc >= '0' and uc <= '9') or
	       uc == 0x0B7 or
	       is_name_start_char(uc) or
	       (uc >= 0x00300 and uc <= 0x0036F) or
	       (uc >= 0x0203F and uc <= 0x02040);
}

bool is_valid_xml_1_0_char(char32_t uc)
{
	return uc == 0x09 or
	       uc == 0x0A or
	       uc == 0x0D or
	       (uc >= 0x020 and uc <= 0x0D7FF) or
	       (uc >= 0x0E000 and uc <= 0x0FFFD) or
	       (uc >= 0x010000 and uc <= 0x010FFFF);
}

bool is_valid_xml_1_1_char(char32_t uc)
{
	return uc == 0x09 or
	       uc == 0x0A or
	       uc == 0x0D or
	       (uc >= 0x020 and uc < 0x07F) or
	       uc == 0x085 or
	       (uc >= 0x0A0 and uc <= 0x0D7FF) or
	       (uc >= 0x0E000 and uc <= 0x0FFFD) or
	       (uc >= 0x010000 and uc <= 0x010FFFF);
}

bool is_valid_system_literal_char(char32_t uc)
{
	return uc > 0x1f and
	       uc != ' ' and
	       uc != '<' and
	       uc != '>' and
	       uc != '"' and
	       uc != '#';
}

bool is_valid_system_literal(const std::string &s)
{
	bool result = true;
	for (std::string::const_iterator ch = s.begin(); result == true and ch != s.end(); ++ch)
		result = is_valid_system_literal_char(*ch);
	return result;
}

bool is_valid_public_id_char(char32_t uc)
{
	static const std::string kPubChars(" \r\n-'()+,./:=?;!*#@$_%");

	return (uc >= 'a' and uc <= 'z') or
	       (uc >= 'A' and uc <= 'Z') or
	       (uc >= '0' and uc <= '9') or
	       (uc < 128 and kPubChars.find(static_cast<char>(uc)) != std::string::npos);
}

bool is_valid_public_id(const std::string &s)
{
	bool result = true;
	for (std::string::const_iterator ch = s.begin(); result == true and ch != s.end(); ++ch)
		result = is_valid_public_id_char(*ch);
	return result;
}

} // namespace mxml