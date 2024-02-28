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

/// \file
/// various definitions of data types and routines used to work with Unicode encoded text

#include <cstdint>
#include <locale>
#include <vector>
#include <string>
#include <tuple>

export module mxml:text;

import :error;

namespace mxml
{

/// some character classification routines

export bool is_name_start_char(char32_t uc);
export bool is_name_char(char32_t uc);
export bool is_valid_xml_1_0_char(char32_t uc);
export bool is_valid_xml_1_1_char(char32_t uc);
export bool is_valid_system_literal_char(char32_t uc);
export bool is_valid_system_literal(const std::string& s);
export bool is_valid_public_id_char(char32_t uc);
export bool is_valid_public_id(const std::string& s);

/// \brief the (admittedly limited) set of supported text encodings in libmxml
/// 
/// these are the supported encodings. Perhaps we should extend this list a bit?
enum class encoding_type
{
	ASCII,			///< 7-bit ascii 
	UTF8,			///< UTF-8
	UTF16BE,		///< UTF-16 Big Endian
	UTF16LE,		///< UTF 16 Little Endian
	ISO88591		///< Default single byte encoding, is a subset of utf-8
};

/// \brief utf-8 is not single byte e.g.
constexpr bool is_single_byte_encoding(encoding_type enc)
{
	return enc == encoding_type::ASCII or enc == encoding_type::ISO88591 or enc == encoding_type::UTF8;
}

template <encoding_type enc>
constexpr bool is_single_byte_encoding_v = is_single_byte_encoding(enc);

/// \brief our own implementation of iequals: compares \a a with \a b case-insensitive
///
/// This is a limited use function, works only reliably with ASCII. But that's OK.
bool iequals(const std::string& a, const std::string& b)
{
	bool equal = a.length() == b.length();

	for (std::string::size_type i = 0; equal and i < a.length(); ++i)
		equal = std::toupper(a[i]) == std::toupper(b[i]);

	return equal;
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
char32_t pop_last_char(std::string& s)
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
template<typename Iter>
char32_t get_first_char(Iter &ptr, Iter end)
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

/**
 * @brief Return a hexadecimal string representation for the numerical value in @a i
 * 
 * @param i The value to convert
 * @return std::string The hexadecimal representation
 */
std::string to_hex(uint32_t i)
{
	char s[sizeof(i) * 2 + 3];
	char* p = s + sizeof(s);
	*--p = 0;

	const char kHexChars[] = "0123456789abcdef";

	while (i)
	{
		*--p = kHexChars[i & 0x0F];
		i >>= 4;
	}

	*--p = 'x';
	*--p = '0';

	return p;
}

// --------------------------------------------------------------------

/// \brief A simple implementation of trim, removing white space from start and end of \a s
export void trim(std::string& s)
{
	std::string::iterator b = s.begin();
	while (b != s.end() and *b > 0 and std::isspace(*b))
		++b;
	
	std::string::iterator e = s.end();
	while (e > b and *(e - 1) > 0 and std::isspace(*(e - 1)))
		--e;
	
	if (b != s.begin() or e != s.end())
		s = { b, e };
}

// --------------------------------------------------------------------
/// \brief Simplistic implementation of contains

bool contains(std::string_view s, std::string_view p)
{
	return s.find(p) != std::string_view::npos;
}

// --------------------------------------------------------------------
/// \brief Simplistic implementation of split, with std:string in the vector
void split(std::vector<std::string>& v, std::string_view s, std::string_view p, bool compress = false)
{
	v.clear();

	std::string_view::size_type i = 0;
	const auto e = s.length();

	while (i <= e)
	{
		auto n = s.find(p, i);
		if (n > e)
			n = e;

		if (n > i or compress == false)
			v.emplace_back(s.substr(i, n - i));

		if (n == std::string_view::npos)
			break;
		
		i = n + p.length();
	}
}

// --------------------------------------------------------------------
/// \brief Simplistic to_lower function, works for one byte charsets only...

void to_lower(std::string& s, const std::locale& loc = std::locale())
{
	for (char& ch: s)
		ch = std::tolower(ch, loc);
}

// --------------------------------------------------------------------
/// \brief Simplistic join function

template<typename Container = std::vector<std::string> >
std::string join(const Container& v, std::string_view d)
{
	std::string result;

	if (not v.empty())
	{
		auto i = v.begin();
		for (;;)
		{
			result += *i++;

			if (i == v.end())
				break;

			result += d;
		}
	}
	return result;
}

// --------------------------------------------------------------------
/// \brief Simplistic replace_all

void replace_all(std::string& s, std::string_view p, std::string_view r)
{
	std::string::size_type i = 0;
	for (;;)
	{
		auto l = s.find(p, i);
		if (l == std::string::npos)
			break;
		
		s.replace(l, p.length(), r);
		i = l + r.length();
	}
}

} // namespace xml
