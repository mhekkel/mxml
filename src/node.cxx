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

export module mxml:node;

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

void write_string(std::ostream &os, const std::string &s, bool escape_whitespace, bool escape_quot, bool trim, version_type version)
{
	bool last_is_space = false;

	auto sp = s.cbegin();
	auto se = s.cend();

	while (sp < se)
	{
		auto sb = sp;

		char32_t c = pop_front_char(sp, se);

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
				if (c >= 0x0A0 or (version == version_type{ 1, 0 } ? is_valid_xml_1_0_char(c) : is_valid_xml_1_1_char(c)))
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

bool basic_node_list::operator==(const basic_node_list &b) const
{
	bool result = true;
	auto na = m_header->m_next, nb = b.m_header->m_next;
	for (; result and na != m_header and nb != b.m_header; na = na->m_next, nb = nb->m_next)
		result = na->equals(nb);
	return result and na == m_header and nb == b.m_header;
}

void basic_node_list::clear()
{
	// avoid deep recursion and stack overflows

	std::stack<basic_node_list *> stack;

	stack.push(this);

	while (not stack.empty())
	{
		auto nl = stack.top();

		for (auto n = nl->m_header->m_next; n != nl->m_header; n = n->m_next)
		{
			if (n->type() != node_type::element)
				continue;

			auto e = static_cast<element_container *>(n);
			if (e->empty())
				continue;

			stack.push(e);
			break;
		}

		if (stack.top() != nl)
			continue;

		// nothing was added, we can safely delete nl
		stack.pop();

		for (auto n = nl->m_header->m_next; n != nl->m_header;)
		{
			auto t = n->m_next;
			assert(n->type() != node_type::header);
			delete n;
			n = t;
		}

		nl->m_header->m_next = nl->m_header->m_prev = nl->m_header;
	}
}

node *basic_node_list::insert_impl(const node *p, node *n)
{
	assert(n != nullptr);
	assert(n->next() == n);
	assert(n->prev() == n);

	if (n == nullptr)
		throw exception("Invalid pointer passed to insert");

	if (n->parent() != nullptr or n->next() != n or n->prev() != n)
		throw exception("attempt to add a node that already has a parent or siblings");

	n->parent(m_header->m_parent);

	n->prev(p->prev());
	n->prev()->next(n);
	n->next(p);
	n->next()->prev(n);

	return n;
}

node *basic_node_list::erase_impl(node *n)
{
	if (n == m_header)
		return n;

	if (n->m_parent != m_header->m_parent)
		throw exception("attempt to remove node whose parent is invalid");

	node *result = n->next();

	n->next()->prev(n->prev());
	n->prev()->next(n->next());

	n->next(nullptr);
	n->prev(nullptr);
	n->parent(nullptr);
	delete n;

	return result;
}

// --------------------------------------------------------------------
// comment

void comment::write(std::ostream &os, format_info fmt) const
{
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

	if (n->type() == node_type::text)
	{
		auto t = static_cast<const text *>(n);

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
// element_container

std::string element_container::str() const
{
	std::string result;

	for (auto &n : nodes())
		result += n.str();

	return result;
}

void element_container::write(std::ostream &os, format_info fmt) const
{
}

element_set element_container::find(const std::string &path) const
{
	return xpath(path).evaluate<element>(*this);
}

element_container::iterator element_container::find_first(const std::string &path)
{
	element_set s = xpath(path).evaluate<element>(*this);

	return s.empty() ? end() : iterator(s.front());
}

element_container::const_iterator element_container::find_first(const std::string &path) const
{
	return const_cast<element_container *>(this)->find_first(path);
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

std::string element::get_attribute(std::string_view qname) const
{
	std::string result;

	auto a = m_attributes.find(qname);
	if (a != m_attributes.end())
		result = a->value();

	return result;
}

void element::set_attribute(std::string_view qname, std::string_view value)
{
	m_attributes.emplace(qname, value);
}

bool element::equals(const node *n) const
{
	bool result = false;

	if (type() == n->type())
	{
		const element *e = static_cast<const element *>(n);

		result = name() == e->name() and get_ns() == e->get_ns();

		auto na = nodes();
		auto nb = e->nodes();

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

			if (a->type() == node_type::text)
			{
				auto t = static_cast<const text *>(&*a);
				if (t->is_space())
				{
					++a;
					continue;
				}
			}

			if (b->type() == node_type::text)
			{
				auto t = static_cast<const text *>(&*b);
				if (t->is_space())
				{
					++b;
					continue;
				}
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

std::string element::get_content() const
{
	std::string result;

	for (auto &n : nodes())
	{
		if (n.type() == node_type::text or n.type() == node_type::cdata)
			result += static_cast<const node_with_text &>(n).get_text();
	}

	return result;
}

void element::set_content(const std::string &s)
{
	// remove all existing text nodes (including cdata ones)
	auto nn = nodes();
	for (auto n = nn.begin(); n != nn.end(); ++n)
	{
		if (n->type() == node_type::text or n->type() == node_type::cdata)
			n = nn.erase(n);
	}

	// and add a new text node with the content
	nn.emplace_back(text(s));
}

void element::add_text(const std::string &s)
{
	auto nn = nodes();

	if (nn.back().type() == node_type::text)
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
		if (n->type() != node_type::text or n->m_next->type() != node_type::text)
		{
			n = n->m_next;
			continue;
		}

		auto tn = static_cast<text *>(&*n);
		auto ntn = static_cast<text *>(n->m_next);

		tn->append(ntn->get_text());
		nn.erase(n->m_next);
	}
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

	if (result.empty() and m_parent != nullptr)
		result = m_parent->namespace_for_prefix(prefix);

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

	if (not found and m_parent != nullptr)
		std::tie(result, found) = m_parent->prefix_for_namespace(uri);

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
			wrote_element = n.type() == node_type::element;
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

		if (n->type() != node_type::element)
			continue;

		auto el = static_cast<element *>(n);
		for (auto &c : *el)
			s.push(&c);

		for (auto &a : el->attributes())
			s.push(&a);
	}
}

} // namespace mxml