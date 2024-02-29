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

#include <iostream>
#include <map>
#include <set>
#include <stack>
#include <string>
#include <tuple>
#include <utility>

#include <cassert>

// export module mxml:node;

module mxml;

import :error;
import :node;
import :text;

namespace mxml
{

const std::set<std::string> kEmptyHTMLElements{
	"area", "base", "br", "col", "embed", "hr", "img", "input", "keygen", "link", "meta", "param", "source", "track", "wbr"
};

// --------------------------------------------------------------------

void write_string(std::ostream &os, const std::string &s, bool escape_whitespace, bool escape_quot, bool trim, float version)
{
	bool last_is_space = false;

	auto sp = s.begin();
	auto se = s.end();

	while (sp < se)
	{
		auto sb = sp;

		char32_t c = get_first_char(sp, se);

		switch (c)
		{
			case '&':
				os << "&amp;";
				last_is_space = false;
				break;
			case '<':
				os << "&lt;";
				last_is_space = false;
				break;
			case '>':
				os << "&gt;";
				last_is_space = false;
				break;
			case '\"':
				if (escape_quot)
					os << "&quot;";
				else
					os << static_cast<char>(c);
				last_is_space = false;
				break;
			case '\n':
				if (escape_whitespace)
					os << "&#10;";
				else
					os << static_cast<char>(c);
				last_is_space = true;
				break;
			case '\r':
				if (escape_whitespace)
					os << "&#13;";
				else
					os << static_cast<char>(c);
				last_is_space = false;
				break;
			case '\t':
				if (escape_whitespace)
					os << "&#9;";
				else
					os << static_cast<char>(c);
				last_is_space = false;
				break;
			case ' ':
				if (not trim or not last_is_space)
					os << ' ';
				last_is_space = true;
				break;
			case 0: throw exception("Invalid null character in XML content");
			default:
				if (c >= 0x0A0 or (version == 1.0 ? is_valid_xml_1_0_char(c) : is_valid_xml_1_1_char(c)))
					for (auto ci = sb; ci < sp; ++ci)
						os << *ci;
				else
					os << "&#" << static_cast<int>(c) << ';';
				last_is_space = false;
				break;
		}

		sb = sp;
	}
}

// --------------------------------------------------------------------

node::~node()
{
}

node *node::root()
{
	node *result = nullptr;
	if (m_parent != nullptr)
		result = m_parent->root();
	return result;
}

const node *node::root() const
{
	const node *result = nullptr;
	if (m_parent != nullptr)
		result = m_parent->root();
	return result;
}

bool node::equals(const node *n) const
{
	assert(false);
	return n == this;
}

std::string node::lang() const
{
	std::string result;
	if (m_parent != nullptr)
		result = m_parent->lang();
	return result;
}

std::string node::get_qname() const
{
	return "";
}

std::string node::name() const
{
	std::string qn = get_qname();
	std::string::size_type s = qn.find(':');
	if (s != std::string::npos)
		qn.erase(0, s + 1);
	return qn;
}

std::string node::get_prefix() const
{
	std::string qn = get_qname();
	std::string::size_type s = qn.find(':');

	std::string p;

	if (s != std::string::npos)
		p = qn.substr(0, s);

	return p;
}

std::string node::get_ns() const
{
	std::string p = get_prefix();
	return namespace_for_prefix(p);
}

std::string node::namespace_for_prefix(const std::string &prefix) const
{
	std::string result;
	if (m_parent != nullptr)
		result = m_parent->namespace_for_prefix(prefix);
	return result;
}

std::pair<std::string, bool> node::prefix_for_namespace(const std::string &uri) const
{
	std::pair<std::string, bool> result{};
	if (m_parent != nullptr)
		result = m_parent->prefix_for_namespace(uri);
	return result;
}

std::string node::prefix_tag(std::string tag, const std::string &uri) const
{
	auto prefix = prefix_for_namespace(uri);
	return prefix.second ? prefix.first + ':' + tag : tag;
}

// --------------------------------------------------------------------

void basic_node_list::clear()
{
	// avoid deep recursion and stack overflows

	std::stack<basic_node_list *> stack;
	std::stack<element *> s2;

	stack.push(this);

	while (not stack.empty())
	{
		auto nl = stack.top();
		stack.pop();

		for (auto n = nl->m_node->m_next; n != nl->m_node; n = n->m_next)
		{
			if (n->type() != node_type::element)
				continue;
			
			auto e = static_cast<element *>(n);
			s2.push(e);
			stack.push(e);
		}
	}

	while (not s2.empty())
	{
		auto e = s2.top();
		s2.pop();

		static_cast<element *>(e->parent())->erase(e);
	}

	// And now clean up what remains
	auto n = m_node->m_next;

	while (n != m_node)
	{
		auto t = n->m_next;
		delete n;
		n = t;
		assert(n != nullptr);
	}

	m_node->m_next = m_node->m_prev = m_node;
}

// --------------------------------------------------------------------
// comment

void comment::write(std::ostream &os, format_info fmt) const
{
	// if (fmt.indent_width != 0)
	// 	os << '\n' << std::string(fmt.indent_width, ' ');

	if (not fmt.suppress_comments)
	{
		os << "<!--";

		bool lastWasHyphen = false;
		for (char ch : m_text)
		{
			if (ch == '-' and lastWasHyphen)
				os << ' ';

			os << ch;
			lastWasHyphen = ch == '-';

			// if (ch == '\n')
			// {
			// 	for (size_t i = 0; i < indent; ++i)
			// 		os << ' ';
			// }
		}

		os << "-->";

		if (fmt.indent_width != 0)
			os << '\n';
	}
}

// --------------------------------------------------------------------
// processing_instruction

void processing_instruction::write(std::ostream &os, format_info fmt) const
{
	if (fmt.indent)
		os << '\n'
		   << std::string(fmt.indent_level * fmt.indent_width, ' ');

	os << "<?" << m_target << ' ' << m_text << "?>";

	if (fmt.indent != 0)
		os << '\n';
}

// --------------------------------------------------------------------
// text

bool text::equals(const node *n) const
{
	bool result = false;
	auto t = dynamic_cast<const text *>(n);

	if (t != nullptr)
	{
		std::string text = m_text;
		trim(text);

		std::string ttext = t->m_text;
		trim(ttext);

		result = text == ttext;
	}

	return result;
}

bool text::is_space() const
{
	bool result = true;
	for (auto ch : m_text)
	{
		if (not(ch == ' ' or ch == '\t' or ch == '\n' or ch == '\r'))
		{
			result = false;
			break;
		}
	}
	return result;
}

void text::write(std::ostream &os, format_info fmt) const
{
	write_string(os, m_text, fmt.escape_white_space, fmt.escape_double_quote, false, fmt.version);
}

// --------------------------------------------------------------------
// cdata

void cdata::write(std::ostream &os, format_info fmt) const
{
	if (fmt.indent)
		os << '\n'
		   << std::string(fmt.indent_level * fmt.indent_width, ' ');

	os << "<![CDATA[" << m_text << "]]>";

	if (fmt.indent)
		os << '\n';
}

// --------------------------------------------------------------------
// attribute

std::string attribute::uri() const
{
	assert(is_namespace());
	if (not is_namespace())
		throw exception("Attribute is not a namespace");
	return m_value;
}

void attribute::write(std::ostream &os, format_info fmt) const
{
	if (fmt.indent_width != 0)
		os << '\n'
		   << std::string(fmt.indent_width, ' ');
	else
		os << ' ';

	os << m_qname << "=\"";

	write_string(os, m_value, fmt.escape_white_space, true, false, fmt.version);

	os << '"';
}

// --------------------------------------------------------------------
// element

std::string element::lang() const
{
	std::string result;

	auto i = m_attributes.find("xml:lang");
	if (i != m_attributes.end())
		result = i->value();
	else if (m_parent != nullptr)
		result = m_parent->lang();

	return result;
}

std::string element::id() const
{
	std::string result;

	for (auto &a : m_attributes)
	{
		if (a.is_id())
		{
			result = a.value();
			break;
		}
	}

	return result;
}

std::string element::get_attribute(const std::string &qname) const
{
	std::string result;

	auto a = m_attributes.find(qname);
	if (a != m_attributes.end())
		result = a->value();

	return result;
}

void element::set_attribute(const std::string &qname, const std::string &value)
{
	m_attributes.emplace(qname, value);
}

bool element::equals(const node *n) const
{
	bool result = false;
	const element *e = dynamic_cast<const element *>(n);

	if (e != nullptr)
	{
		result = name() == e->name() and get_ns() == e->get_ns();

		auto &na = m_nodes;
		auto &nb = e->m_nodes;

		auto a = na.begin();
		auto b = nb.begin();

		while (a != na.end() or b != nb.end())
		{
			if (a->equals(b))
			{
				++a;
				++b;
				continue;
			}

			const text *t = dynamic_cast<const text *>((const node *)a);

			if (t != nullptr and t->is_space())
			{
				++a;
				continue;
			}

			t = dynamic_cast<const text *>((const node *)b);
			if (t != nullptr and t->is_space())
			{
				++b;
				continue;
			}

			result = false;
			break;
		}

		result = result and a == na.end() and b == nb.end();

		if (result)
		{
			result = m_attributes == e->m_attributes;
			if (not result)
			{
				std::set<attribute> as(m_attributes.begin(), m_attributes.end());
				std::set<attribute> bs(e->m_attributes.begin(), e->m_attributes.end());

				std::set<std::string> nsa, nsb;

				auto ai = as.begin(), bi = bs.begin();
				for (;;)
				{
					if (ai == as.end() and bi == bs.end())
						break;

					if (ai != as.end() and ai->is_namespace())
					{
						nsa.insert(ai->value());
						++ai;
						continue;
					}

					if (bi != bs.end() and bi->is_namespace())
					{
						nsb.insert(bi->value());
						++bi;
						continue;
					}

					if (ai == as.end() or bi == bs.end() or *ai++ != *bi++)
					{
						result = false;
						break;
					}
				}

				result = ai == as.end() and bi == bs.end() and nsa == nsb;
			}
		}
	}

	return result;
}

// void element::clear()
// {
// 	m_nodes.clear();
// 	m_attributes.clear();
// }

std::string element::get_content() const
{
	std::string result;

	for (auto &n : nodes())
	{
		auto t = dynamic_cast<const text *>(&n);
		if (t != nullptr)
			result += t->get_text();
	}

	return result;
}

void element::set_content(const std::string &s)
{
	// remove all existing text nodes (including cdata ones)
	auto nn = nodes();
	for (auto n = nn.begin(); n != nn.end(); ++n)
	{
		if (auto &t = *n; (typeid(t) == typeid(text)) or (typeid(t) == typeid(cdata)))
			n = nn.erase(n);
	}

	// and add a new text node with the content
	nn.emplace_back(text(s));
}

std::string element::str() const
{
	std::string result;

	for (auto &n : nodes())
		result += n.str();

	return result;
}

void element::add_text(const std::string &s)
{
	auto nn = nodes();

	if (auto &t = nn.back(); typeid(t) == typeid(text))
		static_cast<text &>(nn.back()).append(s);
	else
		nn.emplace_back(text(s));
}

void element::set_text(const std::string &s)
{
	set_content(s);
}

void element::flatten_text()
{
	auto nn = nodes();
	auto n = nn.begin();
	while (n != nn.end())
	{
		auto tn = dynamic_cast<text *>(&*n);
		if (tn == nullptr)
		{
			n = n->m_next;
			continue;
		}

		auto ntn = dynamic_cast<text *>(n->m_next);
		if (ntn == nullptr)
		{
			n = n->m_next;
			continue;
		}

		tn->append(ntn->get_text());
		nn.erase(n->m_next);
	}
}

void element::write(std::ostream &os, format_info fmt) const
{
	// if width is set, we wrap and indent the file
	size_t indentation = fmt.indent_level * fmt.indent_width;

	if (fmt.indent)
	{
		if (fmt.indent_level > 0)
			os << '\n';
		if (indentation > 0)
			os << std::string(indentation, ' ');
	}

	os << '<' << m_qname;

	// if the left flag is set, wrap and indent attributes as well
	auto attr_fmt = fmt;
	attr_fmt.indent_width = 0;

	for (auto &attr : m_attributes)
	{
		attr.write(os, attr_fmt);
		if (attr_fmt.indent_width == 0 and fmt.indent_attributes)
			attr_fmt.indent_width = indentation + 1 + m_qname.length() + 1;
	}

	if ((fmt.html and kEmptyHTMLElements.count(m_qname)) or
		(not fmt.html and fmt.collapse_tags and nodes().empty()))
		os << "/>";
	else
	{
		os << '>';
		auto sub_fmt = fmt;
		++sub_fmt.indent_level;

		bool wrote_element = false;
		for (auto &n : nodes())
		{
			n.write(os, sub_fmt);
			wrote_element = dynamic_cast<const element *>(&n) != nullptr;
		}

		if (wrote_element and fmt.indent != 0)
			os << '\n'
			   << std::string(indentation, ' ');

		os << "</" << m_qname << '>';
	}
}

std::ostream &operator<<(std::ostream &os, const element &e)
{
	auto flags = os.flags({});
	auto width = os.width(0);

	format_info fmt;
	fmt.indent = width > 0;
	fmt.indent_width = width;
	fmt.indent_attributes = flags & std::ios_base::left;

	e.write(os, fmt);

	return os;
}

std::string element::namespace_for_prefix(const std::string &prefix) const
{
	std::string result;

	for (auto &a : m_attributes)
	{
		if (not a.is_namespace())
			continue;

		if (a.name() == "xmlns")
		{
			if (prefix.empty())
			{
				result = a.value();
				break;
			}
			continue;
		}

		if (a.name() == prefix)
		{
			result = a.value();
			break;
		}
	}

	if (result.empty() and dynamic_cast<element *>(m_parent) != nullptr)
		result = static_cast<element *>(m_parent)->namespace_for_prefix(prefix);

	return result;
}

std::pair<std::string, bool> element::prefix_for_namespace(const std::string &uri) const
{
	std::string result;
	bool found = false;

	for (auto &a : m_attributes)
	{
		if (not a.is_namespace())
			continue;

		if (a.value() == uri)
		{
			found = true;
			if (a.get_qname().length() > 6)
				result = a.get_qname().substr(6);
			break;
		}
	}

	if (not found and dynamic_cast<element *>(m_parent) != nullptr)
		std::tie(result, found) = static_cast<element *>(m_parent)->prefix_for_namespace(uri);

	return make_pair(result, found);
}

void element::move_to_name_space(const std::string &prefix, const std::string &uri,
	bool recursive, bool including_attributes)
{
	// first some sanity checks
	auto p = prefix_for_namespace(uri);
	if (p.second)
	{
		if (p.first != prefix)
			throw exception("Invalid prefix in move_to_name_space, already known as '" + p.first + "'");
	}
	else
	{
		bool set = false;
		for (auto &a : m_attributes)
		{
			if (not a.is_namespace())
				continue;

			if (a.get_qname().length() > 6 and a.get_qname().substr(6) == prefix)
			{
				set = true;
				a.set_value(uri);
				break;
			}
		}

		if (not set)
			m_attributes.emplace(prefix.empty() ? "xmlns" : "xmlns:" + std::string{ prefix }, uri);
	}

	set_qname(prefix, name());

	if (including_attributes)
	{
		// first process the namespace attributes...
		for (auto &attr : m_attributes)
		{
			if (not attr.is_namespace())
				continue;

			auto nsp = prefix_for_namespace(attr.uri());
			if (not nsp.second)
				attr.set_qname("xmlns", nsp.first);
		}

		// ... and then the others, makes sure the namespaces are known
		for (auto &attr : m_attributes)
		{
			if (attr.is_namespace())
				continue;

			auto ns = attr.get_ns();

			if (ns.empty())
				attr.set_qname(prefix, attr.name());
			else
			{
				auto nsp = prefix_for_namespace(ns);
				if (not nsp.second)
					throw exception("Cannot move element to new namespace, namespace not found: " + ns);
				attr.set_qname(nsp.first, attr.name());
			}
		}
	}

	if (recursive)
	{
		for (element &e : *this)
			e.move_to_name_space(prefix, uri, true, including_attributes);
	}
}

element_set element::find(std::string_view path) const
{
	return xpath(path).evaluate<element>(*this);
}

element *element::find_first(std::string_view path)
{
	element_set s = xpath(path).evaluate<element>(*this);

	return s.empty() ? nullptr : s.front();
}

// --------------------------------------------------------------------

void fix_namespaces(element &e, element &source, element &dest)
{
	std::stack<node *> s;

	s.push(&e);

	std::map<std::string, std::string> mapped;

	while (not s.empty())
	{
		auto n = s.top();
		s.pop();

		auto p = n->get_prefix();
		if (not p.empty())
		{
			if (mapped.count(p))
			{
				if (mapped[p] != p)
					n->set_qname(mapped[p], n->name());
			}
			else
			{
				auto ns = n->namespace_for_prefix(p);
				if (ns.empty())
					ns = source.namespace_for_prefix(p);

				auto dp = dest.prefix_for_namespace(ns);
				if (dp.second)
				{
					mapped[p] = dp.first;
					n->set_qname(dp.first, n->name());
				}
				else
				{
					mapped[p] = p;
					dest.attributes().emplace({ "xmlns:" + p, ns });
				}
			}
		}

		auto el = dynamic_cast<element *>(n);
		if (el == nullptr)
			continue;

		for (auto &c : *el)
			s.push(&c);

		for (auto &a : el->attributes())
			s.push(&a);
	}
}

} // namespace mxml