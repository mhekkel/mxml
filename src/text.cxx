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
#include <vector>

module mxml;

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

/// \brief Append a single unicode character to an utf-8 string
void append(std::string& s, char32_t uc)
{
	if (uc < 0x080)
		s += (static_cast<char>(uc));
	else if (uc < 0x0800)
	{
		char ch[2] = {
			static_cast<char>(0x0c0 | (uc >> 6)),
			static_cast<char>(0x080 | (uc & 0x3f))
		};
		s.append(ch, 2);
	}
	else if (uc < 0x00010000)
	{
		char ch[3] = {
			static_cast<char>(0x0e0 | (uc >> 12)),
			static_cast<char>(0x080 | ((uc >> 6) & 0x3f)),
			static_cast<char>(0x080 | (uc & 0x3f))
		};
		s.append(ch, 3);
	}
	else
	{
		char ch[4] = {
			static_cast<char>(0x0f0 | (uc >> 18)),
			static_cast<char>(0x080 | ((uc >> 12) & 0x3f)),
			static_cast<char>(0x080 | ((uc >> 6) & 0x3f)),
			static_cast<char>(0x080 | (uc & 0x3f))
		};
		s.append(ch, 4);
	}
}

/// \brief remove the last unicode character from an utf-8 string
char32_t pop_back_char(std::string& s)
{
	char32_t result = 0;

	if (not s.empty())
	{
		std::string::iterator ch = s.end() - 1;
		
		if ((*ch & 0x0080) == 0)
		{
			result = *ch;
			s.erase(ch);
		}
		else
		{
			int o = 0;
			
			do
			{
				result |= (*ch & 0x03F) << o;
				o += 6;
				--ch;
			}
			while (ch != s.begin() and (*ch & 0x0C0) == 0x080);
			
			switch (o)
			{
				case  6: result |= (*ch & 0x01F) <<  6; break;
				case 12: result |= (*ch & 0x00F) << 12; break;
				case 18: result |= (*ch & 0x007) << 18; break;
			}
			
			s.erase(ch, s.end());
		}
	}
	
	return result;
}

/// \brief return the first unicode and the advanced pointer from a string
char32_t pop_front_char(std::string::const_iterator &ptr, std::string::const_iterator end)
{
	char32_t result = static_cast<unsigned char>(*ptr);
	++ptr;

	if (result > 0x07f)
	{
		unsigned char ch[3];
		
		if ((result & 0x0E0) == 0x0C0)
		{
			if (ptr >= end)
				throw mxml::exception("Invalid utf-8");

			ch[0] = static_cast<unsigned char>(*ptr); ++ptr;

			if ((ch[0] & 0x0c0) != 0x080)
				throw mxml::exception("Invalid utf-8");

			result = ((result & 0x01F) << 6) | (ch[0] & 0x03F);
		}
		else if ((result & 0x0F0) == 0x0E0)
		{
			if (ptr + 1 >= end)
				throw mxml::exception("Invalid utf-8");

			ch[0] = static_cast<unsigned char>(*ptr); ++ptr;
			ch[1] = static_cast<unsigned char>(*ptr); ++ptr;

			if ((ch[0] & 0x0c0) != 0x080 or (ch[1] & 0x0c0) != 0x080)
				throw mxml::exception("Invalid utf-8");

			result = ((result & 0x00F) << 12) | ((ch[0] & 0x03F) << 6) | (ch[1] & 0x03F);
		}
		else if ((result & 0x0F8) == 0x0F0)
		{
			if (ptr + 2 >= end)
				throw mxml::exception("Invalid utf-8");

			ch[0] = static_cast<unsigned char>(*ptr); ++ptr;
			ch[1] = static_cast<unsigned char>(*ptr); ++ptr;
			ch[2] = static_cast<unsigned char>(*ptr); ++ptr;

			if ((ch[0] & 0x0c0) != 0x080 or (ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080)
				throw mxml::exception("Invalid utf-8");

			result = ((result & 0x007) << 18) | ((ch[0] & 0x03F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F);
		}
	}

	return result;
}

// --------------------------------------------------------------------

/// \brief A simple implementation of trim, removing white space from start and end of \a s
void trim(std::string& s)
{
	auto in = s.begin(), out = s.begin(), end = s.end();

	while (end != s.begin() and std::isspace(*(end - 1)))
		--end;

	while (in != end and std::isspace(*in))
		++in;
	
	if (in == end)
		s.clear();
	else if (in != out)
	{
		while (in != end)
			*out++ = *in++;
		s.erase(out, s.end());
	}
	else if (end != s.end())
		s.erase(end, s.end());
}

} // namespace mxml