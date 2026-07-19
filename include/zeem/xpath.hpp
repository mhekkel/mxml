// Copyright (c) 2024 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

/**
 * \file
 * definition of the zeem::xpath class, implementing a XPath 1.0 compatible search facility
 */

#ifndef ZEEM_CXX_MODULE
# include <memory>
# include <string>
# include <string_view>
# include <type_traits>
# include <vector>
#endif

namespace zeem
{

class node;

// --------------------------------------------------------------------
/// XPath's can contain variables. And variables can contain all kinds of data
/// like strings, numbers and even node_sets. If you want to use variables,
/// you can define a context, add your variables to it and then pass it on
/// in the xpath::evaluate method.

/**
 * @brief The context class, containing named variables to use in XPaths
 *
 */

ZEEM_EXPORT class context final
{
  public:
	/// @brief constructor
	context();

	/// @brief Constructor to create a new scope
	context(const context &ctxt) = default;

	/// @brief move constructor
	context(context &&ctxt) noexcept
	{
		std::swap(m_impl, ctxt.m_impl);
	}

	/// @brief assignment operator
	context &operator=(context ctxt)
	{
		std::swap(m_impl, ctxt.m_impl);
		return *this;
	}

	/// @brief Store a new variable in this context with name \a name and value \a value
	void set(const std::string &name, std::string value);

	/// @brief Store a new variable in this context with name \a name and value \a value
	void set(const std::string &name, double value);

	/// @brief Get a variable stored in this context or further up the scopes
	template <typename T>
		requires std::is_same_v<T, std::string> or std::is_same_v<T, double>
	T get(std::string name);

	/** @cond */
  private:
	friend class xpath;

	std::shared_ptr<struct context_imp> m_impl;
	/** @endcond */
};

// --------------------------------------------------------------------
/// The actual xpath implementation. It expects an xpath in the constructor and
/// this path _must_ be UTF-8 encoded.

/**
 * @brief Class encapsulating an XPath
 *
 */

ZEEM_EXPORT class xpath final
{
  public:
	/// @brief constructor taking a UTF-8 encoded xpath in \a path
	explicit xpath(std::string_view path);

	/// @brief copy constructor
	xpath(const xpath &rhs) = default;

	/// @brief move constructor
	xpath(xpath &&rhs) noexcept
	{
		std::swap(m_impl, rhs.m_impl);
	}

	/// @brief assignment operator
	xpath &operator=(xpath xp)
	{
		std::swap(m_impl, xp.m_impl);
		return *this;
	}

	/**
	 * @brief Evaluate an XPath and return a node_set. If you're only interested
	 * in zeem::element results, you should call the evaluate<element>()
	 * instantiation.
	 * Use @a ctxt to provide values for variables.
	 */

	template <typename T>
	[[nodiscard]] std::vector<T *> evaluate(const node &root, const context &ctxt = {}) const;

	/**
	 * @brief Returns true if the \a n node matches the XPath
	 * Use @a ctxt to provide values for variables.
	 */
	bool matches(const node *n, const context &ctxt = {}) const;

  private:
	std::shared_ptr<class expression> m_impl;
};

} // namespace zeem
