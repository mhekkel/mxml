// Copyright (c) 2024 Maarten L. Hekkelman
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef ZEEM_CXX_MODULE
# include "zeem/zeem.hpp"

# include <cassert>
# include <cctype>
# include <functional>
# include <memory>
# include <ranges>
# include <string_view>
# include <tuple>
# include <vector>
#endif

namespace zeem::doctype
{

// --------------------------------------------------------------------
// validator code

// a refcounted state base class
struct state_base : std::enable_shared_from_this<state_base>
{
	state_base() = default;

	virtual std::tuple<bool, bool> allow(std::string_view name) = 0;
	virtual bool allow_char_data() { return false; }
	virtual bool allow_empty() { return false; }
	virtual bool must_be_empty() { return false; }

	virtual void reset() {}

	virtual ~state_base() = default;
};

struct state_any : public state_base
{
	std::tuple<bool, bool> allow(std::string_view /*name*/) override { return std::make_tuple(true, true); }
	bool allow_char_data() override { return true; }
	bool allow_empty() override { return true; }
};

struct state_empty : public state_base
{
	std::tuple<bool, bool> allow(std::string_view /*name*/) override { return std::make_tuple(false, true); }
	bool allow_empty() override { return true; }
	bool must_be_empty() override { return true; }
};

struct state_element : public state_base
{
	explicit state_element(std::string name)
		: m_name(std::move(name))
	{
	}

	std::tuple<bool, bool> allow(std::string_view name) override
	{
		bool result = false;
		if (not m_done and m_name == name)
			m_done = result = true;
		//									m_done = true;
		return std::make_tuple(result, m_done);
	}

	void reset() override { m_done = false; }

  private:
	std::string m_name;
	bool m_done{};
};

struct state_repeated : public state_base
{
	explicit state_repeated(const content_spec_base_ptr &sub)
		: m_sub(sub->create_state())
	{
	}

	void reset() override
	{
		m_sub->reset();
		m_state = 0;
	}

	bool allow_char_data() override { return m_sub->allow_char_data(); }

  protected:
	state_base_ptr m_sub;
	int m_state{};
};

// repeat for ?

struct state_repeated_zero_or_once : public state_repeated
{
	explicit state_repeated_zero_or_once(const content_spec_base_ptr &sub)
		: state_repeated(sub)
	{
	}

	std::tuple<bool, bool> allow(std::string_view name) override;

	bool allow_empty() override { return true; }
};

std::tuple<bool, bool> state_repeated_zero_or_once::allow(std::string_view name)
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

		default:
			break;
	}

	return std::make_tuple(result, done);
}

struct state_repeated_any : public state_repeated
{
	explicit state_repeated_any(const content_spec_base_ptr &sub)
		: state_repeated(sub)
	{
	}

	std::tuple<bool, bool> allow(std::string_view name) override;

	bool allow_empty() override { return true; }
};

std::tuple<bool, bool> state_repeated_any::allow(std::string_view name)
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

		default:
			break;
	}

	return std::make_tuple(result, done);
}

struct state_repeated_at_least_once : public state_repeated
{
	explicit state_repeated_at_least_once(const content_spec_base_ptr &sub)
		: state_repeated(sub)
	{
	}

	std::tuple<bool, bool> allow(std::string_view name) override;

	bool allow_empty() override { return m_sub->allow_empty(); }
};

std::tuple<bool, bool> state_repeated_at_least_once::allow(std::string_view name)
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

		default:
			break;
	}

	return std::make_tuple(result, done);
}

// allow a sequence

struct state_seq : public state_base
{
	explicit state_seq(const content_spec_list &allowed)
	{
		for (const auto &a : allowed)
			m_states.emplace_back(a->create_state());
	}

	std::tuple<bool, bool> allow(std::string_view name) override;

	void reset() override
	{
		m_state = 0;
		for (const auto &state : m_states)
			state->reset();
	}

	bool allow_char_data() override
	{
		bool result = false;
		for (const auto &s : m_states)
		{
			if (s->allow_char_data())
			{
				result = true;
				break;
			}
		}

		return result;
	}

	bool allow_empty() override;

	std::vector<state_base_ptr> m_states;
	std::vector<state_base_ptr>::iterator m_next;
	int m_state{};
};

std::tuple<bool, bool> state_seq::allow(std::string_view name)
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

		default:
			break;
	}

	return std::make_tuple(result, done);
}

bool state_seq::allow_empty()
{
	bool result = true;

	for (const auto &s : m_states)
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
	{
		for (const auto &a : allowed)
			m_states.push_back(a->create_state());
	}

	std::tuple<bool, bool> allow(std::string_view name) override;

	void reset() override
	{
		m_state = 0;
		for (const auto &state : m_states)
			state->reset();
	}

	bool allow_char_data() override { return m_mixed; }

	bool allow_empty() override;

	std::vector<state_base_ptr> m_states;
	bool m_mixed;
	int m_state{};
	state_base_ptr m_sub;
};

std::tuple<bool, bool> state_choice::allow(std::string_view name)
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
			for (const auto &choice : m_states)
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

		default:
			break;
	}

	return std::make_tuple(result, done);
}

bool state_choice::allow_empty()
{
	using namespace std::placeholders;
	return m_mixed or
	       std::ranges::find_if(m_states, [](auto &&s)
			   { return s->allow_empty(); }) != m_states.end();
}

// --------------------------------------------------------------------

validator::validator(content_spec_base &allowed)
	: m_state(allowed.create_state())
	, m_allowed(allowed.get_content_spec())
	, m_done(m_state->allow_empty())
{
}

validator::validator(const element_ptr &e)
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

bool validator::allow(std::string_view name)
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
	m_allowed.emplace_back(std::move(a));
}

state_base_ptr content_spec_seq::create_state() const
{
	return std::make_shared<state_seq>(m_allowed);
}

bool content_spec_seq::element_content() const
{
	bool result = true;
	for (const auto &a : m_allowed)
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
	m_allowed.emplace_back(std::move(a));
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
		for (const auto &a : m_allowed)
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

		while (c != s.end())
		{
			if (not is_name_char(*c))
				break;
			result = true;
			t += *c;
			++c;
		}

		if (not result or c == s.end())
			break;

		result = false;
		while (c != s.end() and *c == ' ')
		{
			result = true;
			++c;
		}

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
		result = std::ranges::find(m_enum, value) != m_enum.end();
	}

	if (result and m_default == attribute_default::Fixed and value != m_default_value)
		result = false;

	return result;
}

bool attribute::is_unparsed_entity(std::string_view s, const entity_list &l) const
{
	bool result = false;

	auto i = std::ranges::find_if(l, [s](const auto &e)
		{ return e->name() == s; });
	if (i != l.end())
		result = (*i)->is_parsed() == false;

	return result;
}

// --------------------------------------------------------------------

void element::set_allowed(content_spec_base_ptr allowed)
{
	m_allowed = std::move(allowed);
}

void element::add_attribute(attribute_ptr attrib)
{
	if (std::ranges::find_if(m_attlist, [attrib](const auto &a)
			{ return a->name() == attrib->name(); }) == m_attlist.end())
		m_attlist.emplace_back(std::move(attrib));
}

const attribute_ptr element::get_attribute(std::string_view name) const
{
	attribute_ptr result;

	for (const auto &dta : m_attlist)
	{
		if (dta->name() == name)
		{
			result = dta;
			break;
		}
	}

	return result;
}

} // namespace zeem::doctype
