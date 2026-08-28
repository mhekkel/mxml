// Copyright (c) 2024-2026 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

/** @file
 * File containing code to support DOCTYPE handling. This is private code
 * to the zeem library.
 *
 * @cond
 */

#ifndef ZEEM_CXX_MODULE
# include "zeem/export.hpp"
# include <cassert>
# include <memory>
# include <string>
# include <string_view>
# include <tuple>
# include <utility>
# include <vector>
#endif

namespace zeem::doctype
{
// --------------------------------------------------------------------
// doctype support with full validation.

class attribute;
class element;
class entity;

using entity_ptr = std::shared_ptr<entity>;
using entity_list = std::vector<entity_ptr>;

using element_ptr = std::shared_ptr<element>;
using element_list = std::vector<element_ptr>;

using attribute_ptr = std::shared_ptr<attribute>;
using attribute_list = std::vector<attribute_ptr>;

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
using content_spec_base_ptr = std::shared_ptr<content_spec_base>;

struct state_base;
using state_base_ptr = std::shared_ptr<state_base>;

using content_spec_list = std::vector<content_spec_base_ptr>;

class validator
{
  public:
	explicit validator(content_spec_base &allowed);
	explicit validator(const element_ptr &e);

	validator(const validator &other) = delete;
	validator &operator=(const validator &other) = delete;

	bool allow(std::string_view name);
	[[nodiscard]] content_spec_type get_content_spec() const;
	bool done();

  private:
	state_base_ptr m_state;
	content_spec_type m_allowed;
	bool m_done;
};

// --------------------------------------------------------------------

struct content_spec_base
{
	content_spec_base(const content_spec_base &) = delete;
	content_spec_base &operator=(const content_spec_base &) = delete;

	virtual ~content_spec_base() = default;

	[[nodiscard]] virtual state_base_ptr create_state() const = 0;
	[[nodiscard]] virtual bool element_content() const { return false; }

	[[nodiscard]] content_spec_type get_content_spec() const { return m_content_spec; }

  protected:
	explicit content_spec_base(content_spec_type contentSpec)
		: m_content_spec(contentSpec)
	{
	}

  private:
	content_spec_type m_content_spec;
};

struct content_spec_any : public content_spec_base
{
	content_spec_any()
		: content_spec_base(content_spec_type::Any)
	{
	}

	[[nodiscard]] state_base_ptr create_state() const override;
};

struct content_spec_empty : public content_spec_base
{
	content_spec_empty()
		: content_spec_base(content_spec_type::Empty)
	{
	}

	[[nodiscard]] state_base_ptr create_state() const override;
};

struct content_spec_element : public content_spec_base
{
	explicit content_spec_element(std::string name)
		: content_spec_base(content_spec_type::Children)
		, m_name(std::move(name))
	{
	}

	[[nodiscard]] state_base_ptr create_state() const override;
	[[nodiscard]] bool element_content() const override { return true; }

  private:
	std::string m_name;
};

struct content_spec_repeated : public content_spec_base
{
	content_spec_repeated(const content_spec_base_ptr &allowed, char repetion)
		: content_spec_base(allowed->get_content_spec())
		, m_allowed(allowed)
		, m_repetition(repetion)
	{
		assert(allowed);
	}

	[[nodiscard]] state_base_ptr create_state() const override;
	[[nodiscard]] bool element_content() const override;

  private:
	content_spec_base_ptr m_allowed;
	char m_repetition;
};

struct content_spec_seq : public content_spec_base
{
	explicit content_spec_seq(const content_spec_base_ptr &a)
		: content_spec_base(a->get_content_spec())
	{
		add(a);
	}

	void add(content_spec_base_ptr a);

	[[nodiscard]] state_base_ptr create_state() const override;
	[[nodiscard]] bool element_content() const override;

  private:
	content_spec_list m_allowed;
};

struct content_spec_choice : public content_spec_base
{
	explicit content_spec_choice(bool mixed)
		: content_spec_base(mixed ? content_spec_type::Mixed : content_spec_type::Children)
		, m_mixed(mixed)
	{
	}
	content_spec_choice(const content_spec_base_ptr &a, bool mixed)
		: content_spec_base(mixed ? content_spec_type::Mixed : a->get_content_spec())
		, m_mixed(mixed)
	{
		add(a);
	}

	void add(content_spec_base_ptr a);

	[[nodiscard]] state_base_ptr create_state() const override;
	[[nodiscard]] bool element_content() const override;

  private:
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
	attribute(std::string name, attribute_type type)
		: m_name(std::move(name))
		, m_type(type)
		, m_default(attribute_default::None)
		, m_external(false)
	{
	}

	attribute(std::string name, attribute_type type,
		const std::vector<std::string> &enums)
		: m_name(std::move(name))
		, m_type(type)
		, m_default(attribute_default::None)
		, m_enum(enums)
		, m_external(false)
	{
	}

	[[nodiscard]] const std::string &name() const { return m_name; }

	bool validate_value(std::string &value, const entity_list &entities) const;

	void set_default(attribute_default def, std::string value)
	{
		m_default = def;
		m_default_value = std::move(value);
	}

	[[nodiscard]] std::tuple<attribute_default, std::string> get_default() const { return std::make_tuple(m_default, m_default_value); }

	[[nodiscard]] attribute_type get_type() const { return m_type; }
	[[nodiscard]] attribute_default get_default_type() const { return m_default; }
	[[nodiscard]] const std::vector<std::string> &get_enums() const { return m_enum; }

	void set_external(bool external) { m_external = external; }
	[[nodiscard]] bool is_external() const { return m_external; }

  private:
	// routines used to check _and_ reformat attribute value strings
	bool is_name(std::string &s) const;
	bool is_names(std::string &s) const;
	bool is_nmtoken(std::string &s) const;
	bool is_nmtokens(std::string &s) const;

	[[nodiscard]] bool is_unparsed_entity(std::string_view s, const entity_list &l) const;

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

	element(std::string name, bool declared, [[maybe_unused]] bool external)
		: m_name(std::move(name))
		, m_allowed(nullptr)
		, m_declared(declared)
	{
	}

	[[nodiscard]] const attribute_list &get_attributes() const { return m_attlist; }

	void add_attribute(attribute_ptr attr);

	[[nodiscard]] const attribute_ptr get_attribute(std::string_view name) const;

	[[nodiscard]] const std::string &name() const { return m_name; }

	[[nodiscard]] bool is_declared() const { return m_declared; }

	void set_allowed(content_spec_base_ptr allowed);
	[[nodiscard]] content_spec_base_ptr get_allowed() const { return m_allowed; }

  private:
	std::string m_name;
	attribute_list m_attlist;
	content_spec_base_ptr m_allowed;
	bool m_declared;
};

// --------------------------------------------------------------------

class entity
{
  public:
	entity(const entity &) = default;
	entity &operator=(const entity &) = default;

	[[nodiscard]] const std::string &name() const { return m_name; }
	[[nodiscard]] const std::string &get_replacement() const { return m_replacement; }
	[[nodiscard]] const std::string &get_path() const { return m_path; }

	[[nodiscard]] bool is_parsed() const { return m_parsed; }

	[[nodiscard]] const std::string &get_ndata() const { return m_ndata; }
	void set_ndata(std::string ndata) { m_ndata = std::move(ndata); }

	[[nodiscard]] bool is_external() const { return m_external; }

	[[nodiscard]] bool is_externally_defined() const { return m_externally_defined; }
	void set_externally_defined(bool externally_defined)
	{
		m_externally_defined = externally_defined;
	}

  protected:
	entity(std::string name, std::string replacement,
		bool external, bool parsed)
		: m_name(std::move(name))
		, m_replacement(std::move(replacement))
		// , m_parameter(false)
		, m_parsed(parsed)
		, m_external(external)
		, m_externally_defined(false)
	{
	}

	entity(std::string name, std::string replacement, std::string path)
		: m_name(std::move(name))
		, m_replacement(std::move(replacement))
		, m_path(std::move(path))
		// , m_parameter(true)
		, m_parsed(true)
		, m_external(true)
		, m_externally_defined(false)
	{
	}

  private:
	std::string m_name;
	std::string m_replacement;
	std::string m_ndata;
	std::string m_path;
	// bool m_parameter;
	bool m_parsed;
	bool m_external;
	bool m_externally_defined;
};

class general_entity : public entity
{
  public:
	general_entity(const general_entity &) = default;

	general_entity(std::string name, std::string replacement,
		bool external = false, bool parsed = true)
		: entity(std::move(name), std::move(replacement), external, parsed)
	{
	}
};

class parameter_entity : public entity
{
  public:
	parameter_entity(std::string name, std::string replacement, std::string path)
		: entity(std::move(name), std::move(replacement), std::move(path))
	{
	}
};

// --------------------------------------------------------------------
// HTML5 named character support

const general_entity *get_named_character(std::string_view name);

} // namespace zeem::doctype

/** @endcond */