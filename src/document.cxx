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
#include <algorithm>
#include <fstream>
#include <functional>
#include <iomanip>
#include <memory>
#include <sstream>
#include <tuple>

#include <cassert>

// module mxml;

// import :document;
// import :error;

#include "mxml.ixx"

namespace mxml
{

// --------------------------------------------------------------------

document::document()
	: m_validating(false)
	, m_preserve_cdata(false)
	, m_has_xml_decl(false)
	, m_encoding(encoding_type::UTF8)
	, m_version(1.0f)
	, m_standalone(false)
{
}

document::document(const document &doc)
	: m_doctype(doc.m_doctype)
	, m_validating(doc.m_validating)
	, m_preserve_cdata(doc.m_preserve_cdata)
	, m_has_xml_decl(doc.m_has_xml_decl)
	, m_encoding(doc.m_encoding)
	, m_version(doc.m_version)
	, m_standalone(doc.m_standalone)
	, m_fmt(doc.m_fmt)
{
}

document::document(std::string_view s)
	: document()
{
	struct membuf : public std::streambuf
	{
		membuf(char *text, size_t length)
		{
			this->setg(text, text, text + length);
		}
	} buffer(const_cast<char *>(s.data()), s.length());

	std::istream is(&buffer);
	parse(is);
}

document::document(std::istream &is)
	: document()
{
	parse(is);
}

document::document(std::istream &is, const std::string &base_dir)
	: document()
{
	m_validating = true;
	m_dtd_dir = base_dir;
	parse(is);
}

void swap(document &a, document &b) noexcept
{
	swap(static_cast<element_container&>(a), static_cast<element_container&>(b));

	std::swap(a.m_dtd_dir, b.m_dtd_dir);
	std::swap(a.m_doctype, b.m_doctype);
	std::swap(a.m_validating, b.m_validating);
	std::swap(a.m_validating_ns, b.m_validating_ns);
	std::swap(a.m_preserve_cdata, b.m_preserve_cdata);
	std::swap(a.m_has_xml_decl, b.m_has_xml_decl);
	std::swap(a.m_encoding, b.m_encoding);
	std::swap(a.m_version, b.m_version);
	std::swap(a.m_standalone, b.m_standalone);
	std::swap(a.m_wrap_prolog, b.m_wrap_prolog);
	std::swap(a.m_write_doctype, b.m_write_doctype);
	std::swap(a.m_write_xml_decl, b.m_write_xml_decl);
	std::swap(a.m_fmt, b.m_fmt);
	std::swap(a.m_cur, b.m_cur);
	std::swap(a.m_cdata, b.m_cdata);
	std::swap(a.m_namespaces, b.m_namespaces);
	std::swap(a.m_notations, b.m_notations);
	std::swap(a.m_root_size_at_first_notation, b.m_root_size_at_first_notation);
}

void document::set_base_dir(const std::string &path)
{
	m_dtd_dir = path;
}

encoding_type document::get_encoding() const
{
	return m_encoding;
}

void document::set_encoding(encoding_type enc)
{
	m_encoding = enc;
}

float document::get_version() const
{
	return m_version;
}

void document::set_version(float v)
{
	m_version = v;
}

// --------------------------------------------------------------------

// element::iterator document::insert_impl(const_iterator pos, node *n)
// {
// 	if (not empty())
// 		throw exception("Cannot add a node to a non-empty document");
// 	return element::insert_impl(pos, n);
// }

// --------------------------------------------------------------------

bool document::is_html5() const
{
	return m_doctype.m_root == "html" and
	       (not empty() and front().name() == "html") and
	       m_doctype.m_pubid == "" and
	       m_doctype.m_dtd == "about:legacy-compat";
}

// --------------------------------------------------------------------

bool document::operator==(const document &other) const
{
	return static_cast<const element_container&>(*this) == static_cast<const element_container&>(other);
}

std::ostream &operator<<(std::ostream &os, const document &doc)
{
	auto fmt = doc.m_fmt;

	if (os.width() > 0)
	{
		fmt.indent_width = os.width();
		fmt.indent = true;
		os.width(0);
	}

	doc.write(os, fmt);
	return os;
}

std::istream &operator>>(std::istream &is, document &doc)
{
	doc.parse(is);
	return is;
}

void document::write(std::ostream &os, format_info fmt) const
{
	if (m_version > 1.0f or m_write_xml_decl)
	{
		assert(m_encoding == encoding_type::UTF8);

		os << "<?xml version=\"" << std::fixed << std::setprecision(1) << m_version << "\"";

		// os << " encoding=\"UTF-8\"";

		if (m_standalone)
			os << " standalone=\"yes\"";

		os << "?>";

		if (m_wrap_prolog)
			os << '\n';
	}

	if (not m_notations.empty() or m_write_doctype)
	{
		os << "<!DOCTYPE " << (empty() ? "" : child()->get_qname());
		if (m_write_doctype and not m_doctype.m_dtd.empty())
		{
			if (m_doctype.m_pubid.empty())
				os << " SYSTEM \"";
			else
				os << " PUBLIC \"" << m_doctype.m_pubid << "\" \"";
			os << m_doctype.m_dtd << '"';
		}

		if (not m_notations.empty())
		{
			os << " [\n";

			for (auto &[name, sysid, pubid] : m_notations)
			{
				os << "<!NOTATION " << name;
				if (not pubid.empty())
				{
					os << " PUBLIC \'" << pubid << '\'';
					if (not sysid.empty())
						os << " \'" << sysid << '\'';
				}
				else
					os << " SYSTEM \'" << sysid << '\'';
				os << ">\n";
			}
			os << "]";
		}

		os << ">\n";
	}

	for (auto &n : nodes())
		n.write(os, fmt);
}

// --------------------------------------------------------------------

node *document::insert_impl(const node *p, node *n)
{
	if (child() != nullptr)
		throw exception("Only one child element is allowed in a document");
	return element_container::insert_impl(p, n);
}

// --------------------------------------------------------------------

void document::XmlDeclHandler(encoding_type /*encoding*/, bool standalone, float version)
{
	m_has_xml_decl = true;
	// m_encoding = encoding;
	m_standalone = standalone;
	m_version = version;

	m_fmt.version = version;
}

void document::StartElementHandler(const std::string &name, const std::string &uri, const parser::attr_list_type &atts)
{
	using namespace std::literals;

	std::string qname{ name };
	if (not uri.empty())
	{
		std::string prefix;
		bool found;

		auto i = std::find_if(m_namespaces.begin(), m_namespaces.end(),
			[uri](auto &ns)
			{ return ns.second == uri; });

		if (i != m_namespaces.end())
		{
			prefix = i->first;
			found = true;
		}
		else
			std::tie(prefix, found) = m_cur->prefix_for_namespace(uri);

		if (prefix.empty() and not found)
			throw exception("namespace not found: "s + std::string{ uri });

		if (not prefix.empty())
			qname = std::string{ prefix } + ':' + std::string{ name };
	}

	m_cur = static_cast<element *>(m_cur)->emplace_back(qname);

	for (const auto &[prefix, uri] : m_namespaces)
	{
		// assert(m_cur->type() == nodes_type::element);
		if (prefix.empty())
			static_cast<element *>(m_cur)->attributes().emplace("xmlns", uri);
		else
			static_cast<element *>(m_cur)->attributes().emplace("xmlns:"s + prefix, uri);
	}

	for (const parser::attr_type &a : atts)
	{
		qname = a.m_name;
		if (not a.m_ns.empty())
		{
			auto p = m_cur->prefix_for_namespace(a.m_ns);
			if (not p.second)
				throw exception("namespace not found: " + a.m_ns);

			qname = p.first.empty() ? a.m_name : p.first + ':' + a.m_name;
		}

		static_cast<element *>(m_cur)->attributes().emplace(qname, a.m_value, a.m_id);
	}

	m_namespaces.clear();
}

void document::EndElementHandler(const std::string & /*name*/, const std::string & /*name*/)
{
	if (m_cdata != nullptr)
		throw exception("CDATA section not closed");

#if 0 // This check is not needed since it can never happen anyway
	std::string qname = name;
	if (not uri.empty())
	{
		std::string prefix;
		bool found;

		auto i = std::find_if(m_namespaces.begin(), m_namespaces.end(),
			[uri](auto& ns) { return ns.second == uri; });

		if (i != m_namespaces.end())
		{
			prefix = i->first;
			found = true;
		}
		else
			std::tie(prefix, found) = m_cur->prefix_for_namespace(uri);
	
		if (prefix.empty() and not found)
			throw exception("namespace not found: " + uri);

		if (not prefix.empty())
			qname = prefix + ':' + name;
	}

	assert(m_cur->name() == qname);
#endif

	m_cur = m_cur->parent();
}

void document::CharacterDataHandler(const std::string &data)
{
	if (m_cdata != nullptr)
		m_cdata->append(data);
	else if (m_cur != this)
		static_cast<element *>(m_cur)->add_text(data);
}

void document::ProcessingInstructionHandler(const std::string &target, const std::string &data)
{
	if (m_cur == this)
		nodes().emplace_back(processing_instruction(target, data));
	else
		static_cast<element_container *>(m_cur)->nodes().emplace_back(processing_instruction(target, data));
}

void document::CommentHandler(const std::string &s)
{
	if (m_cur == this)
		nodes().emplace_back(comment(s));
	else
		static_cast<element_container *>(m_cur)->nodes().emplace_back(comment(s));
}

void document::StartCdataSectionHandler()
{
	m_cdata = static_cast<cdata *>((node *)static_cast<element_container *>(m_cur)->nodes().emplace_back(cdata()));
}

void document::EndCdataSectionHandler()
{
	m_cdata = nullptr;
}

void document::StartNamespaceDeclHandler(const std::string &prefix, const std::string &uri)
{
	m_namespaces.emplace_back(std::string{ prefix }, std::string{ uri });
}

void document::EndNamespaceDeclHandler(const std::string & /*prefix*/)
{
}

void document::DoctypeDeclHandler(const std::string &root, const std::string &publicId, const std::string &uri)
{
	m_doctype.m_root = root;
	m_doctype.m_pubid = publicId;
	m_doctype.m_dtd = uri;
}

void document::NotationDeclHandler(const std::string &name, const std::string &sysid, const std::string &pubid)
{
	if (m_notations.empty())
		m_root_size_at_first_notation = nodes().size();

	notation n = { std::string{ name }, std::string{ sysid }, std::string{ pubid } };

	auto i = find_if(m_notations.begin(), m_notations.end(),
		[name](auto &nt)
		{ return nt.m_name >= name; });

	m_notations.insert(i, n);
}

std::istream *document::external_entity_ref(const std::string &base, const std::string &pubid, const std::string &sysid)
{
	std::istream *result = nullptr;

	if (m_external_entity_ref_loader)
		result = m_external_entity_ref_loader(base, pubid, sysid);

	if (result == nullptr and not sysid.empty())
	{
		std::string path;

		if (base.empty())
			path = sysid;
		else
			path = std::string{ base } + '/' + std::string{ sysid };

		std::unique_ptr<std::ifstream> file(new std::ifstream(path, std::ios::binary));
		if (not file->is_open())
			file->open(m_dtd_dir + '/' + path, std::ios::binary);

		if (file->is_open())
			result = file.release();
	}

	return result;
}

void document::parse(std::istream &is)
{
	parser p(is);

	using namespace std::placeholders;

	p.xml_decl_handler = std::bind(&document::XmlDeclHandler, this, _1, _2, _3);
	p.doctype_decl_handler = std::bind(&document::DoctypeDeclHandler, this, _1, _2, _3);
	p.start_element_handler = std::bind(&document::StartElementHandler, this, _1, _2, _3);
	p.end_element_handler = std::bind(&document::EndElementHandler, this, _1, _2);
	p.character_data_handler = std::bind(&document::CharacterDataHandler, this, _1);
	if (m_preserve_cdata)
	{
		p.start_cdata_section_handler = std::bind(&document::StartCdataSectionHandler, this);
		p.end_cdata_section_handler = std::bind(&document::EndCdataSectionHandler, this);
	}
	p.start_namespace_decl_handler = std::bind(&document::StartNamespaceDeclHandler, this, _1, _2);
	p.processing_instruction_handler = std::bind(&document::ProcessingInstructionHandler, this, _1, _2);
	p.comment_handler = std::bind(&document::CommentHandler, this, _1);
	p.notation_decl_handler = std::bind(&document::NotationDeclHandler, this, _1, _2, _3);
	p.external_entity_ref_handler = std::bind(&document::external_entity_ref, this, _1, _2, _3);

	m_cur = this;

	p.parse(m_validating, m_validating_ns);

	assert(m_cur == this);
}

std::string document::str() const
{
	if (child())
		return child()->str();
	else
		return {};
}

namespace literals
{

	document operator""_xml(const char *text, size_t length)
	{
		mxml::document doc;
		doc.set_preserve_cdata(true);

		struct membuf : public std::streambuf
		{
			membuf(char *text, size_t length)
			{
				this->setg(text, text, text + length);
			}
		} buffer(const_cast<char *>(text), length);

		std::istream is(&buffer);
		is >> doc;

		return doc;
	}

} // namespace literals

} // namespace mxml
