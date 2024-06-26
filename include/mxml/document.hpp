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

#pragma once

/**
 * \file
 * definition of the mxml::document class
 */

#include "mxml/node.hpp"
#include "mxml/parser.hpp"
#include "mxml/version.hpp"
#include "mxml/text.hpp"

#include <functional>
#include <string>

namespace mxml
{

/**
 * @brief struct containing the doctype information
 */

struct doc_type
{
	std::string m_root;
	std::string m_pubid; // pubid is empty for SYSTEM DOCTYPE
	std::string m_dtd;
};

/**
 * mxml::document is the class that contains a parsed XML file.
 * You can create an empty document and add nodes to it, or you can
 * create it by parsing a string or an std::istream containing XML.
 *
 * If you use an std::fstream to read a file, be sure to open the file
 * ios::binary. Otherwise, the detection of text encoding might go wrong
 * or the content can become corrupted.
 *
 * Default is to parse CDATA sections into mxml::text nodes. If you
 * want to preserve CDATA sections in the DOM tree, you have to call
 * set_preserve_cdata before reading the file.
 *
 * By default a document is not validated. But you can turn on validation
 * by using the appropriate constructor or read method, or by setting
 * set_validating explicitly. The DTD's will be loaded from the base dir
 * specified, but you can change this by assigning a external_entity_ref_handler.
 *
 * A document can have only one mxml::element child node even though it is
 * derived from mxml::element_container. The document object itself is the
 * so-called root-node.
 */

class document final : public element_container
{
  public:
	/// \brief node_type of a document
	node_type type() const override { return node_type::document; }

	/// \brief Constructor for an empty document.
	document();

	/// \brief Copy constructor
	document(const document &doc);

	/// \brief Move constructor
	document(document &&other) noexcept
		: document()
	{
		swap(*this, other);
	}

	/// \brief operator=
	document &operator=(document doc) noexcept
	{
		swap(*this, doc);
		return *this;
	}

	/// \brief Constructor that will parse the XML passed in argument \a s using default settings
	document(std::string_view s);

	/// \brief Constructor that will parse the XML passed in argument \a is using default settings
	document(std::istream &is);

	/// \brief Constructor that will parse the XML passed in argument \a is. This
	/// constructor will also validate the input using DTD's found in \a base_dir
	document(std::istream &is, const std::string &base_dir);

	~document() = default;

	friend void swap(document &a, document &b) noexcept;

	/// options for parsing
	/// validating uses a DTD if it is defined
	bool is_validating() const { return m_validating; }
	void set_validating(bool validate) { m_validating = validate; }

	/// validating_ns: when validating take the NS 1.0 specification into account
	bool is_validating_ns() const { return m_validating_ns; }
	void set_validating_ns(bool validate) { m_validating_ns = validate; }

	/// preserve cdata, preserves CDATA sections instead of converting them
	/// into text nodes.
	bool preserves_cdata() const { return m_preserve_cdata; }

	/// \brief if \a p is true, the CDATA sections will be preserved when parsing XML, if \a p is false, the content of the CDATA will be treated as text
	void set_preserve_cdata(bool p) { m_preserve_cdata = p; }

	/// \brief collapse means replacing e.g. `<foo></foo>` with `<foo/>`
	bool collapses_empty_tags() const { return m_fmt.collapse_tags; }

	/// \brief if \a c is true, empty tags will be replaced, i.e. write `<foo/>` instead of `<foo></foo>`
	void set_collapse_empty_tags(bool c) { m_fmt.collapse_tags = c; }

	/// \brief collapse 'empty elements' according to HTML rules
	bool write_html() const { return m_fmt.html; }

	/// \brief if \a c is true, 'empty elements' will be collapsed according to HTML rules
	void set_write_html(bool f) { m_fmt.html = f; }

	/// \brief whether to write out comments
	bool suppresses_comments() const { return m_fmt.suppress_comments; }

	/// \brief if \a s is true, comments will not be written
	void set_suppress_comments(bool s) { m_fmt.suppress_comments = s; }

	/// \brief whether to escape white space
	bool escapes_white_space() const { return m_fmt.escape_white_space; }

	/// \brief if \a e is true, white space will be written as XML entities
	void set_escape_white_space(bool e) { m_fmt.escape_white_space = e; }

	/// \brief whether to escape double quotes
	bool escapes_double_quote() const { return m_fmt.escape_double_quote; }

	/// \brief if \a e is true, double quotes will be written as &quot;
	void set_escape_double_quote(bool e) { m_fmt.escape_double_quote = e; }

	/// \brief whether to place a newline after a prolog
	bool wraps_prolog() const { return m_wrap_prolog; }

	/// \brief if \a w is true, a newline will be written after the XML prolog
	void set_wrap_prolog(bool w) { m_wrap_prolog = w; }

	/// \brief Get the doctype as parsed
	doc_type get_doctype() const { return m_doctype; }

	/// \brief Set the doctype to write out
	void set_doctype(const std::string &root, const std::string &pubid, const std::string &dtd)
	{
		set_doctype({ root, pubid, dtd });
	}

	/// Set the doctype to write out
	void set_doctype(const doc_type &doctype)
	{
		m_doctype = doctype;
		m_write_doctype = true;
	}

	/// \brief whether to write a XML prolog
	bool writes_xml_decl() const { return m_write_xml_decl; }

	/// \brief if \a w is true, an XML prolog will be written
	void set_write_xml_decl(bool w) { m_write_xml_decl = w; }

	/// \brief whether to write a DOCTYPE
	bool writes_doctype() const { return m_write_doctype; }

	/// \brief if \a f is true a DOCTYPE will be written
	void set_write_doctype(bool f) { m_write_doctype = f; }

	/// \brief Check the doctype to see if this is supposed to be HTML5
	bool is_html5() const;

	/// \brief Write out the document
	friend std::ostream &operator<<(std::ostream &os, const document &doc);

	/// \brief Read in a document
	friend std::istream &operator>>(std::istream &is, document &doc);

	/// Compare two xml documents
	bool operator==(const document &doc) const;

	/// If you want to validate the document using DTD files stored on disk, you can specifiy this directory prior to reading
	/// the document.
	void set_base_dir(const std::string &path);

	/**
	 * @brief Set a callback for loading external entities
	 *
	 * The default for MXML is to locate the external reference based
	 * on sysid and base_dir. Only local files are loaded this way.
	 *
	 * You can specify a entity loader here if you want to be able to load
	 * DTD files from another source.
	 */
	template <typename Callback>
	void set_entity_loader(Callback &&cb)
	{
		m_external_entity_ref_loader = cb;
	}

	encoding_type get_encoding() const;   ///< The text encoding as detected in the input.
	void set_encoding(encoding_type enc); ///< The text encoding to use for output

	version_type get_version() const; ///< XML version, should be either 1.0 or 1.1
	void set_version(version_type v); ///< XML version, should be either 1.0 or 1.1

	element_container *root() override { return this; }             ///< The root node, which is the document of course
	const element_container *root() const override { return this; } ///< The root node, which is the document of course

	/// @brief Return the single child, or nullptr in case the document is empty
	element *child()
	{
		return empty() ? nullptr : &front();
	}

	/// @brief Return the single child, or nullptr in case the document is empty
	const element *child() const { return const_cast<document *>(this)->child(); }

	/// @brief Emplace a single element using \a args for the construction
	template <typename... Args>
		requires std::is_constructible_v<element, Args...>
	auto emplace(Args &&...args)
	{
		return emplace_back(std::forward<Args>(args)...);
	}

	/// @brief Return the concatenation of all contained text nodes
	std::string str() const override;

  protected:
	/** @cond */
	node *insert_impl(const node *p, node *n) override;

	void XmlDeclHandler(encoding_type encoding, bool standalone, version_type version);
	void StartElementHandler(const std::string &name, const std::string &uri, const parser::attr_list_type &atts);
	void EndElementHandler(const std::string &name, const std::string &uri);
	void CharacterDataHandler(const std::string &data);
	void ProcessingInstructionHandler(const std::string &target, const std::string &data);
	void CommentHandler(const std::string &comment);
	void StartCdataSectionHandler();
	void EndCdataSectionHandler();
	void StartNamespaceDeclHandler(const std::string &prefix, const std::string &uri);
	void EndNamespaceDeclHandler(const std::string &prefix);
	void DoctypeDeclHandler(const std::string &root, const std::string &publicId, const std::string &uri);
	void NotationDeclHandler(const std::string &name, const std::string &sysid, const std::string &pubid);

	std::istream *external_entity_ref(const std::string &base, const std::string &pubid, const std::string &sysid);
	void parse(std::istream &data);

	std::function<std::istream *(const std::string &base, const std::string &pubid, const std::string &sysid)>
		m_external_entity_ref_loader;

	void write(std::ostream &os, format_info fmt) const override;

	std::string m_dtd_dir;

	// some content information
	doc_type m_doctype;
	bool m_validating;
	bool m_validating_ns = false;
	bool m_preserve_cdata;
	bool m_has_xml_decl;
	encoding_type m_encoding;
	version_type m_version;
	bool m_standalone;
	bool m_wrap_prolog = true;
	bool m_write_doctype = false;
	bool m_write_xml_decl = false;

	format_info m_fmt;

	struct notation
	{
		std::string m_name;
		std::string m_sysid;
		std::string m_pubid;
	};

	element_container *m_cur = nullptr; // construction
	cdata *m_cdata = nullptr;           // only defined in a CDATA section
	std::vector<std::pair<std::string, std::string>> m_namespaces;
	std::vector<notation> m_notations;
	size_t m_root_size_at_first_notation = 0; // for processing instructions that occur before a notation

	/** @endcond */
};

namespace literals
{
	/**
	 * @brief This operator allows you to construct static XML
	 * documents from strings. As in this example:
	 *
	 * @code{.cpp}
	 * using namespace mxml::literals;
	 *
	 * mxml::document doc = "<text>Hello, world!</text>"_xml;"
	 * @endcode
	 */
	document operator""_xml(const char *text, size_t length);
} // namespace literals

} // namespace mxml
