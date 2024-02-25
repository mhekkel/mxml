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
/// the core of the libzeep XML library defining the main classes in the DOM API

#include <cassert>
#include <compare>
#include <list>
#include <string>
#include <tuple>
#include <typeinfo>

export module mxml:node;

import :error;

namespace mxml
{

// forward declarations

export class node;
export class element;
export class text;
export class attribute;
export class name_space;
export class comment;
export class cdata;
export class processing_instruction;
export class document;

template <typename T>
concept NodeType = std::is_base_of_v<mxml::node, T>;

// template <NodeType T>
// class basic_node_list;

// using node_set = std::list<node *>;
// using element_set = std::list<element *>;

// --------------------------------------------------------------------

/// \brief specification of how XML data should be written out
struct format_info
{
	bool indent = false;
	bool indent_attributes = false;
	bool collapse_tags = true;
	bool suppress_comments = false;
	bool escape_white_space = false;
	bool escape_double_quote = true;
	bool html = false; ///< This flag can be used to collapse only 'empty elements'
	std::size_t indent_width = 0;
	std::size_t indent_level = 0;
	float version = 1.0f;
};

// --------------------------------------------------------------------

/// Node is the abstract base class for all data contained in zeep XML documents.
/// The DOM tree consists of nodes that are linked to each other, each
/// node can have a parent and siblings pointed to by the next and
/// previous members. All nodes in a DOM tree share a common root node.
///
/// Nodes can have a name, and the XPath specification requires that a node can
/// have a so-called expanded-name. This name consists of a local-name and a
/// namespace which is a URI. And we can have a QName which is a concatenation of
/// a prefix (that points to a namespace URI) and a local-name separated by a colon.
///
/// To reduce storage requirements, names are stored in nodes as qnames, if at all.
/// the convenience functions name() and prefix() parse the qname(). ns() returns
/// the namespace URI for the node, if it can be resolved.
///
/// Nodes inherit the namespace of their parent unless they override it which means
/// resolving prefixes and namespaces is done hierarchically
///
/// Nodes are stored in a node_list, a generic list class that resembles std::list

class node
{
  public:
	virtual ~node();

	/// content of a xml:lang attribute of this element, or its nearest ancestor
	virtual std::string lang() const;

	/// Nodes can have a name, and the XPath specification requires that a node can
	/// have a so-called expanded-name. This name consists of a local-name and a
	/// namespace which is a URI. And we can have a QName which is a concatenation of
	/// a prefix (that points to a namespace URI) and a local-name separated by a colon.
	///
	/// To reduce storage requirements, names are stored in nodes as qnames, if at all.
	virtual std::string get_qname() const;
	virtual void set_qname(const std::string &qn) = 0;

	/// \brief set the qname with two parameters, if \a prefix is empty the qname will be simply \a name
	/// otherwise the name will be `prefix:name`
	/// \param prefix	The namespace prefix to use
	/// \param name		The actual name to use
	void set_qname(const std::string &prefix, const std::string &name);

	virtual std::string name() const;       ///< The name for the node as parsed from the qname.
	virtual std::string get_prefix() const; ///< The prefix for the node as parsed from the qname.
	virtual std::string get_ns() const;     ///< Returns the namespace URI for the node, if it can be resolved.

	/// Return the namespace URI for a prefix
	virtual std::string namespace_for_prefix(const std::string &prefix) const;

	/// Return the prefix for a namespace URI
	virtual std::pair<std::string, bool> prefix_for_namespace(const std::string &uri) const;

	/// Prefix the \a tag with the namespace prefix for \a uri
	virtual std::string prefix_tag(const std::string &tag, const std::string &uri) const;

	/// return all content concatenated, including that of children.
	virtual std::string str() const = 0;

	// --------------------------------------------------------------------
	// low level routines

	// basic access

	// All nodes should have a single root node
	virtual element *root();             ///< The root node for this node
	virtual const element *root() const; ///< The root node for this node

	element *parent() { return m_parent; }             ///< The parent node for this node
	const element *parent() const { return m_parent; } ///< The parent node for this node

	node *next() { return m_next; }             ///< The next sibling
	const node *next() const { return m_next; } ///< The next sibling

	node *prev() { return m_prev; }             ///< The previous sibling
	const node *prev() const { return m_prev; } ///< The previous sibling

	/// Compare the node with \a n
	virtual bool equals(const node *n) const;

	/// debug routine
	virtual void validate();

	/// \brief low level routine for writing out XML
	///
	/// This method is usually called by operator<<(std::ostream&, zeep::xml::document&)
	virtual void write(std::ostream &os, format_info fmt) const = 0;

  protected:
	friend class element;
	// friend class document;
	template <typename>
	friend class basic_node_list;
	// friend class node_list;

	node();
	node(const node &n);
	node(node &&n);
	node &operator=(const node &n);
	node &operator=(node &&n);

	// virtual void insert_sibling(node *n, node *before);
	// virtual void remove_sibling(node *n);

	void parent(element *p);
	void next(const node *n);
	void prev(const node *n);

  protected:
	element *m_parent = nullptr;
	node *m_next = nullptr;
	node *m_prev = nullptr;
};

// --------------------------------------------------------------------
// A pseudo node, used to indicate end of list

class sentinel_node : public node
{
  public:
	sentinel_node() = default;

	void set_qname(const std::string &qn) override {}
	std::string str() const override { return {}; }
	void write(std::ostream &os, format_info fmt) const override {}

  private:
};

// // --------------------------------------------------------------------
// /// internal node base class for storing text

// class node_with_text : public node
// {
//   public:
// 	node_with_text() {}
// 	node_with_text(const std::string &s)
// 		: m_text(s)
// 	{
// 	}

// 	/// \brief return the text content
// 	std::string str() const override { return m_text; }

// 	/// \brief return the text content, same as str()
// 	virtual std::string get_text() const { return m_text; }

// 	/// \brief set the text content
// 	virtual void set_text(const std::string &text) { m_text = text; }

//   protected:
// 	std::string m_text;
// };

// // --------------------------------------------------------------------
// /// A node containing a XML comment

// export class comment : public node_with_text
// {
//   public:
// 	comment() {}
// 	comment(comment &&c) noexcept
// 		: node_with_text(std::move(c.m_text))
// 	{
// 	}
// 	comment(const std::string &text)
// 		: node_with_text(text)
// 	{
// 	}

// 	/// \brief compare nodes for equality
// 	bool equals(const node *n) const override;

//   protected:
// 	void write(std::ostream &os, format_info fmt) const override;
// };

// // --------------------------------------------------------------------
// /// A node containing a XML processing instruction (like e.g. \<?php ?\>)

// export class processing_instruction : public node_with_text
// {
//   public:
// 	processing_instruction() {}

// 	processing_instruction(processing_instruction &&pi) noexcept
// 		: node_with_text(std::move(pi.m_text))
// 		, m_target(std::move(pi.m_target))
// 	{
// 	}

// 	/// \brief constructor with parameters
// 	///
// 	/// This constructs a processing instruction with the specified parameters
// 	/// \param target	The target, this will follow the <? characters, e.g. `php` will generate <?php ... ?>
// 	/// \param text		The text inside this node, e.g. the PHP code.
// 	processing_instruction(const std::string &target, const std::string &text)
// 		: node_with_text(text)
// 		, m_target(target)
// 	{
// 	}

// 	/// \brief return the qname which is the same as the target in this case
// 	std::string get_qname() const override { return m_target; }

// 	/// \brief return the target
// 	std::string get_target() const { return m_target; }

// 	/// \brief set the target
// 	void set_target(const std::string &target) { m_target = target; }

// 	/// \brief compare nodes for equality
// 	bool equals(const node *n) const override;

//   protected:
// 	void write(std::ostream &os, format_info fmt) const override;

//   private:
// 	std::string m_target;
// };

// // --------------------------------------------------------------------
// /// A node containing text.

// class text : public node_with_text
// {
//   public:
// 	text() {}

// 	text(text &&t) noexcept
// 		: node_with_text(std::move(t.m_text))
// 	{
// 	}

// 	text(const std::string &text)
// 		: node_with_text(text)
// 	{
// 	}

// 	/// \brief append \a text to the stored text
// 	void append(const std::string &text) { m_text.append(text); }

// 	/// \brief compare nodes for equality
// 	bool equals(const node *n) const override;

// 	/// \brief returns true if this text contains only whitespace characters
// 	bool is_space() const;

//   protected:
// 	void write(std::ostream &os, format_info fmt) const override;
// };

// // --------------------------------------------------------------------
// /// A node containing the contents of a CDATA section. Normally, these nodes are
// /// converted to text nodes but you can specify to preserve them when parsing a
// /// document.

// export class cdata : public text
// {
//   public:
// 	cdata() {}
// 	cdata(cdata &&cd) noexcept
// 		: text(std::move(cd))
// 	{
// 	}
// 	cdata(const std::string &s)
// 		: text(s)
// 	{
// 	}

// 	/// \brief compare nodes for equality
// 	bool equals(const node *n) const override;

//   protected:
// 	void write(std::ostream &os, format_info fmt) const override;
// };

// // --------------------------------------------------------------------
// /// An attribute is a node, has an element as parent, but is not a child of this parent (!)

// export class attribute : public node
// {
//   public:
// 	friend class element;

// 	using parent_type = element;

// 	attribute(const attribute &attr)
// 		: node()
// 		, m_qname(attr.m_qname)
// 		, m_value(attr.m_value)
// 		, m_id(attr.m_id)
// 	{
// 	}

// 	attribute(attribute &&attr) noexcept
// 		: node()
// 		, m_qname(std::move(attr.m_qname))
// 		, m_value(std::move(attr.m_value))
// 		, m_id(attr.m_id)
// 	{
// 	}

// 	attribute(const std::string &qname, const std::string &value, bool id = false)
// 		: m_qname(qname)
// 		, m_value(value)
// 		, m_id(id)
// 	{
// 	}

// 	attribute &operator=(attribute &&attr) noexcept
// 	{
// 		std::swap(m_qname, attr.m_qname);
// 		std::swap(m_value, attr.m_value);
// 		m_id = attr.m_id;
// 		return *this;
// 	}

// 	std::strong_ordering operator<=>(const attribute &a) const
// 	{
// 		if (auto cmp = *this <=> a; cmp != 0)
// 			return cmp;
// 		return m_value <=> a.m_value;
// 	}

// 	std::string get_qname() const override { return m_qname; }
// 	void set_qname(const std::string &qn) override { m_qname = qn; }

// 	using node::set_qname;

// 	/// \brief Is this attribute an xmlns attribute?
// 	bool is_namespace() const
// 	{
// 		return m_qname.compare(0, 5, "xmlns") == 0 and (m_qname[5] == 0 or m_qname[5] == ':');
// 	}

// 	std::string value() const { return m_value; }
// 	void set_value(const std::string &v) { m_value = v; }

// 	/// \brief same as value, but checks to see if this really is a namespace attribute
// 	std::string uri() const;

// 	std::string str() const override { return m_value; }

// 	/// \brief compare nodes for equality
// 	bool equals(const node *n) const override;

// 	/// \brief returns whether this attribute is an ID attribute, as defined in an accompanying DTD
// 	bool is_id() const { return m_id; }

// 	/// \brief support for structured binding
// 	template <size_t N>
// 	decltype(auto) get() const
// 	{
// 		if constexpr (N == 0)
// 			return name();
// 		else if constexpr (N == 1)
// 			return value();
// 	}

// 	void swap(attribute &a)
// 	{
// 		std::swap(m_qname, a.m_qname);
// 		std::swap(m_value, a.m_value);
// 	}

//   protected:
// 	void write(std::ostream &os, format_info fmt) const override;

//   private:
// 	std::string m_qname, m_value;
// 	bool m_id;
// };

// --------------------------------------------------------------------
/// \brief generic iterator class.
///
/// We can have iterators that point to nodes, elements and attributes.
/// Iterating over nodes is simply following next/prev. But iterating
/// elements is a bit more difficult, since you then have to skip nodes
/// in between that are not an element, like comments or text.

template <typename T>
class iterator_impl
{
  public:
	using node_type = T;
	using node_base_type = std::conditional_t<std::is_const_v<T>, const node, node>;

	// using container_node_type = std::remove_cv_t<ContainerNodeType>;
	// using container_type = basic_node_list<container_node_type>;

	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = node_type;
	using pointer = value_type *;
	using reference = value_type &;
	using difference_type = std::ptrdiff_t;

	iterator_impl() = default;
	iterator_impl(node *current);
	iterator_impl(const node *current);

	iterator_impl(const iterator_impl &i) = default;

	/// \brief copy constructor, kind of
	template <NodeType T2>
		requires std::is_assignable_v<node_type, T2>
	iterator_impl(const iterator_impl<T2> &i)
		: m_current(i.m_current)
	{
	}

	// template <typename Iterator>
	// 	requires(not std::is_same_v<std::remove_const_t<typename Iterator::value_type>, element> and
	// 				std::is_base_of_v<value_type, typename Iterator::value_type>)
	// iterator_impl(const Iterator &i)
	// 	: m_container(const_cast<container_type *>(i.m_container))
	// 	, m_current(i.m_current)
	// 	, m_at_end(i.m_at_end)
	// {
	// }

	iterator_impl &operator=(const iterator_impl &i)
	{
		if (this != &i)
			m_current = i.m_current;
		return *this;
	}

	template <typename Iterator>
		requires(std::is_base_of_v<value_type, typename Iterator::value_type>)
	iterator_impl &operator=(const Iterator &i)
	{
		m_current = i.m_current;
		return *this;
	}

	reference operator*() { return *static_cast<node_type *>(m_current); }
	pointer operator->() const { return static_cast<node_type *>(m_current); }

	iterator_impl &operator++();

	iterator_impl operator++(int)
	{
		iterator_impl iter(*this);
		operator++();
		return iter;
	}

	iterator_impl &operator--();

	iterator_impl operator--(int)
	{
		iterator_impl iter(*this);
		operator--();
		return iter;
	}

	template <typename IteratorType>
		requires NodeType<typename IteratorType::node_type>
	bool operator==(const IteratorType &other) const
	{
		return m_current == other.m_current;
	}

	bool operator==(const iterator_impl &other) const
	{
		return m_current == other.m_current;
	}

	bool operator!=(const iterator_impl &other) const
	{
		return not operator==(other);
	}

	template <NodeType T2>
	bool operator==(const T2 *n) const { return m_current == n; }

	template <NodeType T2>
	bool operator!=(const T2 *n) const { return m_current != n; }

	explicit operator pointer() const { return m_current; }
	explicit operator pointer() { return m_current; }

  private:
	node_base_type *m_current = nullptr;
};

// --------------------------------------------------------------------
/// \brief basic_node_list, a base class for containers of nodes
///
/// We have two container classes (node_list specializations)
/// One is for attributes and name_spaces. The other is the
/// node_list for nodes in elements. However, this list can
/// present itself as node_list for elements hiding all other
/// node types.

template <typename T>
class basic_node_list
{
  public:
	// template <typename>
	// friend class iterator_impl;
	// friend class element;

	using node_type = T;

	// element is a container of elements
	using value_type = node_type;
	using allocator_type = std::allocator<value_type>;
	using size_type = size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;

  protected:
	basic_node_list(element &e, node *&head, sentinel_node &sentinel)
		: m_element(e)
		, m_head(head)
		, m_sentinel(sentinel)
	{
	}

  public:
	virtual ~basic_node_list()
	{
	}

	// bool operator==(const basic_node_list &l) const
	// {
	// 	bool result = true;
	// 	auto a = begin(), b = l.begin();
	// 	for (; result and a != end() and b != l.end(); ++a, ++b)
	// 		result = a->equals(b.current());
	// 	return result and a == end() and b == l.end();
	// }

	// bool operator!=(const basic_node_list &l) const
	// {
	// 	return not operator==(l);
	// }

	using iterator = iterator_impl<node_type>;
	using const_iterator = iterator_impl<const node_type>;

	iterator begin() { return iterator(m_head); }
	iterator end() { return iterator(&m_sentinel); }

	const_iterator cbegin() { return const_iterator(m_head); }
	const_iterator cend() { return const_iterator(&m_sentinel); }

	const_iterator begin() const { return const_iterator(m_head); }
	const_iterator end() const { return const_iterator(&m_sentinel); }

	value_type &front() { return *begin(); }
	const value_type &front() const { return *begin(); }

	value_type &back() { return std::prev(end()); }
	const value_type &back() const { return std::prev(end()); }

	bool empty() const { return m_head == &m_sentinel; }
	size_t size() const { return std::distance(begin(), end()); }

	void clear()
	{
		delete m_head;
		m_head = &m_sentinel;
	}

	// void swap(basic_node_list &l) noexcept; /*
	//  {
	//      std::swap(m_head, l.m_head);
	//      std::swap(m_tail, l.m_tail);

	//      for (auto &n : *this)
	//          n.m_parent = &m_element;

	//      for (auto &n : l)
	//          n.m_parent = &l.m_element;
	//  } */

	// /// \brief sort the (direct) nodes in this list using \a comp as comparator
	// template <typename Compare>
	// void sort(Compare comp)
	// {
	// 	for (auto a = begin(); a + 1 != end(); ++a)
	// 	{
	// 		for (auto b = a + 1; b != end(); ++b)
	// 		{
	// 			if (comp(*b, *a))
	// 				a->swap(*b);
	// 		}
	// 	}
	// }

  protected:
	// proxy methods for every insertion

	iterator insert_impl(const_iterator pos, node *n)
	{
		assert(n != nullptr);
		assert(n->next() == nullptr);
		assert(n->prev() == nullptr);

		if (n == nullptr)
			throw exception("Invalid pointer passed to insert");

		if (n->parent() != nullptr or n->next() != nullptr or n->prev() != nullptr)
			throw exception("attempt to add a node that already has a parent or siblings");

		n->parent(&m_element);

		if (pos == m_head)
		{
			n->next(m_head);
			m_head->prev(n);
			m_head = n;
		}
		else
		{
			n->prev(pos->prev());
			n->prev()->next(n);
			n->next(&(*pos));
			n->next()->prev(n);
		}

		return iterator(n);
	}

	iterator erase_impl(const_iterator pos)
	{
		if (pos == cend())
			return pos;

		if (pos->m_parent != &m_element)
			throw exception("attempt to remove node whose parent is invalid");

		node *n = pos;
		iterator result;

		if (m_head == n)
		{
			m_head = m_head->m_next;
			m_head->m_prev = nullptr;

			result = iterator(m_head);
		}
		else
		{
			result = iterator(n->next());

			n->next()->prev(n->prev());
			n->prev()->next(n->next());
		}


		n->next(nullptr);
		n->prev(nullptr);
		n->parent(nullptr);

		delete n;

		return result;
	}

  private:
	element &m_element;
	sentinel_node &m_sentinel;
	node *&m_head;
};

// // --------------------------------------------------------------------
// /// \brief implementation of basic_node_list for node objects

// class node_list : public basic_node_list<node>
// {
//   public:
// 	using basic_list = basic_node_list;

// 	using iterator = typename basic_list::iterator;
// 	using const_iterator = typename basic_list::const_iterator;

// 	node_list(element &e)
// 		: basic_list(e)
// 	{
// 	}

// 	// node_list(element &e, const node_list &l)
// 	// 	: basic_list(e)
// 	// {
// 	// 	for (auto &n : l)
// 	// 		emplace_back(n);
// 	// }

// 	// node_list(element &e, node_list &&l)
// 	// 	: basic_list(e)
// 	// {
// 	// 	for (auto &&n : l)
// 	// 		emplace_back(std::move(n));
// 	// }

// 	using basic_list::begin;
// 	using basic_list::clear;
// 	using basic_list::end;

// 	node_list &operator=(const node_list &l)
// 	{
// 		if (this != &l)
// 		{
// 			clear();

// 			for (auto &n : l)
// 				emplace_back(n);
// 		}

// 		return *this;
// 	}

// 	node_list &operator=(node_list &&l) noexcept
// 	{
// 		if (this != &l)
// 		{
// 			clear();
// 			swap(l);
// 		}

// 		return *this;
// 	}

// 	bool operator==(const node_list &l) const
// 	{
// 		bool result = true;
// 		auto a = begin(), b = l.begin();
// 		for (; result and a != end() and b != l.end(); ++a, ++b)
// 			result = a->equals(b.current());
// 		return result and a == end() and b == l.end();
// 	}

// 	bool operator!=(const node_list &l) const
// 	{
// 		return not operator==(l);
// 	}

// 	// insert a copy of e
// 	template <NodeType T>
// 	void insert(const_iterator pos, const T &e)
// 	{
// 		insert_impl(pos, new T(e));
// 	}

// 	// insert a copy of e, moving its data
// 	template <NodeType T>
// 	void insert(const_iterator pos, T &&e)
// 	{
// 		insert_impl(pos, new T(std::move(e)));
// 	}

// 	// iterator insert(const_iterator pos, size_t count, const value_type& n);

// 	template <typename InputIter>
// 		requires std::is_base_of_v<node, typename InputIter::value_type>
// 	iterator insert(const_iterator pos, InputIter first, InputIter last)
// 	{
// 		for (auto i = first; i != last; ++i, ++pos)
// 			insert(pos, *i);
// 		return pos;
// 	}

// 	template <NodeType T>
// 	iterator insert(const_iterator pos, std::initializer_list<T> nodes)
// 	{
// 		for (auto &n : nodes)
// 			pos = insert_impl(pos, new T(std::move(n)));
// 		return pos;
// 	}

// 	template <NodeType T>
// 	iterator emplace(const_iterator pos, const T &n)
// 	{
// 		auto i = insert_impl(pos, new T(n));
// 		return iterator(*this, i);
// 	}

// 	template <NodeType T>
// 	iterator emplace(const_iterator pos, T &&n)
// 	{
// 		auto i = insert_impl(pos, new T(std::move(n)));
// 		return iterator(*this, i);
// 	}

// 	iterator erase(const_iterator pos)
// 	{
// 		return erase_impl(pos);
// 	}

// 	iterator erase(iterator first, iterator last)
// 	{
// 		while (first != last)
// 		{
// 			auto next = first;
// 			++next;

// 			erase(first);
// 			first = next;
// 		}
// 		return last;
// 	}

// 	// iterator erase(const_iterator first, const_iterator last);

// 	template <NodeType T>
// 	void push_front(const T &e)
// 	{
// 		emplace(begin(), e);
// 	}

// 	template <NodeType T>
// 	void push_front(T &&e)
// 	{
// 		emplace(begin(), std::move(e));
// 	}

// 	template <typename... Args>
// 	node &emplace_front(Args &&...args)
// 	{
// 		return *emplace(begin(), std::forward<Args>(args)...);
// 	}

// 	void pop_front()
// 	{
// 		erase(begin());
// 	}

// 	template <NodeType T>
// 	void push_back(const T &e)
// 	{
// 		emplace(end(), e);
// 	}

// 	template <NodeType T>
// 	void push_back(T &&e)
// 	{
// 		emplace(end(), std::move(e));
// 	}

// 	template <NodeType T>
// 	T &emplace_back(const T &n)
// 	{
// 		auto i = insert_impl(end(), new T(n));
// 		return static_cast<T &>(*i);
// 	}

// 	template <NodeType T>
// 	T &emplace_back(T &&n)
// 	{
// 		auto i = insert_impl(end(), new T(std::move(n)));
// 		return static_cast<T &>(*i);
// 	}

// 	template <typename... Args>
// 	node &emplace_back(Args &&...args)
// 	{
// 		return *emplace(end(), std::forward<Args>(args)...);
// 	}

// 	void pop_back()
// 	{
// 		erase(std::prev(end()));
// 	}
// };

// // --------------------------------------------------------------------
// /// \brief set of attributes and name_spaces. Is a node_list but with a set interface

// class attribute_set : public basic_node_list<attribute>
// {
//   public:
// 	using node_list = basic_node_list<attribute>;

// 	using node_type = typename node_list::node_type;
// 	using iterator = typename node_list::iterator;
// 	using const_iterator = typename node_list::const_iterator;
// 	using size_type = std::size_t;

// 	attribute_set(element &e)
// 		: node_list(e)
// 	{
// 	}

// 	attribute_set(element &e, attribute_set &&as)
// 		: node_list(e)
// 	{
// 		for (auto &a : as)
// 			emplace(std::move(a));
// 	}

// 	attribute_set(element &e, const attribute_set &as)
// 		: node_list(e)
// 	{
// 		for (auto &a : as)
// 			emplace(a);
// 	}

// 	using node_list::clear;

// 	attribute_set &operator=(const attribute_set &l)
// 	{
// 		if (this != &l)
// 		{
// 			clear();

// 			for (auto &n : l)
// 				emplace(n);
// 		}

// 		return *this;
// 	}

// 	attribute_set &operator=(attribute_set &&l) noexcept
// 	{
// 		if (this != &l)
// 		{
// 			clear();
// 			swap(l);
// 		}

// 		return *this;
// 	}

// 	/// \brief attribute_set is a bit like a std::map and the key type is a std::string
// 	using key_type = std::string;

// 	/// \brief return true if the attribute with name \a key is defined
// 	bool contains(const key_type &key) const
// 	{
// 		return find(key) != end();
// 	}

// 	/// \brief return const_iterator to the attribute with name \a key
// 	const_iterator find(const key_type &key) const
// 	{
// 		const node_type *result = nullptr;
// 		for (auto &a : *this)
// 		{
// 			if (a.get_qname() == key)
// 			{
// 				result = &a;
// 				break;
// 			}
// 		}
// 		return const_iterator(*this, result);
// 	}

// 	/// \brief return iterator to the attribute with name \a key
// 	iterator find(const key_type &key)
// 	{
// 		return const_cast<const attribute_set &>(*this).find(key);
// 	}

// 	/// \brief emplace a newly constructed attribute with argumenst \a args
// 	template <typename... Args>
// 	std::pair<iterator, bool> emplace(Args... args)
// 	{
// 		node_type a(std::forward<Args>(args)...);
// 		return emplace(std::move(a));
// 	}

// 	/// \brief emplace an attribute move constructed from \a a
// 	/// \return returns a std::pair with an iterator pointing to the inserted attribute
// 	/// and a boolean indicating if this attribute was inserted instead of replaced.
// 	std::pair<iterator, bool> emplace(node_type &&a)
// 	{
// 		key_type key = a.get_qname();
// 		bool inserted = false;

// 		auto i = find(key);
// 		if (i != node_list::end())
// 			*i = std::move(a); // move assign value of a
// 		else
// 		{
// 			i = node_list::insert_impl(node_list::end(), new attribute(std::move(a)));
// 			inserted = true;
// 		}

// 		return std::make_pair(i, inserted);
// 	}

// 	/// \brief remove attribute at position \a pos
// 	iterator erase(const_iterator pos)
// 	{
// 		return node_list::erase_impl(pos);
// 	}

// 	/// \brief remove attributes between \a first and \a last
// 	iterator erase(iterator first, iterator last)
// 	{
// 		while (first != last)
// 		{
// 			auto next = first;
// 			++next;

// 			erase(first);
// 			first = next;
// 		}
// 		return last;
// 	}

// 	/// \brief remove attribute with name \a key
// 	size_type erase(const key_type key)
// 	{
// 		size_type result = 0;
// 		auto i = find(key);
// 		if (i != node_list::end())
// 		{
// 			erase(i);
// 			result = 1;
// 		}
// 		return result;
// 	}
// };

// --------------------------------------------------------------------
/// \brief the element class modelling a XML element
///
/// element is the most important zeep::xml::node object. It encapsulates a
/// XML element as found in the XML document. It has a qname, can have children,
/// attributes and a namespace.

class element : public node, public basic_node_list<element>
{
  public:
	// 	template <typename, typename>
	// 	friend class iterator_impl;
	// 	template <NodeType>
	// 	friend class basic_node_list;
	// 	friend class node_list;
	// 	friend class node;

	// element is a container of elements
	using value_type = element;
	using allocator_type = std::allocator<element>;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = element &;
	using const_reference = const element &;
	using pointer = element *;
	using const_pointer = const element *;
	using iterator = iterator_impl<element>;
	using const_iterator = iterator_impl<const element>;

	element()
		: basic_node_list<element>(*this, m_head, m_sentinel)
	{
	}

	// 	/// \brief constructor taking a \a qname and a list of \a attributes
	element(const std::string &qname /*, std::initializer_list<attribute> attributes = {}*/);

	element(std::initializer_list<element> il);

	element(const element &e);
	element(element &&e);
	element &operator=(const element &e);
	element &operator=(element &&e);

	~element();

	using node::set_qname;

	std::string get_qname() const override { return m_qname; }
	void set_qname(const std::string &qn) override { m_qname = qn; }

	// 	/// content of a xml:lang attribute of this element, or its nearest ancestor
	// 	virtual std::string lang() const;

	// 	/// content of the xml:id attribute, or the attribute that was defined to be
	// 	/// of type ID by the DOCTYPE.
	// 	std::string id() const;

	bool operator==(const element &e) const;
	bool operator!=(const element &e) const;

	// 	void swap(element &e) noexcept;

	// --------------------------------------------------------------------
	// children

	// 	node_list &nodes() { return m_nodes; }
	// 	const node_list &nodes() const { return m_nodes; }

	// 	using iterator = iterator_impl<element, node>;
	// 	using const_iterator = iterator_impl<const element, node>;

	iterator begin() { return iterator(m_head); }
	iterator end() { return iterator(&m_sentinel); }

	const_iterator begin() const { return const_iterator(m_head); }
	const_iterator end() const { return const_iterator(&m_sentinel); }

	const_iterator cbegin() { return const_iterator(m_head); }
	const_iterator cend() { return const_iterator(&m_sentinel); }

	element &front() { return *begin(); }
	const element &front() const { return *begin(); }

	element &back() { return *std::prev(end()); }
	const element &back() const { return *std::prev(end()); }

	// 	using node_iterator = node_list::iterator;
	// 	using const_node_iterator = node_list::const_iterator;

	// 	/// \brief insert a copy of \a e
	// 	void insert(const_iterator pos, const element &e)
	// 	{
	// 		emplace(pos, e);
	// 	}

	template <typename... Args>
		requires std::is_constructible_v<element, Args...>
	iterator emplace(const_iterator p, Args... args)
	{
		return insert_impl(p, new element(args...));
	}

	/// \brief insert a copy of \a e at position \a pos, moving its data
	void insert(const_iterator pos, element &&e)
	{
		emplace(pos, std::move(e));
	}

	// 	// iterator insert(const_iterator pos, size_t count, const value_type& n);

	// 	/// \brief insert copies of the nodes from \a first to \a last at position \a pos
	// 	template <typename InputIter>
	// 	iterator insert(const_iterator pos, InputIter first, InputIter last)
	// 	{
	// 		difference_type offset = pos - cbegin();
	// 		for (auto i = first; i != last; ++i, ++pos)
	// 			insert(pos, *i);
	// 		return begin() + offset;
	// 	}

	// 	/// \brief insert copies of the nodes in \a nodes at position \a pos
	// 	iterator insert(const_iterator pos, std::initializer_list<element> nodes)
	// 	{
	// 		return insert(pos, nodes.begin(), nodes.end());
	// 	}

	// 	/// \brief insert the data of node \a n at position \a pos, using move semantics
	// 	iterator emplace(const_iterator pos, text &&n)
	// 	{
	// 		return insert_impl(pos, new text(std::move(n)));
	// 	}

	// 	/// \brief insert the data of node \a n at position \a pos, using move semantics
	// 	iterator emplace(const_iterator pos, cdata &&n)
	// 	{
	// 		return insert_impl(pos, new cdata(std::move(n)));
	// 	}

	// 	/// \brief insert the data of node \a n at position \a pos, using move semantics
	// 	iterator emplace(const_iterator pos, comment &&n)
	// 	{
	// 		return insert_impl(pos, new comment(std::move(n)));
	// 	}

	// 	/// \brief insert the data of node \a n at position \a pos, using move semantics
	// 	iterator emplace(const_iterator pos, processing_instruction &&n)
	// 	{
	// 		return insert_impl(pos, new processing_instruction(std::move(n)));
	// 	}

	// 	/// \brief emplace a newly constructed element at \a pos using argument \a arg
	// 	template <typename Arg>
	// 		requires std::is_same_v<std::remove_cv_t<Arg>, element>
	// 	inline iterator emplace(const_iterator pos, Arg &&arg)
	// 	{
	// 		return insert_impl(pos, new element(std::forward<Arg>(arg)));
	// 	}

	// 	/// \brief emplace a newly constructed element at \a pos using arguments \a args
	// 	template <typename... Args>
	// 	inline iterator emplace(const_iterator pos, Args &&...args)
	// 	{
	// 		return insert_impl(pos, new element(std::forward<Args>(args)...));
	// 	}

	// 	/// \brief emplace a newly constructed element at \a pos using name \a name and attributes \a attrs
	// 	inline iterator emplace(const_iterator pos, const std::string &name,
	// 		std::initializer_list<attribute> attrs)
	// 	{
	// 		return insert_impl(pos, new element(name,
	// 									std::forward<std::initializer_list<attribute>>(attrs)));
	// 	}

	// 	/// \brief emplace an element at the front using arguments \a args
	// 	template <typename... Args>
	// 	inline element &emplace_front(Args &&...args)
	// 	{
	// 		return *emplace(begin(), std::forward<Args>(args)...);
	// 	}

	// 	/// \brief emplace a newly constructed element at the front using name \a name and attributes \a attrs
	// 	inline element &emplace_front(const std::string &name,
	// 		std::initializer_list<attribute> attrs)
	// 	{
	// 		return *emplace(begin(), name,
	// 			std::forward<std::initializer_list<attribute>>(attrs));
	// 	}

	// 	/// \brief emplace an element at the back using arguments \a args
	// 	template <typename... Args>
	// 	inline element &emplace_back(Args &&...args)
	// 	{
	// 		return *emplace(end(), std::forward<Args>(args)...);
	// 	}

	// 	/// \brief emplace a newly constructed element at the back using name \a name and attributes \a attrs
	// 	inline element &emplace_back(const std::string &name,
	// 		std::initializer_list<attribute> attrs)
	// 	{
	// 		return *emplace(end(), name,
	// 			std::forward<std::initializer_list<attribute>>(attrs));
	// 	}

	// 	/// \brief erase the node at \a pos
	// 	inline iterator erase(const_node_iterator pos)
	// 	{
	// 		return m_nodes.erase_impl(pos);
	// 	}

	// 	/// \brief erase the nodes from \a first to \a last
	// 	iterator erase(iterator first, iterator last)
	// 	{
	// 		while (first != last)
	// 		{
	// 			auto next = first;
	// 			++next;

	// 			erase(first);
	// 			first = next;
	// 		}
	// 		return last;
	// 	}

	// 	/// \brief erase the first node
	// 	inline void pop_front()
	// 	{
	// 		erase(begin());
	// 	}

	// 	/// \brief erase the last node
	// 	inline void pop_back()
	// 	{
	// 		erase(end() - 1);
	// 	}

	// 	/// \brief move the element \a e to the front of this element.
	// 	inline void push_front(element &&e)
	// 	{
	// 		emplace(begin(), std::move(e));
	// 	}

	// 	/// \brief copy the element \a e to the front of this element.
	// 	inline void push_front(const element &e)
	// 	{
	// 		emplace(begin(), e);
	// 	}

	// 	/// \brief move the element \a e to the back of this element.
	// 	inline void push_back(element &&e)
	// 	{
	// 		emplace(end(), std::move(e));
	// 	}

	// 	/// \brief copy the element \a e to the back of this element.
	// 	inline void push_back(const element &e)
	// 	{
	// 		emplace(end(), e);
	// 	}

	// 	/// \brief remove all nodes
	// 	void clear();

	// 	size_t size() const { return std::distance(begin(), end()); }
	// 	bool empty() const { return size() == 0; }

	// 	// --------------------------------------------------------------------
	// 	// attribute support

	// 	/// \brief return the set of attributes for this element
	// 	attribute_set &attributes() { return m_attributes; }

	// 	/// \brief return the set of attributes for this element
	// 	const attribute_set &attributes() const { return m_attributes; }

	// 	// --------------------------------------------------------------------

	// 	/// \brief write the element to \a os
	// 	friend std::ostream &operator<<(std::ostream &os, const element &e);
	// 	friend class document;

	/// \brief will return the concatenation of str() from all child nodes
	std::string str() const override;

	// 	/// \brief return the URI of the namespace for \a prefix
	// 	virtual std::string namespace_for_prefix(const std::string &prefix) const;

	// 	/// \brief return the prefix for the XML namespace with uri \a uri.
	// 	/// \return The result is a pair of a std::string containing the actual prefix value
	// 	/// and a boolean indicating if the namespace was found at all, needed since empty prefixes
	// 	/// are allowed.
	// 	virtual std::pair<std::string, bool> prefix_for_namespace(const std::string &uri) const;

	// 	/// \brief move this element and optionally everyting beneath it to the
	// 	///        specified namespace/prefix
	// 	///
	// 	/// \param prefix				The new prefix name
	// 	/// \param uri					The new namespace uri
	// 	/// \param recursive			Apply this to the child nodes as well
	// 	/// \param including_attributes	Move the attributes to this new namespace as well
	// 	void move_to_name_space(const std::string &prefix, const std::string &uri,
	// 		bool recursive, bool including_attributes);

	// 	/// \brief return the concatenation of the content of all enclosed zeep::xml::text nodes
	// 	std::string get_content() const;

	// 	/// \brief replace all existing child text nodes with a new single text node containing \a content
	// 	void set_content(const std::string &content);

	// 	/// \brief return the value of attribute name \a qname or the empty string if not found
	// 	std::string get_attribute(const std::string &qname) const;

	// 	/// \brief set the value of attribute named \a qname to the value \a value
	// 	void set_attribute(const std::string &qname, const std::string &value);

	// 	/// \brief The set_text method replaces any text node with the new text (call set_content)
	// 	virtual void set_text(const std::string &s);

	// 	/// The add_text method checks if the last added child is a text node,
	// 	/// and if so, it appends the string to this node's value. Otherwise,
	// 	/// it adds a new text node child with the new text.
	// 	void add_text(const std::string &s);

	// 	/// To combine all adjacent child text nodes into one
	// 	void flatten_text();

	// 	/// xpath wrappers
	// 	/// TODO: create recursive iterator and use it as return type here

	// 	/// \brief return the elements that match XPath \a path.
	// 	///
	// 	/// If you need to find other classes than xml::element, of if your XPath
	// 	/// contains variables, you should create a zeep::xml::xpath object and use
	// 	/// its evaluate method.
	// 	element_set find(const std::string &path) const { return find(path.c_str()); }

	// 	/// \brief return the first element that matches XPath \a path.
	// 	///
	// 	/// If you need to find other classes than xml::element, of if your XPath
	// 	/// contains variables, you should create a zeep::xml::xpath object and use
	// 	/// its evaluate method.
	// 	iterator find_first(const std::string &path) { return find_first(path.c_str()); }
	// 	const_iterator find_first(const std::string &path) const
	// 	{
	// 		return const_cast<element *>(this)->find_first(path);
	// 	}

	// 	/// \brief return the elements that match XPath \a path.
	// 	///
	// 	/// If you need to find other classes than xml::element, of if your XPath
	// 	/// contains variables, you should create a zeep::xml::xpath object and use
	// 	/// its evaluate method.
	// 	element_set find(const char *path) const;

	// 	/// \brief return the first element that matches XPath \a path.
	// 	///
	// 	/// If you need to find other classes than xml::element, of if your XPath
	// 	/// contains variables, you should create a zeep::xml::xpath object and use
	// 	/// its evaluate method.
	// 	iterator find_first(const char *path);
	// 	const_iterator find_first(const char *path) const
	// 	{
	// 		return const_cast<element *>(this)->find_first(path);
	// 	}

	// 	// debug routine
	// 	virtual void validate();

  protected:
	void write(std::ostream &os, format_info fmt) const override;

	// // bottleneck to validate insertions (e.g. document may have only one child element)
	// iterator insert_impl(const_iterator pos, node *n)
	// {
	// 	assert(n != nullptr);
	// 	assert(n->next() == nullptr);
	// 	assert(n->prev() == nullptr);

	// 	if (n == nullptr)
	// 		throw exception("Invalid pointer passed to insert");

	// 	if (n->parent() != nullptr or n->next() != nullptr or n->prev() != nullptr)
	// 		throw exception("attempt to add a node that already has a parent or siblings");

	// 	n->parent(&m_element);

	// 	// insert at end, most often this is the case
	// 	if (pos.m_current == nullptr)
	// 	{
	// 		if (m_head == nullptr)
	// 			m_tail = m_head = n;
	// 		else
	// 		{
	// 			m_tail->insert_sibling(n, nullptr);
	// 			m_tail = n;
	// 		}
	// 	}
	// 	else
	// 	{
	// 		assert(m_head != nullptr);

	// 		if (pos.m_current == m_head)
	// 		{
	// 			n->m_next = m_head;
	// 			m_head->m_prev = n;
	// 			m_head = n;
	// 		}
	// 		else
	// 			m_head->insert_sibling(n, pos.m_current);
	// 	}

	// 	// #if defined(DEBUG)
	// 	// 		validate();
	// 	// #endif

	// 	return iterator(*this, n);
	// }

  private:
	std::string m_qname;
	sentinel_node m_sentinel;
	node *m_head = &m_sentinel;
	// attribute_set m_attributes;
};

// // --------------------------------------------------------------------

// template <>
// inline void iterator_impl<element, node>::skip()
// {
// 	while (m_current != nullptr)
// 	{
// 		if (dynamic_cast<element *>(m_current) != nullptr)
// 			break;
// 		m_current = m_current->next();
// 	}
// }

// template <>
// inline void iterator_impl<const element, node>::skip()
// {
// 	while (m_current != nullptr)
// 	{
// 		if (dynamic_cast<element *>(m_current) != nullptr)
// 			break;
// 		m_current = m_current->next();
// 	}
// }

template <typename T>
iterator_impl<T>::iterator_impl(node *current)
	: m_current(current)
{
	if constexpr (std::is_same_v<node_type, element>)
	{
		while (m_current)
		{
			if (typeid(*m_current) == typeid(element))
				continue;
			m_current = m_current->next();
		}
	}
}

template <typename T>
iterator_impl<T>::iterator_impl(const node *current)
	: m_current(current)
{
	if constexpr (std::is_same_v<node_type, element>)
	{
		while (m_current)
		{
			if (typeid(*m_current) == typeid(element))
				continue;
			m_current = m_current->next();
		}
	}
}

template <typename T>
iterator_impl<T> &iterator_impl<T>::operator++()
{
	while (m_current)
	{
		m_current = m_current->next();
		if constexpr (std::is_same_v<node_type, element>)
		{
			if (typeid(*m_current) != typeid(element))
				continue;
		}
	}

	return *this;
}

template <typename T>
iterator_impl<T> &iterator_impl<T>::operator--()
{
	while (m_current)
	{
		m_current = m_current->prev();
		if constexpr (std::is_same_v<node_type, element>)
		{
			if (typeid(*m_current) != typeid(element))
				continue;
		}
	}

	return *this;
}

// // --------------------------------------------------------------------

// /// \brief This method fixes namespace attribute when transferring an element
// /// 	   from one document to another (replaces prefixes e.g.)
// ///
// /// When moving an element from one document to another, we need to fix the
// /// namespaces, make sure the destination has all the namespace specifications
// /// required by the element and make sure the prefixes used are correct.
// /// \param e		The element that is being transferred
// /// \param source	The (usually) document element that was the source
// /// \param dest		The (usually) document element that is the destination

// void fix_namespaces(element &e, element &source, element &dest);

// } // namespace mxml

// // --------------------------------------------------------------------
// // structured binding support

// namespace std
// {

// template <>
// struct tuple_size<::mxml::attribute>
// 	: public std::integral_constant<std::size_t, 2>
// {
// };

// template <>
// struct tuple_element<0, ::mxml::attribute>
// {
// 	using type = decltype(std::declval<::mxml::attribute>().name());
// };

// template <>
// struct tuple_element<1, ::mxml::attribute>
// {
// 	using type = decltype(std::declval<::mxml::attribute>().value());
// };

} // namespace mxml