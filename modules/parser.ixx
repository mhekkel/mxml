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
/// definition of the libzeep XML parser, a recursive descent parser

#include <functional>
#include <list>
#include <string>

export module mxml:parser;

import :error;
import :node;
import :text;

namespace mxml
{

struct attr
{
	std::string m_ns;
	std::string m_name;
	std::string m_value;
	bool m_id; // whether the attribute is defined as type ID in its ATTLIST decl
};

/// If an invalid_exception is thrown, it means the XML document is not valid: it does
/// not conform the DTD specified in the XML document.
/// This is only thrown when validation is enabled.
///
/// The what() member of the exception object will contain an explanation.

class invalid_exception : public exception
{
  public:
	invalid_exception(std::string_view msg)
		: exception(msg)
	{
	}
	~invalid_exception() noexcept {}
};

/// If an not_wf_exception is thrown, it means the XML document is not well formed.
/// Often this means syntax errors, missing \< or \> characters, non matching open
/// and close tags, etc.
///
/// The what() member of the exception object will contain an explanation.

class not_wf_exception : public exception
{
  public:
	not_wf_exception(std::string_view msg)
		: exception(msg)
	{
	}
	~not_wf_exception() noexcept {}
};

/// zeep::xml::parser is a SAX parser. After construction, you should assign
/// call back handlers for the SAX events and then call parse().

class parser
{
  public:
	using attr_type = attr;
	using attr_list_type = std::list<attr>;

	parser(std::istream &is);
	parser(std::string_view s);

	virtual ~parser();

	std::function<void(encoding_type encoding, bool standalone, float version)> xml_decl_handler;
	std::function<void(std::string_view name, std::string_view uri, const attr_list_type &atts)> start_element_handler;
	std::function<void(std::string_view name, std::string_view uri)> end_element_handler;
	std::function<void(std::string_view data)> character_data_handler;
	std::function<void(std::string_view target, std::string_view data)> processing_instruction_handler;
	std::function<void(std::string_view data)> comment_handler;
	std::function<void()> start_cdata_section_handler;
	std::function<void()> end_cdata_section_handler;
	std::function<void(std::string_view prefix, std::string_view uri)> start_namespace_decl_handler;
	std::function<void(std::string_view prefix)> end_namespace_decl_handler;
	std::function<void(std::string_view root, std::string_view publicId, std::string_view uri)> doctype_decl_handler;
	std::function<void(std::string_view name, std::string_view systemId, std::string_view publicId)> notation_decl_handler;
	std::function<std::istream *(std::string_view base, std::string_view pubid, std::string_view uri)> external_entity_ref_handler;
	std::function<void(std::string_view msg)> report_invalidation_handler;

	void parse(bool validate, bool validate_ns);

  protected:
	friend struct parser_imp;

	virtual void xml_decl(encoding_type encoding, bool standalone, float version);

	virtual void doctype_decl(std::string_view root, std::string_view publicId, std::string_view uri);

	virtual void start_element(std::string_view name,
		std::string_view uri, const attr_list_type &atts);

	virtual void end_element(std::string_view name, std::string_view uri);

	virtual void character_data(std::string_view data);

	virtual void processing_instruction(std::string_view target, std::string_view data);

	virtual void comment(std::string_view data);

	virtual void start_cdata_section();

	virtual void end_cdata_section();

	virtual void start_namespace_decl(std::string_view prefix, std::string_view uri);

	virtual void end_namespace_decl(std::string_view prefix);

	virtual void notation_decl(std::string_view name,
		std::string_view systemId, std::string_view publicId);

	virtual void report_invalidation(std::string_view msg);

	virtual std::istream *
	external_entity_ref(std::string_view base,
		std::string_view pubid, std::string_view uri);

	struct parser_imp *m_impl;
	std::istream *m_istream;
};

} // namespace mxml