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
#include <memory>
#include <numeric>

#include <cassert>

export module mxml:doctype;

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

enum class ContentSpecType
{
	Empty,
	Any,
	Mixed,
	Children
};

// --------------------------------------------------------------------
// validation of elements is done by the validator classes

struct content_spec_base;
using content_spec_ptr = content_spec_base *;
using content_spec_list = std::list<content_spec_ptr>;

struct state_base;
using state_ptr = state_base *;

class validator
{
  public:
	validator(content_spec_base *allowed);
	validator(const element *e);

	validator(const validator &other) = delete;
	validator &operator=(const validator &other) = delete;

	~validator();

	bool allow(std::string_view name);
	ContentSpecType get_content_spec() const;
	bool done();

  private:
	state_ptr m_state;
	content_spec_ptr m_allowed;
	bool m_done;
};

// --------------------------------------------------------------------

struct content_spec_base
{
	content_spec_base(const content_spec_base &) = delete;
	content_spec_base &operator=(const content_spec_base &) = delete;

	virtual ~content_spec_base() {}

	virtual state_ptr create_state() const = 0;
	virtual bool element_content() const { return false; }

	ContentSpecType get_content_spec() const { return m_content_spec; }

  protected:
	content_spec_base(ContentSpecType contentSpec)
		: m_content_spec(contentSpec)
	{
	}

	ContentSpecType m_content_spec;
};

struct content_spec_any : public content_spec_base
{
	content_spec_any()
		: content_spec_base(ContentSpecType::Any)
	{
	}

	state_ptr create_state() const override;
};

struct content_spec_empty : public content_spec_base
{
	content_spec_empty()
		: content_spec_base(ContentSpecType::Empty)
	{
	}

	state_ptr create_state() const override;
};

struct content_spec_element : public content_spec_base
{
	content_spec_element(std::string_view name)
		: content_spec_base(ContentSpecType::Children)
		, m_name(name)
	{
	}

	state_ptr create_state() const override;
	bool element_content() const override { return true; }

	std::string m_name;
};

struct content_spec_repeated : public content_spec_base
{
	content_spec_repeated(content_spec_ptr allowed, char repetion)
		: content_spec_base(allowed->get_content_spec())
		, m_allowed(allowed)
		, m_repetition(repetion)
	{
		assert(allowed);
	}

	~content_spec_repeated();

	state_ptr create_state() const override;
	bool element_content() const override;

	content_spec_ptr m_allowed;
	char m_repetition;
};

struct content_spec_seq : public content_spec_base
{
	content_spec_seq(content_spec_ptr a)
		: content_spec_base(a->get_content_spec())
	{
		add(a);
	}
	~content_spec_seq();

	void add(content_spec_ptr a);

	state_ptr create_state() const override;
	bool element_content() const override;

	content_spec_list m_allowed;
};

struct content_spec_choice : public content_spec_base
{
	content_spec_choice(bool mixed)
		: content_spec_base(mixed ? ContentSpecType::Mixed : ContentSpecType::Children)
		, m_mixed(mixed)
	{
	}
	content_spec_choice(content_spec_ptr a, bool mixed)
		: content_spec_base(mixed ? ContentSpecType::Mixed : a->get_content_spec())
		, m_mixed(mixed)
	{
		add(a);
	}
	~content_spec_choice();

	void add(content_spec_ptr a);

	state_ptr create_state() const override;
	bool element_content() const override;

	content_spec_list m_allowed;
	bool m_mixed;
};

// --------------------------------------------------------------------

enum class AttributeType
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

enum class AttributeDefault
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
	attribute(std::string_view name, AttributeType type)
		: m_name(name)
		, m_type(type)
		, m_default(AttributeDefault::None)
		, m_external(false)
	{
	}

	attribute(std::string_view name, AttributeType type,
		const std::vector<std::string> &enums)
		: m_name(name)
		, m_type(type)
		, m_default(AttributeDefault::None)
		, m_enum(enums)
		, m_external(false)
	{
	}

	std::string_view name() const { return m_name; }

	bool validate_value(std::string &value, const entity_list &entities) const;

	void set_default(AttributeDefault def, std::string_view value)
	{
		m_default = def;
		m_default_value = value;
	}

	std::tuple<AttributeDefault, std::string>
	get_default() const { return std::make_tuple(m_default, m_default_value); }

	AttributeType get_type() const { return m_type; }
	AttributeDefault get_default_type() const { return m_default; }
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
	AttributeType m_type;
	AttributeDefault m_default;
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

	std::string_view name() const { return m_name; }

	bool is_declared() const { return m_declared; }

	bool empty() const;

	void set_allowed(content_spec_ptr allowed);
	content_spec_ptr get_allowed() const { return m_allowed; }

  private:
	std::string m_name;
	attribute_list m_attlist;
	content_spec_ptr m_allowed;
	bool m_declared, m_external;
};

// --------------------------------------------------------------------

class entity
{
  public:
	entity(const entity &) = delete;
	entity &operator=(const entity &) = delete;

	std::string_view name() const { return m_name; }
	std::string_view get_replacement() const { return m_replacement; }
	std::string_view get_path() const { return m_path; }

	bool is_parsed() const { return m_parsed; }

	std::string_view get_ndata() const { return m_ndata; }
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
// validator code

// a refcounted state base class
struct state_base : std::enable_shared_from_this<state_base>
{
	state_base()
		: m_ref_count(1)
	{
	}

	virtual std::tuple<bool, bool> allow(const std::string &name) = 0;
	virtual bool allow_char_data() { return false; }
	virtual bool allow_empty() { return false; }
	virtual bool must_be_empty() { return false; }

	virtual void reset() {}

	void reference() { ++m_ref_count; }
	void release()
	{
		if (--m_ref_count == 0)
			delete this;
	}

  protected:
	virtual ~state_base() { assert(m_ref_count == 0); }

  private:
	int m_ref_count;
};

struct state_any : public state_base
{
	virtual std::tuple<bool, bool>
	allow(const std::string & /*name*/) { return std::make_tuple(true, true); }
	virtual bool allow_char_data() { return true; }
	virtual bool allow_empty() { return true; }
};

struct state_empty : public state_base
{
	virtual std::tuple<bool, bool>
	allow(const std::string & /*name*/) { return std::make_tuple(false, true); }
	virtual bool allow_empty() { return true; }
	virtual bool must_be_empty() { return true; }
};

struct state_element : public state_base
{
	state_element(const std::string &name)
		: m_name(name)
		, m_done(false)
	{
	}

	virtual std::tuple<bool, bool>
	allow(const std::string &name)
	{
		bool result = false;
		if (not m_done and m_name == name)
			m_done = result = true;
		//									m_done = true;
		return std::make_tuple(result, m_done);
	}

	virtual void reset() { m_done = false; }

	std::string m_name;
	bool m_done;
};

struct state_repeated : public state_base
{
	state_repeated(content_spec_ptr sub)
		: m_sub(sub->create_state())
		, m_state(0)
	{
	}

	~state_repeated()
	{
		m_sub->release();
	}

	virtual void reset()
	{
		m_sub->reset();
		m_state = 0;
	}

	virtual bool allow_char_data() { return m_sub->allow_char_data(); }

	state_ptr m_sub;
	int m_state;
};

// repeat for ?

struct state_repeated_zero_or_once : public state_repeated
{
	state_repeated_zero_or_once(content_spec_ptr sub)
		: state_repeated(sub)
	{
	}

	std::tuple<bool, bool> allow(const std::string &name);

	virtual bool allow_empty() { return true; }
};

std::tuple<bool, bool> state_repeated_zero_or_once::allow(const std::string &name)
{
	// use a state machine
	enum State
	{
		state_Start = 0,
		state_Loop
	};

	bool result = false, done = false;

	switch (m_state)
	{
		case state_Start:
			std::tie(result, done) = m_sub->allow(name);
			if (result == true)
				m_state = state_Loop;
			else
				done = true;
			break;

		case state_Loop:
			std::tie(result, done) = m_sub->allow(name);
			if (result == false and done)
				done = true;
			break;
	}

	return std::make_tuple(result, done);
}

struct state_repeated_any : public state_repeated
{
	state_repeated_any(content_spec_ptr sub)
		: state_repeated(sub)
	{
	}

	std::tuple<bool, bool> allow(const std::string &name);

	virtual bool allow_empty() { return true; }
};

std::tuple<bool, bool> state_repeated_any::allow(const std::string &name)
{
	// use a state machine
	enum State
	{
		state_Start = 0,
		state_Loop
	};

	bool result = false, done = false;

	switch (m_state)
	{
		case state_Start:
			std::tie(result, done) = m_sub->allow(name);
			if (result == true)
				m_state = state_Loop;
			else
				done = true;
			break;

		case state_Loop:
			std::tie(result, done) = m_sub->allow(name);
			if (result == false and done)
			{
				m_sub->reset();
				std::tie(result, done) = m_sub->allow(name);
				if (result == false)
					done = true;
			}
			break;
	}

	return std::make_tuple(result, done);
}

struct state_repeated_at_least_once : public state_repeated
{
	state_repeated_at_least_once(content_spec_ptr sub)
		: state_repeated(sub)
	{
	}

	std::tuple<bool, bool> allow(const std::string &name);

	virtual bool allow_empty() { return m_sub->allow_empty(); }
};

std::tuple<bool, bool> state_repeated_at_least_once::allow(const std::string &name)
{
	// use a state machine
	enum State
	{
		state_Start = 0,
		state_FirstLoop,
		state_NextLoop
	};

	bool result = false, done = false;

	switch (m_state)
	{
		case state_Start:
			std::tie(result, done) = m_sub->allow(name);
			if (result == true)
				m_state = state_FirstLoop;
			break;

		case state_FirstLoop:
			std::tie(result, done) = m_sub->allow(name);
			if (result == false and done)
			{
				m_sub->reset();
				std::tie(result, done) = m_sub->allow(name);
				if (result == true)
					m_state = state_NextLoop;
			}
			break;

		case state_NextLoop:
			std::tie(result, done) = m_sub->allow(name);
			if (result == false and done)
			{
				m_sub->reset();
				std::tie(result, done) = m_sub->allow(name);
				if (result == false)
					done = true;
			}
			break;
	}

	return std::make_tuple(result, done);
}

// allow a sequence

struct state_seq : public state_base
{
	state_seq(const content_spec_list &allowed)
		: m_state(0)
	{
		for (content_spec_ptr a : allowed)
			m_states.push_back(a->create_state());
	}

	~state_seq()
	{
		for (state_ptr s : m_states)
			s->release();
	}

	virtual std::tuple<bool, bool>
	allow(const std::string &name);

	virtual void reset()
	{
		m_state = 0;
		for (auto state : m_states)
			state->reset();
		;
	}

	virtual bool allow_char_data()
	{
		bool result = false;
		for (state_ptr s : m_states)
		{
			if (s->allow_char_data())
			{
				result = true;
				break;
			}
		}

		return result;
	}

	virtual bool allow_empty();

	std::list<state_ptr> m_states;
	std::list<state_ptr>::iterator m_next;
	int m_state;
};

std::tuple<bool, bool> state_seq::allow(const std::string &name)
{
	bool result = false, done = false;

	enum State
	{
		state_Start,
		state_Element
	};

	switch (m_state)
	{
		case state_Start:
			m_next = m_states.begin();
			if (m_next == m_states.end())
			{
				done = true;
				break;
			}
			m_state = state_Element;
			// fall through

		case state_Element:
			std::tie(result, done) = (*m_next)->allow(name);
			while (result == false and done)
			{
				++m_next;

				if (m_next == m_states.end())
				{
					done = true;
					break;
				}

				std::tie(result, done) = (*m_next)->allow(name);
			}
			break;
	}

	return std::make_tuple(result, done);
}

bool state_seq::allow_empty()
{
	bool result;

	if (m_states.empty())
		result = true;
	else
	{
		result = accumulate(m_states.begin(), m_states.end(), true,
			[](bool flag, auto &state)
			{ return state->allow_empty() and flag; });
	}

	return result;
}

// allow one of a list

struct state_choice : public state_base
{
	state_choice(const content_spec_list &allowed, bool mixed)
		: m_mixed(mixed)
		, m_state(0)
	{
		for (content_spec_ptr a : allowed)
			m_states.push_back(a->create_state());
	}

	~state_choice()
	{
		for (state_ptr s : m_states)
			s->release();
	}

	virtual std::tuple<bool, bool>
	allow(const std::string &name);

	virtual void reset()
	{
		m_state = 0;
		for (auto state : m_states)
			state->reset();
	}

	virtual bool allow_char_data() { return m_mixed; }

	virtual bool allow_empty();

	std::list<state_ptr> m_states;
	bool m_mixed;
	int m_state;
	state_ptr m_sub;
};

std::tuple<bool, bool> state_choice::allow(const std::string &name)
{
	bool result = false, done = false;

	enum State
	{
		state_Start,
		state_Choice
	};

	switch (m_state)
	{
		case state_Start:
			for (auto choice = m_states.begin(); choice != m_states.end(); ++choice)
			{
				std::tie(result, done) = (*choice)->allow(name);
				if (result == true)
				{
					m_sub = *choice;
					m_state = state_Choice;
					break;
				}
			}
			break;

		case state_Choice:
			std::tie(result, done) = m_sub->allow(name);
			break;
	}

	return std::make_tuple(result, done);
}

bool state_choice::allow_empty()
{
	return m_mixed or
	       std::find_if(m_states.begin(), m_states.end(), std::bind(&state_base::allow_empty, std::placeholders::_1)) != m_states.end();
}

// --------------------------------------------------------------------

validator::validator(content_spec_ptr allowed)
	: m_state(allowed->create_state())
	, m_allowed(allowed)
	, m_done(m_state->allow_empty())
{
}

validator::validator(const element *e)
	: m_allowed(e ? e->get_allowed() : nullptr)
{
	if (m_allowed == nullptr)
	{
		m_state = new state_any();
		m_done = true;
	}
	else
	{
		m_state = m_allowed->create_state();
		m_done = m_state->allow_empty();
	}
}

validator::~validator()
{
	m_state->release();
}

bool validator::allow(const std::string &name)
{
	bool result;
	std::tie(result, m_done) = m_state->allow(name);
	return result;
}

bool validator::done()
{
	return m_done;
}

ContentSpecType validator::get_content_spec() const
{
	return m_allowed ? m_allowed->get_content_spec() : ContentSpecType::Any;
}

// --------------------------------------------------------------------

state_ptr content_spec_any::create_state() const
{
	return new state_any();
}

// --------------------------------------------------------------------

state_ptr content_spec_empty::create_state() const
{
	return new state_empty();
}

// --------------------------------------------------------------------

state_ptr content_spec_element::create_state() const
{
	return new state_element(m_name);
}

// --------------------------------------------------------------------

content_spec_repeated::~content_spec_repeated()
{
	delete m_allowed;
}

state_ptr content_spec_repeated::create_state() const
{
	switch (m_repetition)
	{
		case '?':
			return new state_repeated_zero_or_once(m_allowed);
		case '*':
			return new state_repeated_any(m_allowed);
		case '+':
			return new state_repeated_at_least_once(m_allowed);
		default:
			assert(false);
			throw zeep::exception("illegal repetition character");
	}
}

bool content_spec_repeated::element_content() const
{
	return m_allowed->element_content();
}

// --------------------------------------------------------------------

content_spec_seq::~content_spec_seq()
{
	for (content_spec_ptr a : m_allowed)
		delete a;
}

void content_spec_seq::add(content_spec_ptr a)
{
	m_allowed.push_back(a);
}

state_ptr content_spec_seq::create_state() const
{
	return new state_seq(m_allowed);
}

bool content_spec_seq::element_content() const
{
	bool result = true;
	for (content_spec_ptr a : m_allowed)
	{
		if (not a->element_content())
		{
			result = false;
			break;
		}
	}
	return result;
}

// --------------------------------------------------------------------

content_spec_choice::~content_spec_choice()
{
	for (content_spec_ptr a : m_allowed)
		delete a;
}

void content_spec_choice::add(content_spec_ptr a)
{
	m_allowed.push_back(a);
}

state_ptr content_spec_choice::create_state() const
{
	return new state_choice(m_allowed, m_mixed);
}

bool content_spec_choice::element_content() const
{
	bool result = true;
	if (m_mixed)
		result = false;
	else
	{
		for (content_spec_ptr a : m_allowed)
		{
			if (not a->element_content())
			{
				result = false;
				break;
			}
		}
	}
	return result;
}

// --------------------------------------------------------------------

bool attribute::is_name(std::string &s) const
{
	bool result = true;

	trim(s);

	if (not s.empty())
	{
		std::string::iterator c = s.begin();

		if (c != s.end())
			result = is_name_start_char(*c);

		while (result and ++c != s.end())
			result = is_name_char(*c);
	}

	return result;
}

bool attribute::is_names(std::string &s) const
{
	bool result = true;

	trim(s);

	if (not s.empty())
	{
		std::string::iterator c = s.begin();
		std::string t;

		while (result and c != s.end())
		{
			result = is_name_start_char(*c);
			t += *c;
			++c;

			while (result and c != s.end() and is_name_char(*c))
			{
				t += *c;
				++c;
			}

			if (c == s.end())
				break;

			result = isspace(*c) != 0;
			++c;
			t += ' ';

			while (c != s.end() and isspace(*c))
				++c;
		}

		swap(s, t);
	}

	return result;
}

bool attribute::is_nmtoken(std::string &s) const
{
	trim(s);

	bool result = not s.empty();

	std::string::iterator c = s.begin();
	while (result and ++c != s.end())
		result = is_name_char(*c);

	return result;
}

bool attribute::is_nmtokens(std::string &s) const
{
	// remove leading and trailing spaces
	trim(s);

	bool result = not s.empty();

	std::string::iterator c = s.begin();
	std::string t;

	while (result and c != s.end())
	{
		result = false;

		do
		{
			if (not is_name_char(*c))
				break;
			result = true;
			t += *c;
			++c;
		} while (c != s.end());

		if (not result or c == s.end())
			break;

		result = false;
		do
		{
			if (*c != ' ')
				break;
			result = true;
			++c;
		} while (c != s.end() and *c == ' ');

		t += ' ';
	}

	if (result)
		swap(s, t);

	return result;
}

bool attribute::validate_value(std::string &value, const entity_list &entities) const
{
	bool result = true;

	if (m_type == AttributeType::CDATA)
		result = true;
	else if (m_type == AttributeType::ENTITY)
	{
		result = is_name(value);
		if (result)
			result = is_unparsed_entity(value, entities);
	}
	else if (m_type == AttributeType::ID or m_type == AttributeType::IDREF)
		result = is_name(value);
	else if (m_type == AttributeType::ENTITIES)
	{
		result = is_names(value);
		if (result)
		{
			std::vector<std::string> values;
			split(values, value, " ");
			for (const std::string &v : values)
			{
				if (not is_unparsed_entity(v, entities))
				{
					result = false;
					break;
				}
			}
		}
	}
	else if (m_type == AttributeType::IDREFS)
		result = is_names(value);
	else if (m_type == AttributeType::NMTOKEN)
		result = is_nmtoken(value);
	else if (m_type == AttributeType::NMTOKENS)
		result = is_nmtokens(value);
	else if (m_type == AttributeType::Enumerated or m_type == AttributeType::Notation)
	{
		trim(value);
		result = find(m_enum.begin(), m_enum.end(), value) != m_enum.end();
	}

	if (result and m_default == AttributeDefault::Fixed and value != m_default_value)
		result = false;

	return result;
}

bool attribute::is_unparsed_entity(const std::string &s, const entity_list &l) const
{
	bool result = false;

	entity_list::const_iterator i = std::find_if(l.begin(), l.end(), [s](auto e)
		{ return e->name() == s; });
	if (i != l.end())
		result = (*i)->is_parsed() == false;

	return result;
}

// --------------------------------------------------------------------

element::~element()
{
	for (attribute *attr : m_attlist)
		delete attr;
	delete m_allowed;
}

void element::set_allowed(content_spec_ptr allowed)
{
	if (allowed != m_allowed)
	{
		delete m_allowed;
		m_allowed = allowed;
	}
}

void element::add_attribute(attribute *attrib)
{
	std::unique_ptr<attribute> attr(attrib);
	if (find_if(m_attlist.begin(), m_attlist.end(), [attrib](auto a)
			{ return a->name() == attrib->name(); }) == m_attlist.end())
		m_attlist.push_back(attr.release());
}

const attribute *element::get_attribute(const std::string &name) const
{
	attribute_list::const_iterator dta =
		find_if(m_attlist.begin(), m_attlist.end(), [name](auto a)
			{ return a->name() == name; });

	const attribute *result = nullptr;

	if (dta != m_attlist.end())
		result = *dta;

	return result;
}

// validator element::get_validator() const
// {
// 	// validator valid;
// 	// if (m_allowed)
// 	// 	valid = validator(m_allowed);
// 	return { m_allowed };
// }

bool element::empty() const
{
	return dynamic_cast<content_spec_empty *>(m_allowed) != nullptr;
}

} // namespace mxml::doctype
