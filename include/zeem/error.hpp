// Copyright (c) 2024 Maarten L. Hekkelman
//
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#ifndef ZEEM_EXPORT
# error "Please include zeem.hpp only"
#endif

/**
 * \file
 * definition of the zeem::exception class
 */

#ifndef IN_MODULE_INTERFACE
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

	[[nodiscard]] const char *what() const noexcept override { return m_message.c_str(); }

  private:
	std::string m_message;
};

} // namespace zeem