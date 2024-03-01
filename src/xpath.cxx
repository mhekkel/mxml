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

#include <algorithm>
#include <charconv>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <sstream>

#include <cmath>

module mxml;

import :document;
import :error;
import :node;
import :xpath;

namespace mxml
{

// --------------------------------------------------------------------

enum class Token
{
	Undef = 0,
	Eof = 256,
	LeftParenthesis,
	RightParenthesis,
	LeftBracket,
	RightBracket,
	Slash,
	DoubleSlash,
	Comma,
	Name,

	AxisSpec,
	FunctionName,
	NodeType,

	OperatorUnion,
	OperatorAdd,
	OperatorSubstract,
	OperatorEqual,
	OperatorNotEqual,
	OperatorLess,
	OperatorLessOrEqual,
	OperatorGreater,
	OperatorGreaterOrEqual,

	// next four operators are pseudo tokens, i.e. they are returned as Name from get_next_token
	OperatorAnd,
	OperatorOr,
	OperatorMod,
	OperatorDiv,

	Literal,
	Number,
	Variable,
	Asterisk,
	Colon,
};

enum class AxisType
{
	Ancestor,
	AncestorOrSelf,
	Attribute,
	Child,
	Descendant,
	DescendantOrSelf,
	Following,
	FollowingSibling,
	Namespace,
	Parent,
	Preceding,
	PrecedingSibling,
	Self,

	AxisTypeCount
};

const char *kAxisNames[static_cast<size_t>(AxisType::AxisTypeCount)] = {
	"ancestor",
	"ancestor-or-self",
	"attribute",
	"child",
	"descendant",
	"descendant-or-self",
	"following",
	"following-sibling",
	"namespace",
	"parent",
	"preceding",
	"preceding-sibling",
	"self"
};

enum class CoreFunction
{
	Last,
	Position,
	Count,
	Id,
	LocalName,
	NamespaceUri,
	Name,
	String,
	Concat,
	StartsWith,
	Contains,
	SubstringBefore,
	SubstringAfter,
	Substring,
	StringLength,
	NormalizeSpace,
	Translate,
	Boolean,
	Not,
	True,
	False,
	Lang,
	Number,
	Sum,
	Floor,
	Ceiling,
	Round,
	Comment,

	CoreFunctionCount
};

const size_t kCoreFunctionCount = static_cast<size_t>(CoreFunction::CoreFunctionCount);

struct CoreFunctionInfo
{
	const char *name;
	int arg_count;
};

const int kOptionalArgument = -100;

const CoreFunctionInfo kCoreFunctionInfo[kCoreFunctionCount] = {
	{ "last", 0 },
	{ "position", 0 },
	{ "count", 1 },
	{ "id", 0 },
	{ "local-name", kOptionalArgument },
	{ "namespace-uri", kOptionalArgument },
	{ "name", kOptionalArgument },
	{ "string", kOptionalArgument },
	{ "concat", -2 },
	{ "starts-with", 2 },
	{ "contains", 2 },
	{ "substring-before", 2 },
	{ "substring-after", 2 },
	{ "substring", -2 },
	{ "string-length", kOptionalArgument },
	{ "normalize-space", kOptionalArgument },
	{ "translate", 3 },
	{ "boolean", 1 },
	{ "not", 1 },
	{ "true", 0 },
	{ "false", 0 },
	{ "lang", 0 },
	{ "number", kOptionalArgument },
	{ "sum", 0 },
	{ "floor", 1 },
	{ "ceiling", 1 },
	{ "round", 1 },
	{ "comment", 1 },
};

// the expressions are implemented as interpreter objects
// they return 'objects' that can hold various data.

enum class object_type
{
	undef,
	node_set,
	boolean,
	number,
	string
};

class object
{
  public:
	object();
	object(node_set ns);
	object(bool b);
	object(double n);
	object(const std::string &s);
	object(const object &o);
	object &operator=(const object &o);

	bool operator==(const object o);
	bool operator<(const object o);

	object_type type() const { return m_type; }

	template <typename T>
	T as() const;

  private:
	object_type m_type;
	node_set m_node_set;
	bool m_boolean;
	double m_number;
	std::string m_string;
};

object operator%(const object &lhs, const object &rhs);
object operator/(const object &lhs, const object &rhs);
object operator+(const object &lhs, const object &rhs);
object operator-(const object &lhs, const object &rhs);

object::object()
	: m_type(object_type::undef)
{
}

object::object(node_set ns)
	: m_type(object_type::node_set)
	, m_node_set(ns)
{
}

object::object(bool b)
	: m_type(object_type::boolean)
	, m_boolean(b)
{
}

object::object(double n)
	: m_type(object_type::number)
	, m_number(n)
{
}

object::object(const std::string &s)
	: m_type(object_type::string)
	, m_string(s)
{
}

object::object(const object &o)
	: m_type(o.m_type)
{
	switch (m_type)
	{
		case object_type::node_set: m_node_set = o.m_node_set; break;
		case object_type::boolean: m_boolean = o.m_boolean; break;
		case object_type::number: m_number = o.m_number; break;
		case object_type::string: m_string = o.m_string; break;
		default: break;
	}
}

object &object::operator=(const object &o)
{
	m_type = o.m_type;
	switch (m_type)
	{
		case object_type::node_set: m_node_set = o.m_node_set; break;
		case object_type::boolean: m_boolean = o.m_boolean; break;
		case object_type::number: m_number = o.m_number; break;
		case object_type::string: m_string = o.m_string; break;
		default: break;
	}
	return *this;
}

template <>
const node_set &object::as<const node_set &>() const
{
	if (m_type != object_type::node_set)
		throw exception("object is not of type node-set");
	return m_node_set;
}

template <>
bool object::as<bool>() const
{
	bool result;
	switch (m_type)
	{
		case object_type::number: result = m_number != 0 and not std::isnan(m_number); break;
		case object_type::node_set: result = not m_node_set.empty(); break;
		case object_type::string: result = not m_string.empty(); break;
		case object_type::boolean: result = m_boolean; break;
		default: result = false; break;
	}

	return result;
}

template <>
double object::as<double>() const
{
	double result;
	switch (m_type)
	{
		case object_type::number: result = m_number; break;
		case object_type::node_set:
		{
			if (m_node_set.empty())
				result = std::nan("1");
			else
			{
				auto s = m_node_set.front()->str();
				if (auto r = std::from_chars(s.data(), s.data() + s.size(), result); r.ec != std::errc{})
					result = std::nan("1");
			}
			break;
		}
		case object_type::string:
		{
			if (auto r = std::from_chars(m_string.data(), m_string.data() + m_string.size(), result); r.ec != std::errc{})
				result = std::nan("1");
			break;
		}
		case object_type::boolean: result = m_boolean; break;
		default: result = 0; break;
	}
	return result;
}

template <>
int object::as<int>() const
{
	if (m_type != object_type::number)
		throw exception("object is not of type number");
	return static_cast<int>(round(m_number));
}

template <>
const std::string &object::as<const std::string &>() const
{
	if (m_type != object_type::string)
		throw exception("object is not of type string");
	return m_string;
}

template <>
std::string object::as<std::string>() const
{
	std::string result;

	switch (m_type)
	{
		case object_type::number: result = std::to_string(m_number); break;
		case object_type::string: result = m_string; break;
		case object_type::boolean: result = (m_boolean ? "true" : "false"); break;
		case object_type::node_set:
			for (auto &n : m_node_set)
				result += n->str();
			break;
		default: break;
	}

	return result;
}

bool object::operator==(const object o)
{
	bool result = false;

	if (m_type == o.m_type)
	{
		switch (m_type)
		{
			case object_type::node_set: result = m_node_set == o.m_node_set; break;
			case object_type::boolean: result = m_boolean == o.m_boolean; break;
			case object_type::number: result = m_number == o.m_number; break;
			case object_type::string: result = m_string == o.m_string; break;
			default: break;
		}
	}
	else
	{
		if (m_type == object_type::number or o.m_type == object_type::number)
			result = as<double>() == o.as<double>();
		else if (m_type == object_type::string or o.m_type == object_type::string)
			result = as<std::string>() == o.as<std::string>();
		else if (m_type == object_type::boolean or o.m_type == object_type::boolean)
			result = as<bool>() == o.as<bool>();
	}

	return result;
}

bool object::operator<(const object o)
{
	bool result = false;
	switch (m_type)
	{
		case object_type::node_set: result = m_node_set < o.m_node_set; break;
		case object_type::boolean: result = m_boolean < o.m_boolean; break;
		case object_type::number: result = m_number < o.m_number; break;
		case object_type::string: result = m_string < o.m_string; break;
		default: break;
	}
	return result;
}

std::ostream &operator<<(std::ostream &lhs, object &rhs)
{
	switch (rhs.type())
	{
		case object_type::undef: lhs << "undef()"; break;
		case object_type::number: lhs << "number(" << rhs.as<double>() << ')'; break;
		case object_type::string: lhs << "string(" << rhs.as<std::string>() << ')'; break;
		case object_type::boolean: lhs << "boolean(" << (rhs.as<bool>() ? "true" : "false") << ')'; break;
		case object_type::node_set: lhs << "node_set(#" << rhs.as<const node_set &>().size() << ')'; break;
	}

	return lhs;
}

// --------------------------------------------------------------------
// visiting (or better, collecting) other nodes in the hierarchy is done here.

using context_node = node_list<element>;

template <typename PREDICATE>
void iterate_child_elements(context_node *context, node_set &s, bool deep, PREDICATE pred)
{
	for (element &child : *context)
	{
		if (find(s.begin(), s.end(), &child) != s.end())
			continue;

		if (pred(&child))
			s.push_back(&child);

		if (deep)
			iterate_child_elements(&child, s, true, pred);
	}
}

template <typename PREDICATE>
void iterate_child_nodes(context_node *context, node_set &s, bool deep, PREDICATE pred)
{
	for (node &child : node_list<>(*context))
	{
		if (find(s.begin(), s.end(), &child) != s.end())
			continue;

		if (pred(&child))
			s.push_back(&child);

		if (deep)
		{
			context_node *child_element = dynamic_cast<context_node *>(&child);
			if (child_element != nullptr)
				iterate_child_nodes(child_element, s, true, pred);
		}
	}
}

template <typename PREDICATE>
inline void iterate_children(context_node *context, node_set &s, bool deep, PREDICATE pred, bool elementsOnly)
{
	if (elementsOnly)
		iterate_child_elements(context, s, deep, pred);
	else
		iterate_child_nodes(context, s, deep, pred);
}

template <typename PREDICATE>
void iterate_ancestor(context_node *e, node_set &s, PREDICATE pred)
{
	auto n = dynamic_cast<node *>(e)->parent();
	while (n != nullptr and n->type() != node_type::document)
	{
		if (pred(n))
			s.push_back(n);
		n = n->parent();
	}
}

template <typename PREDICATE>
void iterate_preceding(node *n, node_set &s, bool sibling, PREDICATE pred, bool elementsOnly)
{
	while (n != nullptr and n->type() != node_type::document)
	{
		if (n->prev()->type() == node_type::header)
		{
			if (sibling)
				break;

			n = n->parent();
			continue;
		}

		n = n->prev();

		context_node *e = dynamic_cast<context_node *>(n);
		if (e == nullptr)
			continue;

		if (pred(n))
			s.push_back(n);

		if (sibling == false)
			iterate_children(e, s, true, pred, elementsOnly);
	}
}

template <typename PREDICATE>
void iterate_following(node *n, node_set &s, bool sibling, PREDICATE pred, bool elementsOnly)
{
	while (n != nullptr and n->type() != node_type::document)
	{
		if (n->next()->type() == node_type::header)
		{
			if (sibling)
				break;

			n = n->parent();
			continue;
		}

		n = n->next();

		context_node *e = dynamic_cast<context_node *>(n);
		if (e == nullptr)
			continue;

		if (pred(n))
			s.push_back(n);

		if (sibling == false)
			iterate_children(e, s, true, pred, elementsOnly);
	}
}

template <typename PREDICATE>
void iterate_attributes(context_node *e, node_set &s, PREDICATE pred)
{
	auto el = dynamic_cast<element *>(e);
	if (el != nullptr)
	{
		for (auto &a : el->attributes())
		{
			if (pred(&a))
				s.push_back(&a);
		}
	}
}

template <typename PREDICATE>
void iterate_namespaces(context_node *e, node_set &s, PREDICATE pred)
{
	auto el = dynamic_cast<element *>(e);
	if (el != nullptr)
	{
		for (auto &a : el->attributes())
		{
			if (not a.is_namespace())
				continue;

			if (pred(&a))
				s.push_back(&a);
		}
	}
}

// --------------------------------------------------------------------
// context for the expressions
// Need to add support for external variables here.

struct context_imp
{
	virtual ~context_imp() {}

	virtual object &get(const std::string &name)
	{
		return m_variables[name];
	}

	virtual void set(const std::string &name, const object &value)
	{
		m_variables[name] = value;
	}

	std::map<std::string, object> m_variables;
};

struct expression_context : public context_imp
{
	expression_context(context_imp &next, node *n, const node_set &s)
		: m_next(next)
		, m_node(n)
		, m_node_set(s)
	{
	}

	virtual object &get(const std::string &name)
	{
		return m_next.get(name);
	}

	virtual void set(const std::string &name, const object &value)
	{
		m_next.set(name, value);
	}

	void dump();

	size_t position() const;
	size_t last() const;

	context_imp &m_next;
	node *m_node;
	const node_set &m_node_set;
};

size_t expression_context::position() const
{
	size_t result = 0;
	for (const node *n : m_node_set)
	{
		++result;
		if (n == m_node)
			break;
	}

	if (result == 0)
		throw exception("invalid context for position");

	return result;
}

size_t expression_context::last() const
{
	return m_node_set.size();
}

void expression_context::dump()
{
	// std::cout << "context node: " << *m_node << '\n'
	// 	 << "context node-set: ";
	// copy(m_node_set.begin(), m_node_set.end(), std::ostream_iterator<node*>(std::cout, ", "));
	// std::cout << '\n';
}

std::ostream &operator<<(std::ostream &lhs, expression_context &rhs)
{
	rhs.dump();
	return lhs;
}

void indent(int level)
{
	while (level-- > 0)
		std::cout << ' ';
}

// --------------------------------------------------------------------

class expression
{
  public:
	virtual ~expression() {}
	virtual object evaluate(expression_context &context) = 0;

	// 					// print exists only for debugging purposes
	// virtual void		print(int level) = 0;
};

typedef std::shared_ptr<expression> expression_ptr;
typedef std::list<expression_ptr> expression_list;

// needed for CLang/libc++ on FreeBSD 10
expression *get_pointer(std::shared_ptr<expression> const &p)
{
	return p.get();
}

// --------------------------------------------------------------------

class step_expression : public expression
{
  public:
	step_expression(AxisType axis)
		: m_axis(axis)
	{
	}

  protected:
	template <typename T>
	object evaluate(expression_context &context, T pred, bool elementsOnly);

	AxisType m_axis;
};

template <typename T>
object step_expression::evaluate(expression_context &context, T pred, bool elementsOnly)
{
	node_set result;

	context_node *context_element = dynamic_cast<context_node *>(context.m_node);
	if (context_element != nullptr)
	{
		switch (m_axis)
		{
			case AxisType::Parent:
			{
				auto p = context.m_node->parent();
				if (p != nullptr and pred(p))
					result.push_back(p);
				break;
			}

			case AxisType::Ancestor:
				iterate_ancestor(context_element, result, pred);
				break;

			case AxisType::AncestorOrSelf:
				if (pred(context.m_node))
					result.push_back(context.m_node);
				iterate_ancestor(context_element, result, pred);
				break;

			case AxisType::Self:
				if (pred(context.m_node))
					result.push_back(context.m_node);
				break;

			case AxisType::Child:
				iterate_children(context_element, result, false, pred, elementsOnly);
				break;

			case AxisType::Descendant:
				iterate_children(context_element, result, true, pred, elementsOnly);
				break;

			case AxisType::DescendantOrSelf:
				if (pred(context.m_node))
					result.push_back(context.m_node);
				iterate_children(context_element, result, true, pred, elementsOnly);
				break;

			case AxisType::Following:
				iterate_following(context.m_node, result, false, pred, elementsOnly);
				break;

			case AxisType::FollowingSibling:
				iterate_following(context.m_node, result, true, pred, elementsOnly);
				break;

			case AxisType::Preceding:
				iterate_preceding(context.m_node, result, false, pred, elementsOnly);
				break;

			case AxisType::PrecedingSibling:
				iterate_preceding(context.m_node, result, true, pred, elementsOnly);
				break;

			case AxisType::Attribute:
				if (auto c = dynamic_cast<context_node *>(context_element); c != nullptr)
					iterate_attributes(c, result, pred);
				break;

			case AxisType::Namespace:
				if (auto c = dynamic_cast<context_node *>(context_element); c != nullptr)
					iterate_namespaces(c, result, pred);
				break;

			case AxisType::AxisTypeCount:;
		}
	}

	return result;
}

// --------------------------------------------------------------------

class name_test_step_expression : public step_expression
{
  public:
	name_test_step_expression(AxisType axis, const std::string &name)
		: step_expression(axis)
		, m_name(name)
	{
		m_test = std::bind(&name_test_step_expression::name_matches, this, std::placeholders::_1);
	}

	virtual object evaluate(expression_context &context);

	// virtual void		print(int level) { indent(level); std::cout << "name test step " << m_name << '\n'; }

  protected:
	bool name_matches(const node *n)
	{
		bool result = m_name == "*" or n->name() == m_name;
		return result;
	}

	std::string m_name;
	std::function<bool(const node *)> m_test;
};

object name_test_step_expression::evaluate(expression_context &context)
{
	return step_expression::evaluate(context, m_test, true);
}

// --------------------------------------------------------------------

template <typename T>
class node_type_expression : public step_expression
{
  public:
	node_type_expression(AxisType axis)
		: step_expression(axis)
	{
		m_test = std::bind(&node_type_expression::test, std::placeholders::_1);
	}

	virtual object evaluate(expression_context &context);

	// virtual void		print(int level) { indent(level); std::cout << "node type step " << boost::core::demangle(typeid(T).name()) << '\n'; }

  private:
	static bool test(const node *n) { return typeid(*n) == typeid(T); }

	std::function<bool(const node *)> m_test;
};

template <>
bool node_type_expression<node>::test(const node *n)
{
	return true;
}

template <>
bool node_type_expression<text>::test(const node *n)
{
	return n->type() == node_type::text or n->type() == node_type::cdata;
}

template <typename T>
object node_type_expression<T>::evaluate(expression_context &context)
{
	return step_expression::evaluate(context, m_test, false);
}

// --------------------------------------------------------------------

class root_expression : public expression
{
  public:
	virtual object evaluate(expression_context &context);

	// virtual void		print(int level) { indent(level); std::cout << "root\n"; }
};

object root_expression::evaluate(expression_context &context)
{
	node_set result;
	result.push_back(context.m_node->root());
	return result;
}

// --------------------------------------------------------------------

template <Token OP>
class operator_expression : public expression
{
  public:
	operator_expression(expression_ptr lhs, expression_ptr rhs)
		: m_lhs(lhs)
		, m_rhs(rhs)
	{
	}

	virtual object evaluate(expression_context &context);

	// virtual void		print(int level)
	// 					{
	// 						indent(level);
	// 						std::cout << "operator " << boost::core::demangle(typeid(OP).name()) << '\n';
	// 						m_lhs->print(level + 1);
	// 						m_rhs->print(level + 1);
	// 					}

  private:
	expression_ptr m_lhs, m_rhs;
};

template <>
object operator_expression<Token::OperatorAdd>::evaluate(expression_context &context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);

	return v1.as<double>() + v2.as<double>();
}

template <>
object operator_expression<Token::OperatorSubstract>::evaluate(expression_context &context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);

	return v1.as<double>() - v2.as<double>();
}

template <>
object operator_expression<Token::OperatorEqual>::evaluate(expression_context &context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);

	return v1 == v2;
}

template <>
object operator_expression<Token::OperatorNotEqual>::evaluate(expression_context &context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);

	return not(v1 == v2);
}

template <>
object operator_expression<Token::OperatorLess>::evaluate(expression_context &context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);

	return v1 < v2;
}

template <>
object operator_expression<Token::OperatorLessOrEqual>::evaluate(expression_context &context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);

	return v1 < v2 or v1 == v2;
}

template <>
object operator_expression<Token::OperatorGreater>::evaluate(expression_context &context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);

	return v2 < v1;
}

template <>
object operator_expression<Token::OperatorGreaterOrEqual>::evaluate(expression_context &context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);

	return v2 < v1 or v1 == v2;
}

template <>
object operator_expression<Token::OperatorAnd>::evaluate(expression_context &context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);

	return v1.as<bool>() and v2.as<bool>();
}

template <>
object operator_expression<Token::OperatorOr>::evaluate(expression_context &context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);

	return v1.as<bool>() or v2.as<bool>();
}

template <>
object operator_expression<Token::OperatorMod>::evaluate(expression_context &context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);

	return double(v1.as<int>() % v2.as<int>());
}

template <>
object operator_expression<Token::OperatorDiv>::evaluate(expression_context &context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);

	return v1.as<double>() / v2.as<double>();
}

template <>
object operator_expression<Token::Asterisk>::evaluate(expression_context &context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);

	return v1.as<double>() * v2.as<double>();
}

// --------------------------------------------------------------------

class negate_expression : public expression
{
  public:
	negate_expression(expression_ptr expr)
		: m_expr(expr)
	{
	}

	virtual object evaluate(expression_context &context);

	// virtual void		print(int level) { indent(level); std::cout << "negate\n"; m_expr->print(level + 1); }

  private:
	expression_ptr m_expr;
};

object negate_expression::evaluate(expression_context &context)
{
	object v = m_expr->evaluate(context);
	return -v.as<double>();
}

// --------------------------------------------------------------------

class path_expression : public expression
{
  public:
	path_expression(expression_ptr lhs, expression_ptr rhs)
		: m_lhs(lhs)
		, m_rhs(rhs)
	{
	}

	virtual object evaluate(expression_context &context);

	// virtual void		print(int level)
	// 					{
	// 						indent(level);
	// 						std::cout << "path\n";
	// 						m_lhs->print(level + 1);
	// 						m_rhs->print(level + 1);
	// 					}

  private:
	expression_ptr m_lhs, m_rhs;
};

object path_expression::evaluate(expression_context &context)
{
	object v = m_lhs->evaluate(context);
	if (v.type() != object_type::node_set)
		throw exception("filter does not evaluate to a node-set");

	node_set result;
	for (node *n : v.as<const node_set &>())
	{
		expression_context ctxt(context, n, v.as<const node_set &>());

		node_set s = m_rhs->evaluate(ctxt).as<const node_set &>();

		copy(s.begin(), s.end(), back_inserter(result));
	}

	return result;
}

// --------------------------------------------------------------------

class predicate_expression : public expression
{
  public:
	predicate_expression(expression_ptr path, expression_ptr pred)
		: m_path(path)
		, m_pred(pred)
	{
	}

	virtual object evaluate(expression_context &context);

	// virtual void		print(int level)
	// 					{
	// 						indent(level);
	// 						std::cout << "predicate\n";
	// 						m_path->print(level + 1);
	// 						m_pred->print(level + 1);
	// 					}

  private:
	expression_ptr m_path, m_pred;
};

object predicate_expression::evaluate(expression_context &context)
{
	object v = m_path->evaluate(context);

	node_set result;

	for (node *n : v.as<const node_set &>())
	{
		expression_context ctxt(context, n, v.as<const node_set &>());

		object test = m_pred->evaluate(ctxt);

		if (test.type() == object_type::number)
		{
			if (ctxt.position() == test.as<double>())
				result.push_back(n);
		}
		else if (test.as<bool>())
			result.push_back(n);
	}

	return result;
}

// --------------------------------------------------------------------

class variable_expression : public expression
{
  public:
	variable_expression(const std::string &name)
		: m_var(name)
	{
	}

	virtual object evaluate(expression_context &context);

	// virtual void		print(int level) { indent(level); std::cout << "variable " << m_var << '\n'; }

  private:
	std::string m_var;
};

object variable_expression::evaluate(expression_context &context)
{
	return context.get(m_var);
}

// --------------------------------------------------------------------

class literal_expression : public expression
{
  public:
	literal_expression(const std::string &lit)
		: m_lit(lit)
	{
	}

	virtual object evaluate(expression_context &context);

	// virtual void		print(int level) { indent(level); std::cout << "literal " << m_lit << '\n'; }

  private:
	std::string m_lit;
};

object literal_expression::evaluate(expression_context & /*context*/)
{
	return object(m_lit);
}

// --------------------------------------------------------------------

class number_expression : public expression
{
  public:
	number_expression(double number)
		: m_number(number)
	{
	}

	virtual object evaluate(expression_context &context);

	// virtual void		print(int level) { indent(level); std::cout << "number " << m_number << '\n'; }

  private:
	double m_number;
};

object number_expression::evaluate(expression_context & /*context*/)
{
	return object(m_number);
}

// --------------------------------------------------------------------

template <CoreFunction CF>
class core_function_expression : public expression
{
  public:
	core_function_expression(expression_list &arguments)
		: m_args(arguments)
	{
	}

	virtual object evaluate(expression_context &context);

	// virtual void		print(int level)
	// 					{
	// 						indent(level);
	// 						std::cout << "function call " << boost::core::demangle(typeid(CF).name()) << '\n';
	// 						for_each(m_args.begin(), m_args.end(),
	// 							std::bind(&expression::print, std::placeholders::_1, level + 1));
	// 					}

  private:
	expression_list m_args;
};

template <CoreFunction CF>
object core_function_expression<CF>::evaluate(expression_context & /*context*/)
{
	throw exception("unimplemented function ");
}

template <>
object core_function_expression<CoreFunction::Position>::evaluate(expression_context &context)
{
	return object(double(context.position()));
}

template <>
object core_function_expression<CoreFunction::Last>::evaluate(expression_context &context)
{
	return object(double(context.last()));
}

template <>
object core_function_expression<CoreFunction::Count>::evaluate(expression_context &context)
{
	object v = m_args.front()->evaluate(context);
	size_t result = v.as<const node_set &>().size();

	return object(double(result));
}

template <>
object core_function_expression<CoreFunction::Id>::evaluate(expression_context &context)
{
	element *e = nullptr;

	if (m_args.empty())
		e = dynamic_cast<element *>(context.m_node);
	else
	{
		object v = m_args.front()->evaluate(context);
		if (not v.as<const node_set &>().empty())
			e = dynamic_cast<element *>(v.as<const node_set &>().front());
	}

	if (e == nullptr)
		throw exception("argument is not an element in function 'id()'");

	return e->id();
}

template <>
object core_function_expression<CoreFunction::LocalName>::evaluate(expression_context &context)
{
	node *n = nullptr;

	if (m_args.empty())
		n = context.m_node;
	else
	{
		object v = m_args.front()->evaluate(context);
		if (not v.as<const node_set &>().empty())
			n = v.as<const node_set &>().front();
	}

	if (n == nullptr)
		throw exception("argument is not an element in function 'local-name'");

	return n->name();
}

template <>
object core_function_expression<CoreFunction::NamespaceUri>::evaluate(expression_context &context)
{
	node *n = nullptr;

	if (m_args.empty())
		n = context.m_node;
	else
	{
		object v = m_args.front()->evaluate(context);
		if (not v.as<const node_set &>().empty())
			n = v.as<const node_set &>().front();
	}

	if (n == nullptr)
		throw exception("argument is not an element in function 'namespace-uri'");

	return n->get_ns();
}

template <>
object core_function_expression<CoreFunction::Name>::evaluate(expression_context &context)
{
	node *n = nullptr;

	if (m_args.empty())
		n = context.m_node;
	else
	{
		object v = m_args.front()->evaluate(context);
		if (not v.as<const node_set &>().empty())
			n = v.as<const node_set &>().front();
	}

	if (n == nullptr)
		throw exception("argument is not an element in function 'name'");

	return n->get_qname();
}

template <>
object core_function_expression<CoreFunction::String>::evaluate(expression_context &context)
{
	std::string result;

	if (m_args.empty())
		result = context.m_node->str();
	else
	{
		object v = m_args.front()->evaluate(context);
		result = v.as<std::string>();
	}

	return result;
}

template <>
object core_function_expression<CoreFunction::Concat>::evaluate(expression_context &context)
{
	std::string result;
	for (expression_ptr &e : m_args)
	{
		object v = e->evaluate(context);
		result += v.as<std::string>();
	}
	return result;
}

template <>
object core_function_expression<CoreFunction::StringLength>::evaluate(expression_context &context)
{
	std::string result;

	if (m_args.empty())
		result = context.m_node->str();
	else
	{
		object v = m_args.front()->evaluate(context);
		result = v.as<std::string>();
	}

	return double(result.length());
}

template <>
object core_function_expression<CoreFunction::StartsWith>::evaluate(expression_context &context)
{
	object v1 = m_args.front()->evaluate(context);
	object v2 = m_args.back()->evaluate(context);

	try
	{
		return v2.as<std::string>().empty() or
		       v1.as<std::string>().starts_with(v2.as<std::string>());
	}
	catch (const std::exception &)
	{
		throw exception("expected two strings as argument for starts-with");
	}
}

template <>
object core_function_expression<CoreFunction::Contains>::evaluate(expression_context &context)
{
	object v1 = m_args.front()->evaluate(context);
	object v2 = m_args.back()->evaluate(context);

	try
	{
		auto s1 = v1.as<std::string>();
		auto s2 = v2.as<std::string>();

		return s1.find(s2) != std::string::npos;
	}
	catch (...)
	{
		throw exception("expected two strings as argument for contains");
	}
}

template <>
object core_function_expression<CoreFunction::SubstringBefore>::evaluate(expression_context &context)
{
	object v1 = m_args.front()->evaluate(context);
	object v2 = m_args.back()->evaluate(context);

	try
	{
		std::string result;
		if (not v2.as<std::string>().empty())
		{
			std::string::size_type p = v1.as<std::string>().find(v2.as<std::string>());
			if (p != std::string::npos)
				result = v1.as<std::string>().substr(0, p);
		}

		return result;
	}
	catch (...)
	{
		throw exception("expected two strings as argument for substring-before");
	}
}

template <>
object core_function_expression<CoreFunction::SubstringAfter>::evaluate(expression_context &context)
{
	object v1 = m_args.front()->evaluate(context);
	object v2 = m_args.back()->evaluate(context);

	try
	{
		std::string result;
		if (v2.as<std::string>().empty())
			result = v1.as<std::string>();
		else
		{
			std::string::size_type p = v1.as<std::string>().find(v2.as<std::string>());
			if (p != std::string::npos and p + v2.as<std::string>().length() < v1.as<std::string>().length())
				result = v1.as<std::string>().substr(p + v2.as<std::string>().length());
		}

		return result;
	}
	catch (...)
	{
		throw exception("expected two strings as argument for substring-after");
	}
}

template <>
object core_function_expression<CoreFunction::Substring>::evaluate(expression_context &context)
{
	expression_list::iterator a = m_args.begin();

	object v1 = (*a)->evaluate(context);
	++a;
	object v2 = (*a)->evaluate(context);
	++a;
	object v3 = (*a)->evaluate(context);

	if (v2.type() != object_type::number or v3.type() != object_type::number)
		throw exception("expected one string and two numbers as argument for substring");

	try
	{
		return v1.as<std::string>().substr(v2.as<int>() - 1, v3.as<int>());
	}
	catch (...)
	{
		throw exception("expected one string and two numbers as argument for substring");
	}
}

template <>
object core_function_expression<CoreFunction::NormalizeSpace>::evaluate(expression_context &context)
{
	std::string s;

	if (m_args.empty())
		s = context.m_node->str();
	else
	{
		object v = m_args.front()->evaluate(context);
		s = v.as<std::string>();
	}

	std::string result;
	bool space = true;

	for (char c : s)
	{
		if (isspace(c))
		{
			if (not space)
				result += ' ';
			space = true;
		}
		else
		{
			result += c;
			space = false;
		}
	}

	if (not result.empty() and space)
		result.erase(result.end() - 1);

	return result;
}

template <>
object core_function_expression<CoreFunction::Translate>::evaluate(expression_context &context)
{
	expression_list::iterator a = m_args.begin();

	object v1 = (*a)->evaluate(context);
	++a;
	object v2 = (*a)->evaluate(context);
	++a;
	object v3 = (*a)->evaluate(context);

	try
	{
		const std::string &f = v2.as<const std::string &>();
		const std::string &r = v3.as<const std::string &>();

		std::string result;
		result.reserve(v1.as<std::string>().length());
		for (char c : v1.as<std::string>())
		{
			std::string::size_type fi = f.find(c);
			if (fi == std::string::npos)
				result += c;
			else if (fi < r.length())
				result += r[fi];
		}

		return result;
	}
	catch (const std::exception &)
	{
		throw exception("expected three strings as arguments for translate");
	}
}

template <>
object core_function_expression<CoreFunction::Boolean>::evaluate(expression_context &context)
{
	object v = m_args.front()->evaluate(context);
	return v.as<bool>();
}

template <>
object core_function_expression<CoreFunction::Not>::evaluate(expression_context &context)
{
	object v = m_args.front()->evaluate(context);
	return not v.as<bool>();
}

template <>
object core_function_expression<CoreFunction::True>::evaluate(expression_context & /*context*/)
{
	return true;
}

template <>
object core_function_expression<CoreFunction::False>::evaluate(expression_context & /*context*/)
{
	return false;
}

template <>
object core_function_expression<CoreFunction::Lang>::evaluate(expression_context &context)
{
	object v = m_args.front()->evaluate(context);

	std::string test = v.as<std::string>();
	for (auto &ch : test)
		ch = std::tolower(ch);

	std::string lang = context.m_node->lang();
	for (auto &ch : lang)
		ch = std::tolower(ch);

	bool result = test == lang;

	std::string::size_type s;

	if (result == false and (s = lang.find('-')) != std::string::npos)
		result = test == lang.substr(0, s);

	return result;
}

template <>
object core_function_expression<CoreFunction::Number>::evaluate(expression_context &context)
{
	object v;

	if (m_args.size() == 1)
		v = m_args.front()->evaluate(context);
	else
		v = context.m_node->str();

	return v.as<double>();
}

template <>
object core_function_expression<CoreFunction::Floor>::evaluate(expression_context &context)
{
	object v = m_args.front()->evaluate(context);
	return floor(v.as<double>());
}

template <>
object core_function_expression<CoreFunction::Ceiling>::evaluate(expression_context &context)
{
	object v = m_args.front()->evaluate(context);
	return ceil(v.as<double>());
}

template <>
object core_function_expression<CoreFunction::Round>::evaluate(expression_context &context)
{
	object v = m_args.front()->evaluate(context);
	return round(v.as<double>());
}

// --------------------------------------------------------------------

class union_expression : public expression
{
  public:
	union_expression(expression_ptr lhs, expression_ptr rhs)
		: m_lhs(lhs)
		, m_rhs(rhs)
	{
	}

	virtual object evaluate(expression_context &context);

	// virtual void		print(int level)
	// 					{
	// 						indent(level);
	// 						std::cout << "union\n";
	// 						m_lhs->print(level + 1);
	// 						m_rhs->print(level + 1);
	// 					}

  private:
	expression_ptr m_lhs, m_rhs;
};

object union_expression::evaluate(expression_context &context)
{
	object v1 = m_lhs->evaluate(context);
	object v2 = m_rhs->evaluate(context);

	if (v1.type() != object_type::node_set or v2.type() != object_type::node_set)
		throw exception("union operator works only on node sets");

	node_set s1 = v1.as<const node_set &>();
	node_set s2 = v2.as<const node_set &>();

	copy(s2.begin(), s2.end(), back_inserter(s1));

	return s1;
}

// --------------------------------------------------------------------

struct xpath_imp
{
	xpath_imp();

	void reference();
	void release();

	node_set evaluate(node &root, context_imp &context);

	void parse(std::string_view path);
	// void				dump()
	// {
	// 	if (m_expr)
	// 		m_expr->print(0);
	// 	else
	// 		std::cout << "xpath is null\n";
	// }

	void preprocess(std::string_view path);

	unsigned char next_byte();
	char32_t get_next_char();
	void retract();
	Token get_next_token();
	std::string describe_token(Token token);
	void match(Token token);

	expression_ptr location_path();
	expression_ptr absolute_location_path();
	expression_ptr relative_location_path();
	expression_ptr step();
	expression_ptr node_test(AxisType axis);

	expression_ptr expr();
	expression_ptr primary_expr();
	expression_ptr function_call();
	expression_ptr argument();

	expression_ptr union_expr();
	expression_ptr path_expr();
	expression_ptr filter_expr();

	expression_ptr or_expr();
	expression_ptr and_expr();
	expression_ptr equality_expr();
	expression_ptr relational_expr();

	expression_ptr additive_expr();
	expression_ptr multiplicative_expr();
	expression_ptr unary_expr();

	// abbreviated steps are expanded like macros by the scanner
	std::string m_path;
	std::string::const_iterator
		m_begin,
		m_next, m_end;
	Token m_lookahead;
	std::string m_token_string;
	double m_token_number;
	AxisType m_token_axis;
	CoreFunction m_token_function;

	// the generated expression
	expression_ptr m_expr;

  private:
	~xpath_imp();

	long m_refcount;
};

// --------------------------------------------------------------------

xpath_imp::xpath_imp()
	: m_refcount(1)
{
}

xpath_imp::~xpath_imp()
{
}

void xpath_imp::reference()
{
	++m_refcount;
}

void xpath_imp::release()
{
	if (--m_refcount <= 0)
		delete this;
}

void xpath_imp::parse(std::string_view path)
{
	// start by expanding the abbreviations in the path
	preprocess(path);

	m_begin = m_next = m_path.begin();
	m_end = m_path.end();

	m_lookahead = get_next_token();
	m_expr = location_path();

	while (m_lookahead == Token::OperatorUnion)
	{
		match(Token::OperatorUnion);
		m_expr.reset(new union_expression(m_expr, location_path()));
	}

	//	if (VERBOSE)
	//		m_expr->print(0);

	match(Token::Eof);
}

void xpath_imp::preprocess(std::string_view path)
{
	// preprocessing consists of expanding abbreviations
	// replacements are:
	// @  => replaced by 'attribute::'
	// // => replaced by '/descendant-or-self::node()/'
	// . (if at a step location) => 'self::node()'
	// ..  (if at a step location) => 'parent::node()'

	m_path.clear();

	enum State
	{
		pp_Step,
		pp_Data,
		pp_Dot,
		pp_Slash,
		pp_String
	} state;

	state = pp_Step;
	char32_t quoteChar = 0;

	for (auto ch = path.begin(); ch != path.end(); ++ch)
	{
		switch (state)
		{
			case pp_Step:
				state = pp_Data;
				switch (*ch)
				{
					case '@': m_path += "attribute::"; break;
					case '.': state = pp_Dot; break;
					case '/': state = pp_Slash; break;
					case '\'':
					case '\"':
						m_path += *ch;
						quoteChar = *ch;
						state = pp_String;
						break;
					default: m_path += *ch; break;
				}
				break;

			case pp_Data:
				switch (*ch)
				{
					case '@': m_path += "attribute::"; break;
					case '/': state = pp_Slash; break;
					case '[':
						m_path += '[';
						state = pp_Step;
						break;
					case '\'':
					case '\"':
						m_path += *ch;
						quoteChar = *ch;
						state = pp_String;
						break;
					default: m_path += *ch; break;
				}
				break;

			case pp_Dot:
				if (*ch == '.')
					m_path += "parent::node()";
				else
				{
					--ch;
					m_path += "self::node()";
				}
				state = pp_Step;
				break;

			case pp_Slash:
				if (*ch == '/')
					m_path += "/descendant-or-self::node()/";
				else
				{
					--ch;
					m_path += '/';
				}
				state = pp_Step;
				break;

			case pp_String:
				m_path += *ch;
				if (static_cast<unsigned char>(*ch) == quoteChar)
					state = pp_Data;
				break;
		}
	}
}

unsigned char xpath_imp::next_byte()
{
	char result = 0;

	if (m_next < m_end)
	{
		result = *m_next;
		++m_next;
	}

	m_token_string += result;

	return static_cast<unsigned char>(result);
}

// We assume all paths are in valid UTF-8 encoding
char32_t xpath_imp::get_next_char()
{
	char32_t result = 0;
	unsigned char ch[5];

	ch[0] = next_byte();

	if ((ch[0] & 0x080) == 0)
		result = ch[0];
	else if ((ch[0] & 0x0E0) == 0x0C0)
	{
		ch[1] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080)
			throw exception("Invalid utf-8");
		result = ((ch[0] & 0x01F) << 6) | (ch[1] & 0x03F);
	}
	else if ((ch[0] & 0x0F0) == 0x0E0)
	{
		ch[1] = next_byte();
		ch[2] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080)
			throw exception("Invalid utf-8");
		result = ((ch[0] & 0x00F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F);
	}
	else if ((ch[0] & 0x0F8) == 0x0F0)
	{
		ch[1] = next_byte();
		ch[2] = next_byte();
		ch[3] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080 or (ch[3] & 0x0c0) != 0x080)
			throw exception("Invalid utf-8");
		result = ((ch[0] & 0x007) << 18) | ((ch[1] & 0x03F) << 12) | ((ch[2] & 0x03F) << 6) | (ch[3] & 0x03F);
	}

	if (result > 0x10ffff)
		throw exception("invalid utf-8 character (out of range)");

	return result;
}

void xpath_imp::retract()
{
	std::string::iterator c = m_token_string.end();

	// skip one valid character back in the input buffer
	// since we've arrived here, we can safely assume input
	// is valid UTF-8
	do
		--c;
	while ((*c & 0x0c0) == 0x080);

	if (m_next != m_end or *c != 0)
		m_next -= m_token_string.end() - c;
	m_token_string.erase(c, m_token_string.end());
}

std::string xpath_imp::describe_token(Token token)
{
	std::stringstream result;
	switch (token)
	{
		case Token::Undef: result << "undefined"; break;
		case Token::Eof: result << "end of expression"; break;
		case Token::LeftParenthesis: result << "left parenthesis"; break;
		case Token::RightParenthesis: result << "right parenthesis"; break;
		case Token::LeftBracket: result << "left bracket"; break;
		case Token::RightBracket: result << "right bracket"; break;
		case Token::Slash: result << "forward slash"; break;
		case Token::DoubleSlash: result << "double forward slash"; break;
		case Token::Comma: result << "comma"; break;
		case Token::Name: result << "name"; break;
		case Token::AxisSpec: result << "axis specification"; break;
		case Token::FunctionName: result << "function name"; break;
		case Token::NodeType: result << "node type specification"; break;
		case Token::OperatorUnion: result << "union operator"; break;
		case Token::OperatorAdd: result << "addition operator"; break;
		case Token::OperatorSubstract: result << "subtraction operator"; break;
		case Token::OperatorEqual: result << "equals operator"; break;
		case Token::OperatorNotEqual: result << "not-equals operator"; break;
		case Token::OperatorLess: result << "less operator"; break;
		case Token::OperatorLessOrEqual: result << "less-or-equal operator"; break;
		case Token::OperatorGreater: result << "greater operator"; break;
		case Token::OperatorGreaterOrEqual:
			result << "greater-or-equal operator";
			break;
		case Token::OperatorAnd: result << "logical-and operator"; break;
		case Token::OperatorOr: result << "logical-or operator"; break;
		case Token::OperatorMod: result << "modulus operator"; break;
		case Token::OperatorDiv: result << "division operator"; break;
		case Token::Literal: result << "literal"; break;
		case Token::Number: result << "number"; break;
		case Token::Variable: result << "variable"; break;
		case Token::Asterisk: result << "asterisk (or multiply)"; break;
		case Token::Colon: result << "colon"; break;
	}

	result << " {" << m_token_string << '}';
	return result.str();
}

Token xpath_imp::get_next_token()
{
	enum State
	{
		xps_Start,
		xps_VariableStart,
		xps_ExclamationMark,
		xps_LessThan,
		xps_GreaterThan,
		xps_Number,
		xps_NumberFraction,
		xps_Name,
		xps_QName,
		xps_QName2,
		xps_Literal
	} state = xps_Start;

	Token token = Token::Undef;
	bool variable = false;
	double fraction = 1.0;
	char32_t quoteChar = 0;

	m_token_string.clear();

	while (token == Token::Undef)
	{
		char32_t ch = get_next_char();

		switch (state)
		{
			case xps_Start:
				switch (ch)
				{
					case 0: token = Token::Eof; break;
					case '(': token = Token::LeftParenthesis; break;
					case ')': token = Token::RightParenthesis; break;
					case '[': token = Token::LeftBracket; break;
					case ']': token = Token::RightBracket; break;
					case ',': token = Token::Comma; break;
					case ':': token = Token::Colon; break;
					case '$': state = xps_VariableStart; break;
					case '*': token = Token::Asterisk; break;
					case '/': token = Token::Slash; break;
					case '|': token = Token::OperatorUnion; break;
					case '+': token = Token::OperatorAdd; break;
					case '-': token = Token::OperatorSubstract; break;
					case '=': token = Token::OperatorEqual; break;
					case '!': state = xps_ExclamationMark; break;
					case '<': state = xps_LessThan; break;
					case '>': state = xps_GreaterThan; break;
					case ' ':
					case '\n':
					case '\r':
					case '\t':
						m_token_string.clear();
						break;
					case '\'':
						quoteChar = ch;
						state = xps_Literal;
						break;
					case '"':
						quoteChar = ch;
						state = xps_Literal;
						break;

					case '@':
						token = Token::AxisSpec;
						m_token_axis = AxisType::Attribute;
						break;

					default:
						if (ch == '.')
						{
							m_token_number = 0;
							state = xps_NumberFraction;
						}
						else if (ch >= '0' and ch <= '9')
						{
							m_token_number = ch - '0';
							state = xps_Number;
						}
						else if (is_name_start_char(ch))
							state = xps_Name;
						else
							throw exception("invalid character in xpath");
				}
				break;

			case xps_ExclamationMark:
				if (ch != '=')
				{
					retract();
					throw exception("unexpected character ('!') in xpath");
				}
				token = Token::OperatorNotEqual;
				break;

			case xps_LessThan:
				if (ch == '=')
					token = Token::OperatorLessOrEqual;
				else
				{
					retract();
					token = Token::OperatorLess;
				}
				break;

			case xps_GreaterThan:
				if (ch == '=')
					token = Token::OperatorGreaterOrEqual;
				else
				{
					retract();
					token = Token::OperatorGreater;
				}
				break;

			case xps_Number:
				if (ch >= '0' and ch <= '9')
					m_token_number = 10 * m_token_number + (ch - '0');
				else if (ch == '.')
				{
					fraction = 0.1;
					state = xps_NumberFraction;
				}
				else
				{
					retract();
					token = Token::Number;
				}
				break;

			case xps_NumberFraction:
				if (ch >= '0' and ch <= '9')
				{
					m_token_number += fraction * (ch - '0');
					fraction /= 10;
				}
				else
				{
					retract();
					token = Token::Number;
				}
				break;

			case xps_VariableStart:
				if (is_name_start_char(ch))
				{
					variable = true;
					state = xps_Name;
				}
				else
					throw exception("invalid variable name or lone dollar character");
				break;

			case xps_Name:
				if (ch == ':')
					state = xps_QName;
				else if (not is_name_char(ch))
				{
					retract();
					if (variable)
						token = Token::Variable;
					else
						token = Token::Name;
				}
				break;

			case xps_QName:
				if (ch != ':' and is_name_start_char(ch))
					state = xps_QName2;
				else
				{
					retract(); // ch
					retract(); // ':'
					if (variable)
						token = Token::Variable;
					else
						token = Token::Name;
				}
				break;

			case xps_QName2:
				if (ch == ':' or not is_name_char(ch))
				{
					retract();
					if (variable)
						token = Token::Variable;
					else
						token = Token::Name;
				}
				break;

			case xps_Literal:
				if (ch == 0)
					throw exception("run-away string, missing quote character?");
				else if (ch == quoteChar)
				{
					token = Token::Literal;
					m_token_string = m_token_string.substr(1, m_token_string.length() - 2);
				}
				break;
		}
	}

	if (token == Token::Name) // we've scanned a name, but it might as well be a function, nodetype or axis
	{
		if (m_token_string == "and")
			token = Token::OperatorAnd;
		else if (m_token_string == "or")
			token = Token::OperatorOr;
		else if (m_token_string == "mod")
			token = Token::OperatorMod;
		else if (m_token_string == "div")
			token = Token::OperatorDiv;
		else
		{
			// look forward and see what's ahead
			for (std::string::const_iterator c = m_next; c != m_end; ++c)
			{
				if (isspace(*c))
					continue;

				if (*c == ':' and *(c + 1) == ':') // it must be an axis specifier
				{
					token = Token::AxisSpec;

					const int kAxisNameCount = sizeof(kAxisNames) / sizeof(const char *);
					const char **a = find(kAxisNames, kAxisNames + kAxisNameCount, m_token_string);
					if (*a != nullptr)
						m_token_axis = AxisType(a - kAxisNames);
					else
						throw exception("invalid axis specification " + m_token_string);

					// skip over the double colon
					m_next = c + 2;
				}
				else if (*c == '(')
				{
					if (m_token_string == "comment" or m_token_string == "text" or
						m_token_string == "processing-instruction" or m_token_string == "node")
					{
						token = Token::NodeType;

						// set input pointer after the parenthesis
						m_next = c + 1;
						while (m_next != m_end and isspace(*m_next))
							++m_next;
						if (*m_next != ')')
							throw exception("expected '()' after a node type specifier");
						++m_next;
					}
					else
					{
						for (size_t i = 0; i < kCoreFunctionCount; ++i)
						{
							if (m_token_string == kCoreFunctionInfo[i].name)
							{
								token = Token::FunctionName;
								m_token_function = CoreFunction(i);
								break;
							}
						}

						if (token != Token::FunctionName)
							throw exception("invalid function " + m_token_string);
					}
				}

				break;
			}
		}
	}

	//	if (VERBOSE)
	//		std::cout << "get_next_token: " << describe_token(token) << '\n';

	return token;
}

void xpath_imp::match(Token token)
{
	if (m_lookahead == token)
		m_lookahead = get_next_token();
	else
	{
		// aargh... syntax error

		std::string found = describe_token(m_lookahead);

		if (m_lookahead != Token::Eof and m_lookahead != Token::Undef)
		{
			found += " (\"";
			found += m_token_string;
			found += "\")";
		}

		std::string expected = describe_token(token);

		std::stringstream s;
		s << "syntax error in xpath, expected " << expected << " but found " << found;
		throw exception(s.str());
	}
}

expression_ptr xpath_imp::location_path()
{
	bool absolute = false;
	if (m_lookahead == Token::Slash)
	{
		absolute = true;
		match(Token::Slash);
	}

	expression_ptr result(relative_location_path());

	if (absolute)
		result.reset(new path_expression(expression_ptr(new root_expression()), result));

	return result;
}

expression_ptr xpath_imp::relative_location_path()
{
	expression_ptr result(step());

	while (m_lookahead == Token::Slash)
	{
		match(Token::Slash);
		result.reset(new path_expression(result, step()));
	}

	return result;
}

expression_ptr xpath_imp::step()
{
	expression_ptr result;

	AxisType axis = AxisType::Child;
	if (m_lookahead == Token::AxisSpec)
	{
		axis = m_token_axis;
		match(Token::AxisSpec);
	}

	result = node_test(axis);

	while (m_lookahead == Token::LeftBracket)
	{
		match(Token::LeftBracket);
		result.reset(new predicate_expression(result, expr()));
		match(Token::RightBracket);
	}

	return result;
}

expression_ptr xpath_imp::node_test(AxisType axis)
{
	expression_ptr result;

	if (m_lookahead == Token::Asterisk)
	{
		result.reset(new name_test_step_expression(axis, m_token_string));
		match(Token::Asterisk);
	}
	else if (m_lookahead == Token::NodeType)
	{
		std::string name = m_token_string;
		match(Token::NodeType);

		if (name == "comment")
			result.reset(new node_type_expression<comment>(axis));
		else if (name == "text")
			result.reset(new node_type_expression<text>(axis));
		else if (name == "processing-instruction")
			result.reset(new node_type_expression<processing_instruction>(axis));
		else if (name == "node")
			result.reset(new node_type_expression<node>(axis));
		else
			throw exception("invalid node type specified: " + name);
	}
	else
	{
		result.reset(new name_test_step_expression(axis, m_token_string));
		match(Token::Name);
	}

	return result;
}

expression_ptr xpath_imp::expr()
{
	expression_ptr result(and_expr());

	while (m_lookahead == Token::OperatorOr)
	{
		match(Token::OperatorOr);
		result.reset(new operator_expression<Token::OperatorOr>(result, and_expr()));
	}

	return result;
}

expression_ptr xpath_imp::primary_expr()
{
	expression_ptr result;

	switch (m_lookahead)
	{
		case Token::Variable:
			result.reset(new variable_expression(m_token_string.substr(1)));
			match(Token::Variable);
			break;

		case Token::LeftParenthesis:
			match(Token::LeftParenthesis);
			result = expr();
			match(Token::RightParenthesis);
			break;

		case Token::Literal:
			result.reset(new literal_expression(m_token_string));
			match(Token::Literal);
			break;

		case Token::Number:
			result.reset(new number_expression(m_token_number));
			match(Token::Number);
			break;

		case Token::FunctionName:
			result = function_call();
			break;

		default:
			throw exception("invalid primary expression in xpath");
	}

	return result;
}

expression_ptr xpath_imp::function_call()
{
	using namespace std::literals;

	CoreFunction function = m_token_function;

	match(Token::FunctionName);
	match(Token::LeftParenthesis);

	expression_list arguments;

	if (m_lookahead != Token::RightParenthesis)
	{
		for (;;)
		{
			arguments.push_back(expr());
			if (m_lookahead == Token::Comma)
				match(Token::Comma);
			else
				break;
		}
	}
	match(Token::RightParenthesis);

	expression_ptr result;

	int expected_arg_count = kCoreFunctionInfo[int(function)].arg_count;
	if (expected_arg_count > 0)
	{
		if (int(arguments.size()) != expected_arg_count)
			throw exception("invalid number of arguments for function "s + kCoreFunctionInfo[int(function)].name);
	}
	else if (expected_arg_count == kOptionalArgument)
	{
		if (arguments.size() > 1)
			throw exception("incorrect number of arguments for function "s + kCoreFunctionInfo[int(function)].name);
	}
	else if (expected_arg_count < 0 and int(arguments.size()) < -expected_arg_count)
		throw exception("insufficient number of arguments for function "s + kCoreFunctionInfo[int(function)].name);

	switch (function)
	{
		case CoreFunction::Last: result.reset(new core_function_expression<CoreFunction::Last>(arguments)); break;
		case CoreFunction::Position: result.reset(new core_function_expression<CoreFunction::Position>(arguments)); break;
		case CoreFunction::Count: result.reset(new core_function_expression<CoreFunction::Count>(arguments)); break;
		case CoreFunction::Id: result.reset(new core_function_expression<CoreFunction::Id>(arguments)); break;
		case CoreFunction::LocalName: result.reset(new core_function_expression<CoreFunction::LocalName>(arguments)); break;
		case CoreFunction::NamespaceUri: result.reset(new core_function_expression<CoreFunction::NamespaceUri>(arguments)); break;
		case CoreFunction::Name: result.reset(new core_function_expression<CoreFunction::Name>(arguments)); break;
		case CoreFunction::String: result.reset(new core_function_expression<CoreFunction::String>(arguments)); break;
		case CoreFunction::Concat: result.reset(new core_function_expression<CoreFunction::Concat>(arguments)); break;
		case CoreFunction::StartsWith: result.reset(new core_function_expression<CoreFunction::StartsWith>(arguments)); break;
		case CoreFunction::Contains: result.reset(new core_function_expression<CoreFunction::Contains>(arguments)); break;
		case CoreFunction::SubstringBefore: result.reset(new core_function_expression<CoreFunction::SubstringBefore>(arguments)); break;
		case CoreFunction::SubstringAfter: result.reset(new core_function_expression<CoreFunction::SubstringAfter>(arguments)); break;
		case CoreFunction::StringLength: result.reset(new core_function_expression<CoreFunction::StringLength>(arguments)); break;
		case CoreFunction::NormalizeSpace: result.reset(new core_function_expression<CoreFunction::NormalizeSpace>(arguments)); break;
		case CoreFunction::Translate: result.reset(new core_function_expression<CoreFunction::Translate>(arguments)); break;
		case CoreFunction::Boolean: result.reset(new core_function_expression<CoreFunction::Boolean>(arguments)); break;
		case CoreFunction::Not: result.reset(new core_function_expression<CoreFunction::Not>(arguments)); break;
		case CoreFunction::True: result.reset(new core_function_expression<CoreFunction::True>(arguments)); break;
		case CoreFunction::False: result.reset(new core_function_expression<CoreFunction::False>(arguments)); break;
		case CoreFunction::Lang: result.reset(new core_function_expression<CoreFunction::Lang>(arguments)); break;
		case CoreFunction::Number: result.reset(new core_function_expression<CoreFunction::Number>(arguments)); break;
		case CoreFunction::Sum: result.reset(new core_function_expression<CoreFunction::Sum>(arguments)); break;
		case CoreFunction::Floor: result.reset(new core_function_expression<CoreFunction::Floor>(arguments)); break;
		case CoreFunction::Ceiling: result.reset(new core_function_expression<CoreFunction::Ceiling>(arguments)); break;
		case CoreFunction::Round: result.reset(new core_function_expression<CoreFunction::Round>(arguments)); break;
		case CoreFunction::Comment: result.reset(new core_function_expression<CoreFunction::Comment>(arguments)); break;
		default: break;
	}

	return result;
}

expression_ptr xpath_imp::union_expr()
{
	expression_ptr result(path_expr());

	while (m_lookahead == Token::OperatorUnion)
	{
		match(m_lookahead);
		result.reset(new union_expression(result, path_expr()));
	}

	return result;
}

expression_ptr xpath_imp::path_expr()
{
	expression_ptr result;

	if (m_lookahead == Token::Variable or m_lookahead == Token::LeftParenthesis or
		m_lookahead == Token::Literal or m_lookahead == Token::Number or m_lookahead == Token::FunctionName)
	{
		result = filter_expr();

		if (m_lookahead == Token::Slash)
		{
			match(Token::Slash);
			result.reset(new path_expression(result, relative_location_path()));
		}
	}
	else
		result = location_path();

	return result;
}

expression_ptr xpath_imp::filter_expr()
{
	expression_ptr result(primary_expr());

	while (m_lookahead == Token::LeftBracket)
	{
		match(Token::LeftBracket);
		result.reset(new predicate_expression(result, expr()));
		match(Token::RightBracket);
	}

	return result;
}

expression_ptr xpath_imp::and_expr()
{
	expression_ptr result(equality_expr());

	while (m_lookahead == Token::OperatorAnd)
	{
		match(Token::OperatorAnd);
		result.reset(new operator_expression<Token::OperatorAnd>(result, equality_expr()));
	}

	return result;
}

expression_ptr xpath_imp::equality_expr()
{
	expression_ptr result(relational_expr());

	while (m_lookahead == Token::OperatorEqual or m_lookahead == Token::OperatorNotEqual)
	{
		Token op = m_lookahead;
		match(m_lookahead);
		if (op == Token::OperatorEqual)
			result.reset(new operator_expression<Token::OperatorEqual>(result, relational_expr()));
		else
			result.reset(new operator_expression<Token::OperatorNotEqual>(result, relational_expr()));
	}

	return result;
}

expression_ptr xpath_imp::relational_expr()
{
	expression_ptr result(additive_expr());

	while (m_lookahead == Token::OperatorLess or m_lookahead == Token::OperatorLessOrEqual or
		   m_lookahead == Token::OperatorGreater or m_lookahead == Token::OperatorGreaterOrEqual)
	{
		Token op = m_lookahead;
		match(m_lookahead);

		expression_ptr rhs = additive_expr();
		switch (op)
		{
			case Token::OperatorLess:
				result.reset(new operator_expression<Token::OperatorLess>(result, rhs));
				break;

			case Token::OperatorLessOrEqual:
				result.reset(new operator_expression<Token::OperatorLessOrEqual>(result, rhs));
				break;

			case Token::OperatorGreater:
				result.reset(new operator_expression<Token::OperatorGreater>(result, rhs));
				break;

			case Token::OperatorGreaterOrEqual:
				result.reset(new operator_expression<Token::OperatorGreaterOrEqual>(result, rhs));
				break;

			default:
				break;
		}
	}

	return result;
}

expression_ptr xpath_imp::additive_expr()
{
	expression_ptr result(multiplicative_expr());

	while (m_lookahead == Token::OperatorAdd or m_lookahead == Token::OperatorSubstract)
	{
		Token op = m_lookahead;
		match(m_lookahead);
		if (op == Token::OperatorAdd)
			result.reset(new operator_expression<Token::OperatorAdd>(result, multiplicative_expr()));
		else
			result.reset(new operator_expression<Token::OperatorSubstract>(result, multiplicative_expr()));
	}

	return result;
}

expression_ptr xpath_imp::multiplicative_expr()
{
	expression_ptr result(unary_expr());

	for (;;)
	{
		if (m_lookahead == Token::Asterisk)
		{
			match(m_lookahead);
			result.reset(new operator_expression<Token::Asterisk>(result, unary_expr()));
			continue;
		}

		if (m_lookahead == Token::OperatorMod)
		{
			match(m_lookahead);
			result.reset(new operator_expression<Token::OperatorMod>(result, unary_expr()));
			continue;
		}

		if (m_lookahead == Token::OperatorDiv)
		{
			match(m_lookahead);
			result.reset(new operator_expression<Token::OperatorDiv>(result, unary_expr()));
			continue;
		}

		break;
	}

	return result;
}

expression_ptr xpath_imp::unary_expr()
{
	expression_ptr result;

	if (m_lookahead == Token::OperatorSubstract)
	{
		match(Token::OperatorSubstract);
		result.reset(new negate_expression(unary_expr()));
	}
	else
		result = union_expr();

	return result;
}

// --------------------------------------------------------------------

node_set xpath_imp::evaluate(node &root, context_imp &ctxt)
{
	node_set empty;
	expression_context context(ctxt, &root, empty);
	return m_expr->evaluate(context).as<const node_set &>();
}

// --------------------------------------------------------------------

context::context()
	: m_impl(new context_imp)
{
}

context::~context()
{
	delete m_impl;
}

void context::set(const std::string &name, double value)
{
	m_impl->set(name, value);
}

template <>
double context::get<double>(const std::string &name)
{
	return m_impl->get(name).as<double>();
}

void context::set(const std::string &name, const std::string &value)
{
	m_impl->set(name, value);
}

template <>
std::string context::get<std::string>(const std::string &name)
{
	return m_impl->get(name).as<std::string>();
}

// --------------------------------------------------------------------

xpath::xpath(std::string_view path)
	: m_impl(new xpath_imp())
{
	m_impl->parse(path);
}

xpath::xpath(const xpath &rhs)
	: m_impl(rhs.m_impl)
{
	m_impl->reference();
}

xpath &xpath::operator=(const xpath &rhs)
{
	if (this != &rhs)
	{
		m_impl->release();
		m_impl = rhs.m_impl;
		m_impl->reference();
	}

	return *this;
}

xpath::~xpath()
{
	m_impl->release();
}

template <>
node_set xpath::evaluate<node>(const node &root, context &ctxt) const
{
	return m_impl->evaluate(const_cast<node &>(root), *ctxt.m_impl);
}

template <>
element_set xpath::evaluate<element>(const node &root, context &ctxt) const
{
	element_set result;

	object s(m_impl->evaluate(const_cast<node &>(root), *ctxt.m_impl));
	for (node *n : s.as<const node_set &>())
	{
		if (n->type() == node_type::element)
			result.push_back(static_cast<element *>(n));
	}

	return result;
}

bool xpath::matches(const node *n) const
{
	bool result = false;
	if (n != nullptr)
	{
		const node *root = n->root();

		context ctxt;
		object s(m_impl->evaluate(const_cast<node &>(*root), *ctxt.m_impl));
		for (node *e : s.as<const node_set &>())
		{
			if (e == n)
			{
				result = true;
				break;
			}
		}
	}

	return result;
}

void xpath::dump()
{
	// m_impl->dump();
}

} // namespace mxml
