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

#include <functional>
#include <list>
#include <string>
#include <numeric>

#include <cassert>

export module mxml:doctype;

import :error;
import :text;

namespace mxml::doctype
{
// --------------------------------------------------------------------
// doctype support with full validation.

class element;
class attlist;
class entity;
class attribute;

using entity_list = std::vector<entity *>;
using element_list = std::vector<element *>;
using attribute_list = std::vector<attribute *>;

// --------------------------------------------------------------------

enum class content_spec_type
{
	Empty,
	Any,
	Mixed,
	Children
};

// --------------------------------------------------------------------
// validation of elements is done by the validator classes

struct content_spec_base;
struct state_base;

using content_spec_list = std::list<content_spec_base *>;

class validator
{
  public:
	validator(content_spec_base *allowed);
	validator(const element *e);

	validator(const validator &other) = delete;
	validator &operator=(const validator &other) = delete;

	~validator();

	bool allow(std::string_view name);
	content_spec_type get_content_spec() const;
	bool done();

  private:
	state_base *m_state;
	content_spec_base *m_allowed;
	bool m_done;
};

// --------------------------------------------------------------------

struct content_spec_base
{
	content_spec_base(const content_spec_base &) = delete;
	content_spec_base &operator=(const content_spec_base &) = delete;

	virtual ~content_spec_base() = default;

	virtual state_base *create_state() const = 0;
	virtual bool element_content() const { return false; }

	content_spec_type get_content_spec() const { return m_content_spec; }

  protected:
	content_spec_base(content_spec_type contentSpec)
		: m_content_spec(contentSpec)
	{
	}

	content_spec_type m_content_spec;
};

struct content_spec_any : public content_spec_base
{
	content_spec_any()
		: content_spec_base(content_spec_type::Any)
	{
	}

	state_base *create_state() const override;
};

struct content_spec_empty : public content_spec_base
{
	content_spec_empty()
		: content_spec_base(content_spec_type::Empty)
	{
	}

	state_base *create_state() const override;
};

struct content_spec_element : public content_spec_base
{
	content_spec_element(std::string_view name)
		: content_spec_base(content_spec_type::Children)
		, m_name(name)
	{
	}

	state_base *create_state() const override;
	bool element_content() const override { return true; }

	std::string m_name;
};

struct content_spec_repeated : public content_spec_base
{
	content_spec_repeated(content_spec_base *allowed, char repetion)
		: content_spec_base(allowed->get_content_spec())
		, m_allowed(allowed)
		, m_repetition(repetion)
	{
		assert(allowed);
	}

	~content_spec_repeated();

	state_base *create_state() const override;
	bool element_content() const override;

	content_spec_base *m_allowed;
	char m_repetition;
};

struct content_spec_seq : public content_spec_base
{
	content_spec_seq(content_spec_base *a)
		: content_spec_base(a->get_content_spec())
	{
		add(a);
	}
	~content_spec_seq();

	void add(content_spec_base *a);

	state_base *create_state() const override;
	bool element_content() const override;

	content_spec_list m_allowed;
};

struct content_spec_choice : public content_spec_base
{
	content_spec_choice(bool mixed)
		: content_spec_base(mixed ? content_spec_type::Mixed : content_spec_type::Children)
		, m_mixed(mixed)
	{
	}
	content_spec_choice(content_spec_base *a, bool mixed)
		: content_spec_base(mixed ? content_spec_type::Mixed : a->get_content_spec())
		, m_mixed(mixed)
	{
		add(a);
	}
	~content_spec_choice();

	void add(content_spec_base *a);

	state_base *create_state() const override;
	bool element_content() const override;

	content_spec_list m_allowed;
	bool m_mixed;
};

// --------------------------------------------------------------------

enum class attribute_type
{
	CDATA,
	ID,
	IDREF,
	IDREFS,
	ENTITY,
	ENTITIES,
	NMTOKEN,
	NMTOKENS,
	Notation,
	Enumerated
};

enum class attribute_default
{
	None,
	Required,
	Implied,
	Fixed,
	Default
};

class attribute
{
  public:
	attribute(std::string_view name, attribute_type type)
		: m_name(name)
		, m_type(type)
		, m_default(attribute_default::None)
		, m_external(false)
	{
	}

	attribute(std::string_view name, attribute_type type,
		const std::vector<std::string> &enums)
		: m_name(name)
		, m_type(type)
		, m_default(attribute_default::None)
		, m_enum(enums)
		, m_external(false)
	{
	}

	const std::string &name() const { return m_name; }

	bool validate_value(std::string &value, const entity_list &entities) const;

	void set_default(attribute_default def, std::string_view value)
	{
		m_default = def;
		m_default_value = value;
	}

	std::tuple<attribute_default, std::string>
	get_default() const { return std::make_tuple(m_default, m_default_value); }

	attribute_type get_type() const { return m_type; }
	attribute_default get_default_type() const { return m_default; }
	const std::vector<std::string> &get_enums() const { return m_enum; }

	void set_external(bool external) { m_external = external; }
	bool is_external() const { return m_external; }

  private:
	// routines used to check _and_ reformat attribute value strings
	bool is_name(std::string &s) const;
	bool is_names(std::string &s) const;
	bool is_nmtoken(std::string &s) const;
	bool is_nmtokens(std::string &s) const;

	bool is_unparsed_entity(std::string_view s, const entity_list &l) const;

	std::string m_name;
	attribute_type m_type;
	attribute_default m_default;
	std::string m_default_value;
	std::vector<std::string> m_enum;
	bool m_external;
};

// --------------------------------------------------------------------

class element
{
  public:
	element(const element &) = delete;
	element &operator=(const element &) = delete;

	element(std::string_view name, bool declared, bool external)
		: m_name(name)
		, m_allowed(nullptr)
		, m_declared(declared)
		, m_external(external)
	{
	}

	~element();

	const attribute_list &get_attributes() const { return m_attlist; }

	void add_attribute(attribute *attr);

	const attribute *get_attribute(std::string_view name) const;

	const std::string &name() const { return m_name; }

	bool is_declared() const { return m_declared; }

	bool empty() const;

	void set_allowed(content_spec_base *allowed);
	content_spec_base *get_allowed() const { return m_allowed; }

  private:
	std::string m_name;
	attribute_list m_attlist;
	content_spec_base *m_allowed;
	bool m_declared, m_external;
};

// --------------------------------------------------------------------

class entity
{
  public:
	entity(const entity &) = delete;
	entity &operator=(const entity &) = delete;

	const std::string &name() const { return m_name; }
	const std::string &get_replacement() const { return m_replacement; }
	const std::string &get_path() const { return m_path; }

	bool is_parsed() const { return m_parsed; }

	const std::string &get_ndata() const { return m_ndata; }
	void set_ndata(std::string_view ndata) { m_ndata = ndata; }

	bool is_external() const { return m_external; }

	bool is_externally_defined() const { return m_externally_defined; }
	void set_externally_defined(bool externally_defined)
	{
		m_externally_defined = externally_defined;
	}

  protected:
	entity(std::string_view name, std::string_view replacement,
		bool external, bool parsed)
		: m_name(name)
		, m_replacement(replacement)
		, m_parameter(false)
		, m_parsed(parsed)
		, m_external(external)
		, m_externally_defined(false)
	{
	}

	entity(std::string_view name, std::string_view replacement,
		std::string_view path)
		: m_name(name)
		, m_replacement(replacement)
		, m_path(path)
		, m_parameter(true)
		, m_parsed(true)
		, m_external(true)
		, m_externally_defined(false)
	{
	}

	std::string m_name;
	std::string m_replacement;
	std::string m_ndata;
	std::string m_path;
	bool m_parameter;
	bool m_parsed;
	bool m_external;
	bool m_externally_defined;
};

class general_entity : public entity
{
  public:
	general_entity(std::string_view name, std::string_view replacement,
		bool external = false, bool parsed = true)
		: entity(name, replacement, external, parsed)
	{
	}
};

class parameter_entity : public entity
{
  public:
	parameter_entity(std::string_view name, std::string_view replacement,
		std::string_view path)
		: entity(name, replacement, path)
	{
	}
};

// --------------------------------------------------------------------
// HTML5 named character support

const general_entity *get_named_character(std::string_view name);

} // namespace mxml::doctype
