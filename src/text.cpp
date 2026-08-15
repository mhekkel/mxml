// Copyright (c) 2024-2026 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

#ifndef ZEEM_CXX_MODULE
# include "zeem/zeem.hpp"

# include <string>
# include <string_view>
#endif

namespace zeem
{

// some very basic code to check the class of scanned characters

bool is_name_start_char(char32_t uc)
{
	return uc == ':' or
	       (uc >= 'A' and uc <= 'Z') or
	       uc == '_' or
	       (uc >= 'a' and uc <= 'z') or
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

bool is_space(char32_t uc)
{
	return uc == ' ' or
	       uc == '\f' or
	       uc == '\n' or
	       uc == '\r' or
	       uc == '\t' or
	       uc == '\v';
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

bool is_valid_system_literal(std::string_view s)
{
	bool result = true;
	for (auto ch : s)
	{
		if (not is_valid_system_literal_char(ch))
		{
			result = false;
			break;
		}
	}
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

bool is_valid_public_id(std::string_view s)
{
	bool result = true;
	for (auto ch : s)
	{
		if (not is_valid_public_id_char(ch))
		{
			result = false;
			break;
		}
	}
	return result;
}

/// \brief Append a single unicode character to an utf-8 string
void append(std::string &s, char32_t uc)
{
	if (uc < 0x080)
		s += (static_cast<char>(uc));
	else if (uc < 0x0800)
	{
		s.insert(s.end(), { static_cast<char>(0x0c0U | (uc >> 6U)),
							  static_cast<char>(0x080U | (uc & 0x3fU)) });
	}
	else if (uc < 0x00010000U)
	{
		s.insert(s.end(), { static_cast<char>(0x0e0U | (uc >> 12U)),
							  static_cast<char>(0x080U | ((uc >> 6U) & 0x3fU)),
							  static_cast<char>(0x080U | (uc & 0x3fU)) });
	}
	else if (uc < 0x00110000)
	{
		s.insert(s.end(), { static_cast<char>(0x0f0U | (uc >> 18U)),
							  static_cast<char>(0x080U | ((uc >> 12U) & 0x3fU)),
							  static_cast<char>(0x080U | ((uc >> 6U) & 0x3fU)),
							  static_cast<char>(0x080U | (uc & 0x3fU)) });
	}
}

/// \brief remove the last unicode character from an utf-8 string
char32_t pop_back_char(std::string &s)
{
	char32_t result = 0;

	if (not s.empty())
	{
		std::string::iterator ch = s.end() - 1;

		if ((*ch & 0x0080U) == 0)
		{
			result = *ch;
			s.erase(ch);
		}
		else
		{
			int o = 0;

			do
			{
				result |= (*ch & 0x03FU) << o;
				o += 6;
				--ch;
			} while (ch != s.begin() and (*ch & 0x0C0U) == 0x080U);

			switch (o)
			{
				case 6: result |= (*ch & 0x01FU) << 6; break;
				case 12: result |= (*ch & 0x00FU) << 12; break;
				case 18: result |= (*ch & 0x007U) << 18; break;
				default: break;
			}

			s.erase(ch, s.end());
		}
	}

	return result;
}

/// \brief return the first unicode and the advanced pointer from a string
char32_t pop_front_char(std::string_view::const_iterator &ptr, std::string_view::const_iterator end)
{
	auto get_char = [&]
	{
		if (ptr == end)
			throw zeem::exception("invalid utf-8 character, truncated?");
		return static_cast<unsigned char>(*ptr++);
	};

	char32_t result = get_char();

	if (result & 0x080)
	{
		char8_t ch[3];

		if ((result & 0x0E0) == 0x0C0)
		{
			ch[0] = get_char();
			if ((ch[0] & 0x0c0) != 0x080)
				throw zeem::exception("Invalid utf-8");
			result = ((result & 0x01F) << 6) | (ch[0] & 0x03F);

			if (result < 0x0080)
				throw zeem::exception("invalid utf-8 character (overlong)");
		}
		else if ((result & 0x0F0) == 0x0E0)
		{
			ch[0] = get_char();
			ch[1] = get_char();
			if ((ch[0] & 0x0c0) != 0x080 or (ch[1] & 0x0c0) != 0x080)
				throw zeem::exception("Invalid utf-8");
			result = ((result & 0x00F) << 12) | ((ch[0] & 0x03F) << 6) | (ch[1] & 0x03F);

			if (result < 0x0800)
				throw zeem::exception("invalid utf-8 character (overlong)");
		}
		else if ((result & 0x0F8) == 0x0F0)
		{
			ch[0] = get_char();
			ch[1] = get_char();
			ch[2] = get_char();
			if ((ch[0] & 0x0c0) != 0x080 or (ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080)
				throw zeem::exception("Invalid utf-8");
			result = ((result & 0x007) << 18) | ((ch[0] & 0x03F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F);

			if (result < 0x010000)
				throw zeem::exception("invalid utf-8 character (overlong)");

			if (result > 0x10ffff)
				throw zeem::exception("invalid utf-8 character (out of range)");
		}
		else
		{
			throw zeem::exception("invalid utf-8 start byte");
		}
	}

	if (result >= 0x0D800 and result <= 0x0DFFF)
		throw zeem::exception("invalid utf-8 character, surrogate");

	return result;
}

char32_t pop_front_char(std::string::const_iterator &ptr, std::string::const_iterator end)
{
	std::string_view sv(ptr, end);

	auto sv_ptr = sv.begin();
	auto result = pop_front_char(sv_ptr, sv.end());

	ptr += sv_ptr - sv.begin();
	return result;
}

// --------------------------------------------------------------------

/// \brief A simple implementation of trim, removing white space from start and end of \a s
void trim(std::string &s)
{
	auto in = s.begin(), out = s.begin(), end = s.end();

	while (end != s.begin() and is_space(*(end - 1)))
		--end;

	while (in != end and is_space(*in))
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

} // namespace zeem
