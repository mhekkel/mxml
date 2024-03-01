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
/// definition of the mxml::xpath class, implementing a XPath 1.0 compatible search facility

#include <algorithm>
#include <charconv>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <sstream>

#include <cmath>

export module mxml:xpath;

import :node;

namespace mxml
{

// --------------------------------------------------------------------
/// XPath's can contain variables. And variables can contain all kinds of data
/// like strings, numbers and even node_sets. If you want to use variables,
/// you can define a context, add your variables to it and then pass it on
/// in the xpath::evaluate method.

export class context
{
  public:
	context();
	virtual ~context();

	void set(const std::string &name, const std::string &value);
	void set(const std::string &name, double value);

	template <typename T>
		requires std::is_same_v<T, std::string> or std::is_same_v<T, double>
	T get(const std::string &name);

  private:
	context(const context &) = delete;
	context &operator=(const context &) = delete;

	friend class xpath;

	struct context_imp *m_impl;
};

// --------------------------------------------------------------------
/// The actual xpath implementation. It expects an xpath in the constructor and
/// this path _must_ be UTF-8 encoded.

export class xpath
{
  public:
	xpath(std::string_view path);

	xpath(const xpath &rhs);
	xpath(xpath &&rhs);
	xpath &operator=(const xpath &);
	xpath &operator=(xpath &&);

	virtual ~xpath();

	/// evaluate returns a node_set. If you're only interested in mxml::element
	/// results, you should call the evaluate<element>() instantiation.
	template <typename T>
	std::list<T *> evaluate(const node &root) const
	{
		context ctxt;
		return evaluate<T>(root, ctxt);
	}

	/// The second evaluate method is used for xpaths that contain variables.
	template <typename T>
	std::list<T *> evaluate(const node &root, context &ctxt) const;

	/// Returns true if the \a n node matches the XPath
	bool matches(const node *n) const;

	/// debug routine, dumps the parse tree to stdout
	void dump();

  private:
	struct xpath_imp *m_impl;
};

} // namespace mxml
