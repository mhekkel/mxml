// Copyright (c) 2024-2026 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

/**
 * \file
 * definition of the zeem::exception class
 */

#ifndef ZEEM_CXX_MODULE
# include <exception>
# include <string>
#endif

namespace zeem
{

/// \brief base class of the exceptions thrown by zeem
ZEEM_EXPORT class exception : public std::exception
{
  public:
	/// \brief Create an exception with the message in \a message
	explicit exception(std::string message)
		: m_message(std::move(message))
	{
	}

	/// \brief Return the error message as a NUL-terminated string
	[[nodiscard]] const char *what() const noexcept override { return m_message.c_str(); }

  private:
	std::string m_message;
};

} // namespace zeem