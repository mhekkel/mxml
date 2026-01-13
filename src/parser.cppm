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

/**
 * \file
 * definition of the zeem XML parser, a recursive descent parser
 */

#include <functional>
#include <istream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module zeem:parser;

import :error;
import :text;
import :version;

namespace zeem
{

export enum class encoding_type;
export struct version_type;

/// If an invalid_exception is thrown, it means the XML document is not valid: it does
/// not conform the DTD specified in the XML document.
/// This is only thrown when validation is enabled.
///
/// The what() member of the exception object will contain an explanation.

export class invalid_exception : public exception
{
  public:
	explicit invalid_exception(std::string msg)
		: exception(std::move(msg))
	{
	}
};

/// If an not_wf_exception is thrown, it means the XML document is not well formed.
/// Often this means syntax errors, missing \< or \> characters, non matching open
/// and close tags, etc.
///
/// The what() member of the exception object will contain an explanation.

export class not_wf_exception : public exception
{
  public:
	explicit not_wf_exception(std::string msg)
		: exception(std::move(msg))
	{
	}
};

/**
 * @brief A SAX parser
 *
 * zeem::parser is a SAX parser. After construction, you should assign
 * call back handlers for the SAX events and then call parse().
 */

export class parser
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
		bool m_id{};         ///< Flag indicating the attribute is defined as type ID in its ATTLIST decl
	};

	using attr_list_type = std::vector<attr>;

	/// @brief constructor taking a std::istream in \a is
	explicit parser(std::istream &is);

	/// @brief destructor
	virtual ~parser();

	/**
	 * The callbacks can be set by assinging a callback to each of
	 * the following callback function variables.
	 */

	std::function<void(encoding_type encoding, bool standalone, version_type version)> xml_decl_handler;
	std::function<void(std::string name, std::string uri, const attr_list_type &atts)> start_element_handler;
	std::function<void(std::string name, std::string uri)> end_element_handler;
	std::function<void(std::string data)> character_data_handler;
	std::function<void(std::string target, std::string data)> processing_instruction_handler;
	std::function<void(std::string data)> comment_handler;
	std::function<void()> start_cdata_section_handler;
	std::function<void()> end_cdata_section_handler;
	std::function<void(std::string prefix, std::string uri)> start_namespace_decl_handler;
	std::function<void(std::string prefix)> end_namespace_decl_handler;
	std::function<void(std::string root, std::string publicId, std::string uri)> doctype_decl_handler;
	std::function<void(std::string name, std::string systemId, std::string publicId)> notation_decl_handler;
	std::function<std::unique_ptr<std::istream>(std::string_view base, std::string_view pubid, std::string_view uri)> external_entity_ref_handler;
	std::function<void(std::string msg)> report_invalidation_handler;

	/** @brief Start the actual parsing, optionally validating content and namespaces */
	void parse(bool validate, bool validate_ns);

  protected:
	/** @cond */
	friend struct parser_imp;

	virtual void xml_decl(encoding_type encoding, bool standalone, version_type version);

	virtual void doctype_decl(std::string root, std::string publicId, std::string uri);

	virtual void start_element(std::string name, std::string uri, const attr_list_type &atts);

	virtual void end_element(std::string name, std::string uri);

	virtual void character_data(std::string data);

	virtual void processing_instruction(std::string target, std::string data);

	virtual void comment(std::string data);

	virtual void start_cdata_section();

	virtual void end_cdata_section();

	virtual void start_namespace_decl(std::string prefix, std::string uri);

	virtual void end_namespace_decl(std::string prefix);

	virtual void notation_decl(std::string name,
		std::string systemId, std::string publicId);

	virtual void report_invalidation(std::string msg);

	virtual std::unique_ptr<std::istream> external_entity_ref(std::string_view base,
		std::string_view pubid, std::string_view uri);

  private:
	struct parser_imp *m_impl;
	std::istream *m_istream = nullptr;

	/** @endcond */
};

} // namespace zeem
