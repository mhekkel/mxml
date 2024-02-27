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
#include <utility>

export module mxml:node;

import :error;

namespace mxml
{

// forward declarations

class node;

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
	virtual void set_qname(std::string_view qn) {}

	/// \brief set the qname with two parameters, if \a prefix is empty the qname will be simply \a name
	/// otherwise the name will be `prefix:name`
	/// \param prefix	The namespace prefix to use
	/// \param name		The actual name to use
	void set_qname(std::string_view prefix, std::string_view name)
	{
		set_qname(prefix.empty() ? name : std::string{ prefix } + ':' + std::string{ name });
	}

	virtual std::string name() const;       ///< The name for the node as parsed from the qname.
	virtual std::string get_prefix() const; ///< The prefix for the node as parsed from the qname.
	virtual std::string get_ns() const;     ///< Returns the namespace URI for the node, if it can be resolved.

	/// Return the namespace URI for a prefix
	virtual std::string namespace_for_prefix(std::string_view prefix) const;

	/// Return the prefix for a namespace URI
	virtual std::pair<std::string, bool> prefix_for_namespace(std::string_view uri) const;

	/// Prefix the \a tag with the namespace prefix for \a uri
	virtual std::string prefix_tag(std::string_view tag, std::string_view uri) const;

	/// return all content concatenated, including that of children.
	virtual std::string str() const { return {}; }

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

	/// \brief low level routine for writing out XML
	///
	/// This method is usually called by operator<<(std::ostream&, zeep::xml::document&)
	virtual void write(std::ostream &os, format_info fmt) const {}

  protected:
	template <typename>
	friend class basic_node_list;
	friend class element;

	node()
	{
		m_next = m_prev = this;
	}

	node(const node &n) = delete;
	node(node &&n) = delete;
	node &operator=(const node &n) = delete;
	node &operator=(node &&n) = delete;

	void parent(element *p) { m_parent = p; }
	void next(const node *n) { m_next = const_cast<node *>(n); }
	void prev(const node *n) { m_prev = const_cast<node *>(n); }

  protected:
	element *m_parent = nullptr;
	node *m_next;
	node *m_prev;
};

// --------------------------------------------------------------------
/// internal node base class for storing text

class node_with_text : public node
{
  public:
	node_with_text() = default;

	node_with_text(std::string_view s)
		: m_text(s)
	{
	}

	node_with_text(const node_with_text &n)
		: m_text(n.m_text)
	{
	}

	node_with_text(node_with_text &&n) noexcept
	{
		swap(*this, n);
	}

	friend void swap(node_with_text &a, node_with_text &b) noexcept
	{
		std::swap(a.m_text, b.m_text);
	}

	/// \brief return the text content
	std::string str() const override { return m_text; }

	/// \brief return the text content, same as str()
	virtual std::string get_text() const { return m_text; }

	/// \brief set the text content
	virtual void set_text(std::string_view text) { m_text = text; }

  protected:
	std::string m_text;
};

// --------------------------------------------------------------------
/// A node containing a XML comment

export class comment final : public node_with_text
{
  public:
	comment() = default;

	comment(const comment &c)
		: node_with_text(c)
	{
	}

	comment(std::string_view text)
		: node_with_text(text)
	{
	}

	comment(comment &&c) noexcept
	{
		swap(*this, c);
	}

	comment &operator=(comment c)
	{
		swap(*this, c);
		return *this;
	}

	/// \brief compare nodes for equality
	bool equals(const node *n) const override;

	void write(std::ostream &os, format_info fmt) const override;
};

// --------------------------------------------------------------------
/// A node containing a XML processing instruction (like e.g. \<?php ?\>)

export class processing_instruction final : public node_with_text
{
  public:
	processing_instruction() = default;

	/// \brief constructor with parameters
	///
	/// This constructs a processing instruction with the specified parameters
	/// \param target	The target, this will follow the <? characters, e.g. `php` will generate <?php ... ?>
	/// \param text		The text inside this node, e.g. the PHP code.
	processing_instruction(std::string_view target, std::string_view text)
		: node_with_text(text)
		, m_target(target)
	{
	}

	processing_instruction(const processing_instruction &pi)
		: node_with_text(pi)
		, m_target(pi.m_target)
	{
	}

	processing_instruction(processing_instruction &&pi) noexcept
		: node_with_text(std::move(pi))
		, m_target(std::move(pi.m_target))
	{
	}

	processing_instruction &operator=(processing_instruction pi)
	{
		swap(*this, pi);
		return *this;
	}

	friend void swap(processing_instruction &a, processing_instruction &b) noexcept
	{
		swap(static_cast<node_with_text &>(a), static_cast<node_with_text &>(b));
		std::swap(a.m_target, b.m_target);
	}

	/// \brief return the qname which is the same as the target in this case
	std::string get_qname() const override { return m_target; }

	/// \brief return the target
	std::string get_target() const { return m_target; }

	/// \brief set the target
	void set_target(std::string_view target) { m_target = target; }

	/// \brief compare nodes for equality
	bool equals(const node *n) const override;

	void write(std::ostream &os, format_info fmt) const override;

  private:
	std::string m_target;
};

// --------------------------------------------------------------------
/// A node containing text.

export class text : public node_with_text
{
  public:
	text() {}

	text(std::string_view text)
		: node_with_text(text)
	{
	}

	text(text &&t) noexcept
		: node_with_text(std::move(t.m_text))
	{
	}

	/// \brief append \a text to the stored text
	void append(std::string_view text) { m_text.append(text); }

	/// \brief compare nodes for equality
	bool equals(const node *n) const override;

	/// \brief returns true if this text contains only whitespace characters
	bool is_space() const;

	void write(std::ostream &os, format_info fmt) const override;
};

// --------------------------------------------------------------------
/// A node containing the contents of a CDATA section. Normally, these nodes are
/// converted to text nodes but you can specify to preserve them when parsing a
/// document.

export class cdata final : public text
{
  public:
	cdata() {}
	cdata(cdata &&cd) noexcept
		: text(std::move(cd))
	{
	}
	cdata(std::string_view s)
		: text(s)
	{
	}

	/// \brief compare nodes for equality
	bool equals(const node *n) const override;

	void write(std::ostream &os, format_info fmt) const override;
};

// --------------------------------------------------------------------
/// An attribute is a node, has an element as parent, but is not a child of this parent (!)

export class attribute final : public node
{
  public:
	attribute(const attribute &attr)
		: m_qname(attr.m_qname)
		, m_value(attr.m_value)
		, m_id(attr.m_id)
	{
	}

	attribute(attribute &&attr) noexcept
		: m_qname(std::move(attr.m_qname))
		, m_value(std::move(attr.m_value))
		, m_id(attr.m_id)
	{
	}

	attribute(std::string_view qname, std::string_view value, bool id = false)
		: m_qname(qname)
		, m_value(value)
		, m_id(id)
	{
	}

	attribute &operator=(attribute attr) noexcept
	{
		swap(*this, attr);
		return *this;
	}

	friend void swap(attribute &a, attribute &b)
	{
		std::swap(a.m_qname, b.m_qname);
		std::swap(a.m_value, b.m_value);
		std::swap(a.m_id, b.m_id);
	}

	std::strong_ordering operator<=>(const attribute &a) const
	{
		if (auto cmp = *this <=> a; cmp != 0)
			return cmp;
		return m_value <=> a.m_value;
	}

	std::string get_qname() const override { return m_qname; }
	void set_qname(std::string_view qn) override { m_qname = qn; }

	using node::set_qname;

	/// \brief Is this attribute an xmlns attribute?
	bool is_namespace() const
	{
		return m_qname.compare(0, 5, "xmlns") == 0 and (m_qname[5] == 0 or m_qname[5] == ':');
	}

	std::string value() const { return m_value; }
	void set_value(std::string_view v) { m_value = v; }

	/// \brief same as value, but checks to see if this really is a namespace attribute
	std::string uri() const;

	std::string str() const override { return m_value; }

	/// \brief compare nodes for equality
	bool equals(const node *n) const override;

	/// \brief returns whether this attribute is an ID attribute, as defined in an accompanying DTD
	bool is_id() const { return m_id; }

	/// \brief support for structured binding
	template <size_t N>
	decltype(auto) get() const
	{
		if constexpr (N == 0)
			return name();
		else if constexpr (N == 1)
			return value();
	}

	void write(std::ostream &os, format_info fmt) const override;

  private:
	std::string m_qname, m_value;
	bool m_id;
};

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

	template <typename T2>
	friend class iterator_impl;

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
	// requires std::is_same_v<pointer, std::remove_const_t<typename T2::pointer>>
	iterator_impl(const iterator_impl<T2> &i)
		: m_current(const_cast<node *>(i.m_current))
	{
	}

	template <typename Iterator>
		requires(not std::is_same_v<std::remove_const_t<typename Iterator::value_type>, element> and
				 std::is_base_of_v<value_type, typename Iterator::value_type>)
	iterator_impl(const Iterator &i)
		: m_current(i.m_current)
	{
	}

	iterator_impl &operator=(iterator_impl i)
	{
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

	template <NodeType T2>
	bool operator==(const T2 *n) const { return m_current == n; }

	explicit operator pointer() const { return static_cast<pointer>(m_current); }
	explicit operator pointer() { return static_cast<pointer>(m_current); }

  private:
	using node_base_type = std::conditional_t<std::is_const_v<T>, const node, node>;

	node_base_type *m_current = nullptr;
};

// --------------------------------------------------------------------

template <typename T>
class basic_node_list
{
  public:
	using node_type = T;

	using value_type = node_type;
	using allocator_type = std::allocator<value_type>;
	using size_type = size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;

	basic_node_list(node *n)
		: m_node(n)
		, m_owner(false)
	{
	}

	virtual ~basic_node_list()
	{
		if (m_owner)
		{
			clear();
			delete m_node;
		}
	}

	// bool operator==(const basic_node_list &l) const
	// {
	// 	bool result = true;
	// 	auto a = begin(), b = l.begin();
	// 	for (; result and a != end() and b != l.end(); ++a, ++b)
	// 		result = a->equals(b.current());
	// 	return result and a == end() and b == l.end();
	// }

	using iterator = iterator_impl<node_type>;
	using const_iterator = iterator_impl<const node_type>;

	iterator begin() { return iterator(m_node->m_next); }
	iterator end() { return iterator(m_node); }

	const_iterator cbegin() { return const_iterator(m_node->m_next); }
	const_iterator cend() { return const_iterator(m_node); }

	const_iterator begin() const { return const_iterator(m_node->m_next); }
	const_iterator end() const { return const_iterator(m_node); }

	value_type &front() { return *begin(); }
	const value_type &front() const { return *begin(); }

	value_type &back() { return *std::prev(end()); }
	const value_type &back() const { return *std::prev(end()); }

	bool empty() const { return m_node->m_next == m_node; }
	size_t size() const { return std::distance(begin(), end()); }

	/// \brief insert a copy of \a e
	iterator insert(const_iterator pos, const node_type &e)
	{
		return insert_impl(pos, new node_type(e));
	}

	/// \brief insert a copy of \a e at position \a pos, moving its data
	iterator insert(const_iterator pos, node_type &&e)
	{
		return insert_impl(pos, new node_type(std::move(e)));
	}

	iterator insert(const_iterator pos, size_t count, const node_type &n)
	{
		iterator p(const_cast<node_type *>(&*pos));
		while (count-- > 0)
			p = insert(p, n);
		return p;
	}

	/// \brief insert copies of the nodes from \a first to \a last at position \a pos
	template <typename InputIter>
	iterator insert(const_iterator pos, InputIter first, InputIter last)
	{
		iterator p(const_cast<node_type *>(&*pos));
		iterator result = p;
		bool f = true;
		for (auto i = first; i != last; ++i, ++p)
		{
			p = insert(p, *i);
			if (std::exchange(f, false))
				result = p;
		}
		return result;
	}

	/// \brief insert copies of the nodes in \a nodes at position \a pos
	iterator insert(const_iterator pos, std::initializer_list<node_type> nodes)
	{
		return insert(pos, nodes.begin(), nodes.end());
	}

	/// \brief replace content with copies of the nodes from \a first to \a last
	template <typename InputIter>
	void assign(InputIter first, InputIter last)
	{
		clear();
		insert(begin(), first, last);
	}

	template <typename TN>
		requires std::is_base_of_v<node_type, std::remove_cvref_t<TN>>
	reference emplace(const_iterator p, TN &&n)
	{
		return *insert_impl(p, new node_type(std::move(n)));
	}

	template <typename... Args>
		requires std::is_constructible_v<node_type, Args...>
	reference emplace(const_iterator p, Args... args)
	{
		return *insert_impl(p, new node_type(args...));
	}

	/// \brief emplace an element at the front using arguments \a args
	template <typename... Args>
	node_type &emplace_front(Args &&...args)
	{
		return emplace(begin(), std::forward<Args>(args)...);
	}

	/// \brief emplace an element at the back using arguments \a args
	template <typename... Args>
	node_type &emplace_back(Args &&...args)
	{
		return emplace(end(), std::forward<Args>(args)...);
	}

	/// \brief erase the node at \a pos
	iterator erase(const_iterator pos)
	{
		return erase_impl(pos);
	}

	/// \brief erase the nodes from \a first to \a last
	iterator erase(iterator first, iterator last)
	{
		while (first != last)
		{
			auto next = first;
			++next;

			erase(first);
			first = next;
		}
		return last;
	}

	/// \brief erase the first node
	void pop_front()
	{
		erase(begin());
	}

	/// \brief erase the last node
	void pop_back()
	{
		erase(std::prev(end()));
	}

	/// \brief move the node_type \a e to the front of this node_type.
	void push_front(node_type &&e)
	{
		emplace(begin(), std::move(e));
	}

	/// \brief copy the node_type \a e to the front of this node_type.
	void push_front(const node_type &e)
	{
		emplace(begin(), e);
	}

	/// \brief move the node_type \a e to the back of this node_type.
	void push_back(node_type &&e)
	{
		emplace(end(), std::move(e));
	}

	/// \brief copy the node_type \a e to the back of this node_type.
	void push_back(const node_type &e)
	{
		emplace(end(), e);
	}

	/// \brief remove all nodes
	void clear()
	{
		// avoid deep recursion and stack overflows
		auto n = m_node->m_next;

		assert(n != nullptr);

		while (n != m_node)
		{
			auto t = n->m_next;
			delete n;
			n = t;
			assert(n != nullptr);
		}

		m_node->m_next = m_node->m_prev = m_node;
	}

  protected:
	basic_node_list(element *e)
		: m_node(new node)
		, m_owner(true)
	{
		m_node->m_parent = e;
	}

	basic_node_list(const basic_node_list &nl) = delete;
	basic_node_list(basic_node_list &&nl) = delete;
	basic_node_list &operator=(const basic_node_list &nl) = delete;
	basic_node_list &operator=(basic_node_list &&nl) = delete;

	friend void swap(basic_node_list &a, basic_node_list &b)
	{
		std::swap(a.m_node, b.m_node);
		std::swap(a.m_node->m_parent, b.m_node->m_parent);

		for (node *n = a.m_node->m_next; n != a.m_node; n = n->m_next)
			n->parent(a.m_node->m_parent);

		for (node *n = b.m_node->m_next; n != b.m_node; n = n->m_next)
			n->parent(b.m_node->m_parent);
	}

  protected:
	// proxy methods for every insertion

	virtual iterator insert_impl(const_iterator pos, node *n)
	{
		assert(n != nullptr);
		assert(n->next() == n);
		assert(n->prev() == n);

		if (n == nullptr)
			throw exception("Invalid pointer passed to insert");

		if (n->parent() != nullptr or n->next() != n or n->prev() != n)
			throw exception("attempt to add a node that already has a parent or siblings");

		n->parent(m_node->m_parent);

		auto p = &*pos;

		n->prev(p->prev());
		n->prev()->next(n);
		n->next(p);
		n->next()->prev(n);

		return iterator(n);
	}

	iterator erase_impl(const_iterator pos)
	{
		if (pos == cend())
			return pos;

		if (pos->m_parent != m_node->m_parent)
			throw exception("attempt to remove node whose parent is invalid");

		node *n = const_cast<node_type *>((const node_type *)pos);
		iterator result;

		result = iterator(n->next());

		n->next()->prev(n->prev());
		n->prev()->next(n->next());

		n->next(nullptr);
		n->prev(nullptr);
		n->parent(nullptr);
		delete n;

		return result;
	}

	node *m_node;
	bool m_owner;
};

// --------------------------------------------------------------------
/// \brief set of attributes and name_spaces. Is a node_list but with a set interface

class attribute_set : public basic_node_list<attribute>
{
  public:
	using node_list = basic_node_list<attribute>;

	using iterator = typename node_list::iterator;
	using const_iterator = typename node_list::const_iterator;
	using size_type = std::size_t;

	attribute_set(element *el)
		: basic_node_list(el)
	{
	}

	attribute_set(element *el, const attribute_set &as);

	attribute_set &operator=(attribute_set as)
	{
		swap(*this, as);
		return *this;
	}

	friend void swap(attribute_set &a, attribute_set &b)
	{
		swap(static_cast<basic_node_list<attribute> &>(a), static_cast<basic_node_list<attribute> &>(b));
	}

	/// \brief return true if the attribute with name \a key is defined
	bool contains(std::string_view key) const
	{
		return find(key) != end();
	}

	/// \brief return const_iterator to the attribute with name \a key
	const_iterator find(std::string_view key) const
	{
		for (auto i = begin(); i != end(); ++i)
		{
			if (i->get_qname() == key)
				return i;
		}
		return end();
	}

	/// \brief return iterator to the attribute with name \a key
	iterator find(std::string_view key)
	{
		return const_cast<const attribute_set &>(*this).find(key);
	}

	/// \brief emplace a newly constructed attribute with argumenst \a args
	template <typename... Args>
	std::pair<iterator, bool> emplace(Args... args)
	{
		node_type a(std::forward<Args>(args)...);
		return emplace(std::move(a));
	}

	/// \brief emplace an attribute move constructed from \a a
	/// \return returns a std::pair with an iterator pointing to the inserted attribute
	/// and a boolean indicating if this attribute was inserted instead of replaced.
	std::pair<iterator, bool> emplace(node_type &&a)
	{
		bool inserted = false;

		auto i = find(a.get_qname());

		if (i != node_list::end())
			*i = std::move(a); // move assign value of a
		else
		{
			i = node_list::insert_impl(node_list::end(), new attribute(std::move(a)));
			inserted = true;
		}

		return std::make_pair(i, inserted);
	}

	using node_list::erase;

	/// \brief remove attribute with name \a key
	size_type erase(std::string_view key)
	{
		size_type result = 0;
		auto i = find(key);
		if (i != node_list::end())
		{
			erase(i);
			result = 1;
		}
		return result;
	}
};

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
		: basic_node_list(this)
		, m_attributes(this)
	{
	}

	// 	/// \brief constructor taking a \a qname and a list of \a attributes
	element(std::string_view qname, std::initializer_list<attribute> attributes = {})
		: basic_node_list(this)
		, m_qname(qname)
		, m_attributes(this)
	{
		m_attributes.assign(attributes.begin(), attributes.end());
	}

	element(std::initializer_list<element> il);

	element(const element &e);

	element(element &&e)
		: basic_node_list(this)
		, m_attributes(this)
	{
		swap(*this, e);
	}

	element &operator=(element e)
	{
		swap(*this, e);
		return *this;
	}

	~element();

	friend void swap(element &a, element &b) noexcept;

	using node::set_qname;

	std::string get_qname() const override { return m_qname; }
	void set_qname(std::string_view qn) override { m_qname = qn; }

	/// content of a xml:lang attribute of this element, or its nearest ancestor
	std::string lang() const override;

	/// content of the xml:id attribute, or the attribute that was defined to be
	/// of type ID by the DOCTYPE.
	std::string id() const;

	bool operator==(const element &e) const;

	// --------------------------------------------------------------------
	// children

	basic_node_list<node> nodes() { return basic_node_list<node>(m_node); }
	const basic_node_list<node> nodes() const { return basic_node_list<node>(m_node); }

	// --------------------------------------------------------------------
	// attribute support

	/// \brief return the set of attributes for this element
	attribute_set &attributes() { return m_attributes; }

	/// \brief return the set of attributes for this element
	const attribute_set &attributes() const { return m_attributes; }

	// --------------------------------------------------------------------

	/// \brief write the element to \a os
	friend std::ostream &operator<<(std::ostream &os, const element &e);
	// 	friend class document;

	/// \brief will return the concatenation of str() from all child nodes
	std::string str() const override;

	/// \brief return the URI of the namespace for \a prefix
	std::string namespace_for_prefix(std::string_view prefix) const override;

	/// \brief return the prefix for the XML namespace with uri \a uri.
	/// \return The result is a pair of a std::string containing the actual prefix value
	/// and a boolean indicating if the namespace was found at all, needed since empty prefixes
	/// are allowed.
	std::pair<std::string, bool> prefix_for_namespace(std::string_view uri) const override;

	/// \brief move this element and optionally everyting beneath it to the
	///        specified namespace/prefix
	///
	/// \param prefix				The new prefix name
	/// \param uri					The new namespace uri
	/// \param recursive			Apply this to the child nodes as well
	/// \param including_attributes	Move the attributes to this new namespace as well
	void move_to_name_space(std::string_view prefix, std::string_view uri,
		bool recursive, bool including_attributes);

	/// \brief return the concatenation of the content of all enclosed zeep::xml::text nodes
	std::string get_content() const;

	/// \brief replace all existing child text nodes with a new single text node containing \a content
	void set_content(std::string_view content);

	/// \brief return the value of attribute name \a qname or the empty string if not found
	std::string get_attribute(std::string_view qname) const;

	/// \brief set the value of attribute named \a qname to the value \a value
	void set_attribute(std::string_view qname, std::string_view value);

	/// \brief The set_text method replaces any text node with the new text (call set_content)
	virtual void set_text(std::string_view s);

	/// The add_text method checks if the last added child is a text node,
	/// and if so, it appends the string to this node's value. Otherwise,
	/// it adds a new text node child with the new text.
	void add_text(std::string_view s);

	/// To combine all adjacent child text nodes into one
	void flatten_text();

	// 	/// xpath wrappers
	// 	/// TODO: create recursive iterator and use it as return type here

	// 	/// \brief return the elements that match XPath \a path.
	// 	///
	// 	/// If you need to find other classes than xml::element, of if your XPath
	// 	/// contains variables, you should create a zeep::xml::xpath object and use
	// 	/// its evaluate method.
	// 	element_set find(std::string_view path) const { return find(path.c_str()); }

	// 	/// \brief return the first element that matches XPath \a path.
	// 	///
	// 	/// If you need to find other classes than xml::element, of if your XPath
	// 	/// contains variables, you should create a zeep::xml::xpath object and use
	// 	/// its evaluate method.
	// 	iterator find_first(std::string_view path) { return find_first(path.c_str()); }
	// 	const_iterator find_first(std::string_view path) const
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

	void write(std::ostream &os, format_info fmt) const override;

  private:
	std::string m_qname;
	attribute_set m_attributes;
};

// --------------------------------------------------------------------

template <typename T>
iterator_impl<T>::iterator_impl(node *current)
	: m_current(current)
{
	if constexpr (std::is_same_v<node_type, element>)
	{
		while (typeid(*m_current) != typeid(element) and typeid(*m_current) != typeid(node))
			m_current = m_current->next();
	}
}

template <typename T>
iterator_impl<T>::iterator_impl(const node *current)
	: m_current(current)
{
	if constexpr (std::is_same_v<node_type, element>)
	{
		while (typeid(*m_current) != typeid(element) and typeid(*m_current) != typeid(node))
			m_current = m_current->next();
	}
}

template <typename T>
iterator_impl<T> &iterator_impl<T>::operator++()
{
	m_current = m_current->next();
	if constexpr (std::is_same_v<node_type, element>)
	{
		while (typeid(*m_current) != typeid(element) and typeid(*m_current) != typeid(node))
			m_current = m_current->next();
	}

	return *this;
}

template <typename T>
iterator_impl<T> &iterator_impl<T>::operator--()
{
	m_current = m_current->prev();
	if constexpr (std::is_same_v<node_type, element>)
	{
		while (typeid(*m_current) != typeid(element) and typeid(*m_current) != typeid(node))
			m_current = m_current->prev();
	}

	return *this;
}

// --------------------------------------------------------------------

/// \brief This method fixes namespace attribute when transferring an element
/// 	   from one document to another (replaces prefixes e.g.)
///
/// When moving an element from one document to another, we need to fix the
/// namespaces, make sure the destination has all the namespace specifications
/// required by the element and make sure the prefixes used are correct.
/// \param e		The element that is being transferred
/// \param source	The (usually) document element that was the source
/// \param dest		The (usually) document element that is the destination

void fix_namespaces(element &e, element &source, element &dest);

} // namespace mxml

// --------------------------------------------------------------------
// structured binding support

namespace std
{

template <>
struct tuple_size<::mxml::attribute>
	: public std::integral_constant<std::size_t, 2>
{
};

template <>
struct tuple_element<0, ::mxml::attribute>
{
	using type = decltype(std::declval<::mxml::attribute>().name());
};

template <>
struct tuple_element<1, ::mxml::attribute>
{
	using type = decltype(std::declval<::mxml::attribute>().value());
};

} // namespace std
