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
 * definition of the mxml XML parser, a recursive descent parser
 */

#include "mxml/error.hpp"
#include "mxml/text.hpp"
#include "mxml/version.hpp"

#include <functional>
#include <istream>
#include <string>

namespace mxml
{

/// If an invalid_exception is thrown, it means the XML document is not valid: it does
/// not conform the DTD specified in the XML document.
/// This is only thrown when validation is enabled.
///
/// The what() member of the exception object will contain an explanation.

class invalid_exception : public exception
{
  public:
	invalid_exception(const std::string &msg)
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
	not_wf_exception(const std::string &msg)
		: exception(msg)
	{
	}
	~not_wf_exception() noexcept {}
};

/**
 * @brief A SAX parser
 *
 * mxml::parser is a SAX parser. After construction, you should assign
 * call back handlers for the SAX events and then call parse().
 */

class parser
{
  public:
	/**
	 * @brief Struct containing information about a parsed attribute
	 *
	 */
	struct attr
	{
		std::string m_ns;    ///< The namespace for this attribute
		std::string m_name;  ///< The name of the attribute
		std::string m_value; ///< The value of the attribute
		bool m_id;           ///< Flag indicating the attribute is defined as type ID in its ATTLIST decl
	};

	using attr_list_type = std::vector<attr>;

	/// @brief constructor taking a std::istream in \a is
	parser(std::istream &is);

	/// @brief destructor
	virtual ~parser();

	/**
	 * The callbacks can be set by assinging a callback to each of
	 * the following callback function variables.
	 */

	std::function<void(encoding_type encoding, bool standalone, version_type version)> xml_decl_handler;
	std::function<void(const std::string &name, const std::string &uri, const attr_list_type &atts)> start_element_handler;
	std::function<void(const std::string &name, const std::string &uri)> end_element_handler;
	std::function<void(const std::string &data)> character_data_handler;
	std::function<void(const std::string &target, const std::string &data)> processing_instruction_handler;
	std::function<void(const std::string &data)> comment_handler;
	std::function<void()> start_cdata_section_handler;
	std::function<void()> end_cdata_section_handler;
	std::function<void(const std::string &prefix, const std::string &uri)> start_namespace_decl_handler;
	std::function<void(const std::string &prefix)> end_namespace_decl_handler;
	std::function<void(const std::string &root, const std::string &publicId, const std::string &uri)> doctype_decl_handler;
	std::function<void(const std::string &name, const std::string &systemId, const std::string &publicId)> notation_decl_handler;
	std::function<std::istream *(const std::string &base, const std::string &pubid, const std::string &uri)> external_entity_ref_handler;
	std::function<void(const std::string &msg)> report_invalidation_handler;

	/** @brief Start the actual parsing, optionally validating content and namespaces */
	void parse(bool validate, bool validate_ns);

  protected:
	/** @cond */
	friend struct parser_imp;

	virtual void xml_decl(encoding_type encoding, bool standalone, version_type version);

	virtual void doctype_decl(const std::string &root, const std::string &publicId, const std::string &uri);

	virtual void start_element(const std::string &name,
		const std::string &uri, const attr_list_type &atts);

	virtual void end_element(const std::string &name, const std::string &uri);

	virtual void character_data(const std::string &data);

	virtual void processing_instruction(const std::string &target, const std::string &data);

	virtual void comment(const std::string &data);

	virtual void start_cdata_section();

	virtual void end_cdata_section();

	virtual void start_namespace_decl(const std::string &prefix, const std::string &uri);

	virtual void end_namespace_decl(const std::string &prefix);

	virtual void notation_decl(const std::string &name,
		const std::string &systemId, const std::string &publicId);

	virtual void report_invalidation(const std::string &msg);

	virtual std::istream *external_entity_ref(const std::string &base,
		const std::string &pubid, const std::string &uri);

	struct parser_imp *m_impl;
	std::istream *m_istream;

	/** @endcond */
};

} // namespace mxml
