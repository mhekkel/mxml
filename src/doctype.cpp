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

#include "mxml/doctype.hpp"
#include "mxml/error.hpp"
#include "mxml/text.hpp"

#include <cassert>
#include <functional>
#include <memory>
#include <vector>

namespace mxml::doctype
{

// --------------------------------------------------------------------
// validator code

// a refcounted state base class
struct state_base : std::enable_shared_from_this<state_base>
{
	state_base() = default;

	virtual std::tuple<bool, bool> allow(const std::string &name) = 0;
	virtual bool allow_char_data() { return false; }
	virtual bool allow_empty() { return false; }
	virtual bool must_be_empty() { return false; }

	virtual void reset() {}

  protected:
	virtual ~state_base() = default;
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
	state_repeated(content_spec_base_ptr sub)
		: m_sub(sub->create_state())
		, m_state(0)
	{
	}

	virtual void reset()
	{
		m_sub->reset();
		m_state = 0;
	}

	virtual bool allow_char_data() { return m_sub->allow_char_data(); }

	state_base_ptr m_sub;
	int m_state;
};

// repeat for ?

struct state_repeated_zero_or_once : public state_repeated
{
	state_repeated_zero_or_once(content_spec_base_ptr sub)
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
		Start = 0,
		Loop
	};

	bool result = false, done = false;

	switch (m_state)
	{
		case State::Start:
			std::tie(result, done) = m_sub->allow(name);
			if (result == true)
				m_state = State::Loop;
			else
				done = true;
			break;

		case State::Loop:
			std::tie(result, done) = m_sub->allow(name);
			if (result == false and done)
				done = true;
			break;
	}

	return std::make_tuple(result, done);
}

struct state_repeated_any : public state_repeated
{
	state_repeated_any(content_spec_base_ptr sub)
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
		Start = 0,
		Loop
	};

	bool result = false, done = false;

	switch (m_state)
	{
		case State::Start:
			std::tie(result, done) = m_sub->allow(name);
			if (result == true)
				m_state = State::Loop;
			else
				done = true;
			break;

		case State::Loop:
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
	state_repeated_at_least_once(content_spec_base_ptr sub)
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
		Start = 0,
		FirstLoop,
		NextLoop
	};

	bool result = false, done = false;

	switch (m_state)
	{
		case State::Start:
			std::tie(result, done) = m_sub->allow(name);
			if (result == true)
				m_state = State::FirstLoop;
			break;

		case State::FirstLoop:
			std::tie(result, done) = m_sub->allow(name);
			if (result == false and done)
			{
				m_sub->reset();
				std::tie(result, done) = m_sub->allow(name);
				if (result == true)
					m_state = State::NextLoop;
			}
			break;

		case State::NextLoop:
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
		for (auto a : allowed)
			m_states.emplace_back(a->create_state());
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
		for (auto s : m_states)
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

	std::vector<state_base_ptr> m_states;
	std::vector<state_base_ptr>::iterator m_next;
	int m_state;
};

std::tuple<bool, bool> state_seq::allow(const std::string &name)
{
	bool result = false, done = false;

	enum State
	{
		Start,
		Element
	};

	switch (m_state)
	{
		case State::Start:
			m_next = m_states.begin();
			if (m_next == m_states.end())
			{
				done = true;
				break;
			}
			m_state = State::Element;
			// fall through

		case State::Element:
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
	bool result = true;

	for (auto s : m_states)
	{
		if (not s->allow_empty())
		{
			result = false;
			break;
		}
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
		for (auto a : allowed)
			m_states.push_back(a->create_state());
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

	std::vector<state_base_ptr> m_states;
	bool m_mixed;
	int m_state;
	state_base_ptr m_sub;
};

std::tuple<bool, bool> state_choice::allow(const std::string &name)
{
	bool result = false, done = false;

	enum State
	{
		Start,
		Choice
	};

	switch (m_state)
	{
		case State::Start:
			for (auto choice : m_states)
			{
				std::tie(result, done) = choice->allow(name);
				if (result == true)
				{
					m_sub = choice;
					m_state = State::Choice;
					break;
				}
			}
			break;

		case State::Choice:
			std::tie(result, done) = m_sub->allow(name);
			break;
	}

	return std::make_tuple(result, done);
}

bool state_choice::allow_empty()
{
	using namespace std::placeholders;
	return m_mixed or
	       std::find_if(m_states.begin(), m_states.end(), std::bind(&state_base::allow_empty, _1)) != m_states.end();
}

// --------------------------------------------------------------------

validator::validator(content_spec_base &allowed)
	: m_state(allowed.create_state())
	, m_allowed(allowed.get_content_spec())
	, m_done(m_state->allow_empty())
{
}

validator::validator(element_ptr e)
{
	if (auto allowed = e ? e->get_allowed() : nullptr; allowed != nullptr)
	{
		m_allowed = allowed->get_content_spec();
		m_state = allowed->create_state();
		m_done = m_state->allow_empty();
	}
	else
	{
		m_allowed = content_spec_type::Any;
		m_state = std::make_shared<state_any>();
		m_done = true;
	}
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

content_spec_type validator::get_content_spec() const
{
	return m_allowed;
}

// --------------------------------------------------------------------

state_base_ptr content_spec_any::create_state() const
{
	return std::make_shared<state_any>();
}

// --------------------------------------------------------------------

state_base_ptr content_spec_empty::create_state() const
{
	return std::make_shared<state_empty>();
}

// --------------------------------------------------------------------

state_base_ptr content_spec_element::create_state() const
{
	return std::make_shared<state_element>(m_name);
}

// --------------------------------------------------------------------

state_base_ptr content_spec_repeated::create_state() const
{
	switch (m_repetition)
	{
		case '?':
			return std::make_shared<state_repeated_zero_or_once>(m_allowed);
		case '*':
			return std::make_shared<state_repeated_any>(m_allowed);
		case '+':
			return std::make_shared<state_repeated_at_least_once>(m_allowed);
		default:
			assert(false);
			throw exception("illegal repetition character");
	}
}

bool content_spec_repeated::element_content() const
{
	return m_allowed->element_content();
}

// --------------------------------------------------------------------

void content_spec_seq::add(content_spec_base_ptr a)
{
	m_allowed.push_back(a);
}

state_base_ptr content_spec_seq::create_state() const
{
	return std::make_shared<state_seq>(m_allowed);
}

bool content_spec_seq::element_content() const
{
	bool result = true;
	for (auto a : m_allowed)
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

void content_spec_choice::add(content_spec_base_ptr a)
{
	m_allowed.push_back(a);
}

state_base_ptr content_spec_choice::create_state() const
{
	return std::make_shared<state_choice>(m_allowed, m_mixed);
}

bool content_spec_choice::element_content() const
{
	bool result = true;
	if (m_mixed)
		result = false;
	else
	{
		for (auto a : m_allowed)
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

	if (m_type == attribute_type::CDATA)
		result = true;
	else if (m_type == attribute_type::ENTITY)
	{
		result = is_name(value);
		if (result)
			result = is_unparsed_entity(value, entities);
	}
	else if (m_type == attribute_type::ID or m_type == attribute_type::IDREF)
		result = is_name(value);
	else if (m_type == attribute_type::ENTITIES)
	{
		result = is_names(value);
		if (result)
		{
			std::string::size_type i = 0, j = value.find(' ');
			for (;;)
			{
				if (not is_unparsed_entity(value.substr(i, j - i), entities))
				{
					result = false;
					break;
				}

				if (j == std::string::npos)
					break;

				i = j + 1;
				j = value.find(' ', i);
			}
		}
	}
	else if (m_type == attribute_type::IDREFS)
		result = is_names(value);
	else if (m_type == attribute_type::NMTOKEN)
		result = is_nmtoken(value);
	else if (m_type == attribute_type::NMTOKENS)
		result = is_nmtokens(value);
	else if (m_type == attribute_type::Enumerated or m_type == attribute_type::Notation)
	{
		trim(value);
		result = find(m_enum.begin(), m_enum.end(), value) != m_enum.end();
	}

	if (result and m_default == attribute_default::Fixed and value != m_default_value)
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

void element::set_allowed(content_spec_base_ptr allowed)
{
	m_allowed = allowed;
}

void element::add_attribute(attribute_ptr attrib)
{
	if (find_if(m_attlist.begin(), m_attlist.end(), [attrib](auto a)
			{ return a->name() == attrib->name(); }) == m_attlist.end())
		m_attlist.push_back(attrib);
}

const attribute_ptr element::get_attribute(const std::string &name) const
{
	attribute_ptr result;

	for (auto dta : m_attlist)
	{
		if (dta->name() == name)
		{
			result = dta;
			break;
		}
	}

	return result;
}

} // namespace mxml::doctype
