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

// module;

/// \file
/// definition of the mxml::xpath class, implementing a XPath 1.0 compatible search facility

#include <algorithm>
#include <charconv>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>

#include <cmath>

// /* export */ module mxml:xpath;

// import :node;

#include "mxml/node.ixx"

namespace mxml
{

// --------------------------------------------------------------------
/// XPath's can contain variables. And variables can contain all kinds of data
/// like strings, numbers and even node_sets. If you want to use variables,
/// you can define a context, add your variables to it and then pass it on
/// in the xpath::evaluate method.

/* export */ class context final
{
  public:
	context();
	context(const context &ctxt)
		: m_impl(ctxt.m_impl)
	{
	}

	context(context &&ctxt)
	{
		std::swap(m_impl, ctxt.m_impl);
	}

	context &operator=(context ctxt)
	{
		std::swap(m_impl, ctxt.m_impl);
		return *this;
	}

	void set(const std::string &name, const std::string &value);
	void set(const std::string &name, double value);

	template <typename T>
		requires std::is_same_v<T, std::string> or std::is_same_v<T, double>
	T get(const std::string &name);

  private:
	friend class xpath;

	std::shared_ptr<struct context_imp> m_impl;
};

// --------------------------------------------------------------------
/// The actual xpath implementation. It expects an xpath in the constructor and
/// this path _must_ be UTF-8 encoded.

/* export */ class xpath final
{
  public:
	xpath(const std::string &path);

	xpath(const xpath &rhs)
		: m_impl(rhs.m_impl)
	{
	}

	xpath(xpath &&rhs) noexcept
	{
		std::swap(m_impl, rhs.m_impl);
	}

	xpath &operator=(xpath xp)
	{
		std::swap(m_impl, xp.m_impl);
		return *this;
	}

	/**
	 * @brief Evaluate an XPath and return a node_set. If you're only interested
	 * in mxml::element results, you should call the evaluate<element>()
	 * instantiation.
	 * Use @a ctxt to provide values for variables.
	 */

	template <typename T>
	std::vector<T *> evaluate(const node &root, const context &ctxt = {}) const;

	/**
	 * @brief Returns true if the \a n node matches the XPath
	 * Use @a ctxt to provide values for variables.
	 */
	bool matches(const node *n, const context &ctxt = {}) const;

  private:
	std::shared_ptr<class expression> m_impl;
};

} // namespace mxml
