// Copyright (c) 2024-2026 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

/// \file
/// the core of the zeem XML library defining the main classes in the DOM API

#ifndef ZEEM_CXX_MODULE
# include "zeem/export.hpp"
# include "zeem/error.hpp"
# include "zeem/version.hpp"

# include <algorithm>
# include <cassert>
# include <compare>
# include <cstddef>
# include <cstdint>
# include <initializer_list>
# include <iosfwd>
# include <iterator>
# include <memory>
# include <string>
# include <string_view>
# include <type_traits>
# include <utility>
# include <vector>
#endif

namespace zeem
{

// forward declarations
ZEEM_EXPORT class attribute;
ZEEM_EXPORT class attribute_set;
ZEEM_EXPORT class context;
ZEEM_EXPORT class element;
ZEEM_EXPORT class element_container;
ZEEM_EXPORT class node;
ZEEM_EXPORT class text;
ZEEM_EXPORT class xpath;

ZEEM_EXPORT template <typename T>
concept NodeType = std::is_base_of_v<zeem::node, std::remove_cvref_t<T>>;

/**
 * @brief An enum used as a poor mans RTTI, i.e. you can use this type
 * to find out the actual type of a node
 */

ZEEM_EXPORT enum class ZEEM_API node_type : uint8_t {
	element,
	text,
	attribute,
	comment,
	cdata,
	document,
	processing_instruction,

	header
};

// --------------------------------------------------------------------

/// \brief specification of how XML data should be written out
ZEEM_EXPORT struct ZEEM_API format_info
{
	bool indent = false;             ///< Indent the output
	bool indent_attributes = false;  ///< Indent attributes on separate lines
	bool collapse_tags = true;       ///< Collapse empty tags, e.g. write `<foo/>` instead of `<foo></foo>`
	bool suppress_comments = false;  ///< Do not write comments
	bool escape_white_space = false; ///< Write white space as XML entities
	bool escape_double_quote = true; ///< Write double quotes as &quot;
	bool html = false;               ///< This flag can be used to collapse only 'empty elements'
	std::size_t indent_width = 0;    ///< Number of spaces per indent level
	std::size_t indent_level = 0;    ///< The current indent level
	version_type version{ 1, 0 };    ///< The XML version to write
};

// --------------------------------------------------------------------

/**
 * \brief The node base class
 *
 * Node is the abstract base class for all data contained in zeep XML documents.
 * The DOM tree consists of nodes that are linked to each other, each
 * node can have a parent and siblings pointed to by the next and
 * previous members. All nodes in a DOM tree share a common root node.
 *
 * Nodes can have a name, and the XPath specification requires that a node can
 * have a so-called expanded-name. This name consists of a local-name and a
 * namespace which is a URI. And we can have a QName which is a concatenation of
 * a prefix (that points to a namespace URI) and a local-name separated by a colon.
 *
 * To reduce storage requirements, names are stored in nodes as qnames, if at all.
 * the convenience functions name() and prefix() parse the qname(). ns() returns
 * the namespace URI for the node, if it can be resolved.
 *
 * Nodes inherit the namespace of their parent unless they override it which means
 * resolving prefixes and namespaces is done hierarchically
 *
 * Nodes are stored in a node_list, a generic list class that resembles std::list
 */

class node
{
  public:
	/** @cond */

	ZEEM_API node &operator=(const node &n) = delete;
	ZEEM_API node &operator=(node &&n) = delete;

	ZEEM_API virtual ~node() = default;

	/** @endcond */

	/// \brief node_type to be returned by each implementation of this node class
	ZEEM_API [[nodiscard]] virtual constexpr node_type type() const = 0;

	/// content of a xml:lang attribute of this element, or its nearest ancestor
	ZEEM_API [[nodiscard]] virtual std::string lang() const;

	/**
	 * @brief Get the qualified name
	 *
	 * Nodes can have a name, and the XPath specification requires that a node can
	 * have a so-called expanded-name. This name consists of a local-name and a
	 * namespace which is a URI. And we can have a QName which is a concatenation of
	 * a prefix (that points to a namespace URI) and a local-name separated by a colon.
	 *
	 * To reduce storage requirements, names are stored in nodes as qnames, if at all.
	 *
	 * @return std::string
	 */
	ZEEM_API [[nodiscard]] virtual std::string get_qname() const;

	/**
	 * @brief Set the qualified name to @a qn
	 *
	 * This is only meaningful in attributes and elements.
	 *
	 * @param qn
	 */
	ZEEM_API virtual void set_qname([[maybe_unused]] std::string qn) noexcept {} // NOLINT(performance-unnecessary-value-param)

	/**
	 * \brief set the qname with two parameters, if \a prefix is empty the qname will be simply \a name
	 * otherwise the name will be `prefix:name`
	 *
	 * \param prefix	The namespace prefix to use
	 * \param name		The actual name to use
	 */

	ZEEM_API void set_qname(const std::string &prefix, std::string name)
	{
		set_qname(prefix.empty() ? std::move(name) : prefix + ':' + name);
	}

	ZEEM_API [[nodiscard]] virtual std::string get_local_name() const; ///< The local-name for the node as parsed from the qname.
	ZEEM_API [[nodiscard]] virtual std::string get_prefix() const;     ///< The prefix for the node as parsed from the qname.
	ZEEM_API [[nodiscard]] virtual std::string get_ns() const;         ///< Returns the namespace URI for the node, if it can be resolved.

	ZEEM_API [[nodiscard]] virtual std::string name() const ///< By default, the name returns the local name
	{
		return get_local_name();
	}

	/// Return the namespace URI for a prefix
	ZEEM_API [[nodiscard]] virtual std::string namespace_for_prefix(std::string_view prefix) const;

	/// Return the prefix for a namespace URI
	ZEEM_API [[nodiscard]] virtual std::pair<std::string, bool> prefix_for_namespace(std::string_view uri) const;

	/// Prefix the \a tag with the namespace prefix for \a uri
	ZEEM_API [[nodiscard]] virtual std::string prefix_tag(const std::string &tag, std::string_view uri) const;

	/// return all content concatenated, including that of children.
	ZEEM_API [[nodiscard]] virtual std::string str() const = 0;

	// --------------------------------------------------------------------
	// low level routines

	// basic access

	// All nodes should have a single root node
	ZEEM_API virtual element_container *root();                           ///< The root node for this node
	ZEEM_API [[nodiscard]] virtual const element_container *root() const; ///< The root node for this node

	ZEEM_API void parent(element_container *p) noexcept { m_parent = p; }               ///< Set parent to \a p
	ZEEM_API element_container *parent() { return m_parent; }                           ///< The parent node for this node
	ZEEM_API [[nodiscard]] const element_container *parent() const { return m_parent; } ///< The parent node for this node

	ZEEM_API void next(node *n) noexcept { m_next = const_cast<node *>(n); } ///< Set next to \a n
	ZEEM_API node *next() { return m_next; }                                 ///< The next sibling
	ZEEM_API [[nodiscard]] const node *next() const { return m_next; }       ///< The next sibling

	ZEEM_API void prev(node *n) noexcept { m_prev = const_cast<node *>(n); } ///< Set prev to \a n
	ZEEM_API node *prev() { return m_prev; }                                 ///< The previous sibling
	ZEEM_API [[nodiscard]] const node *prev() const { return m_prev; }       ///< The previous sibling

	/// Compare the node with \a n
	ZEEM_API virtual bool equals(const node *n) const;

	/// \brief low level routine for writing out XML
	///
	/// This method is usually called by operator<<(std::ostream&, zeem::document&)
	ZEEM_API virtual void write(std::ostream &os, format_info fmt) const = 0;

  protected:
	/** @cond */

	friend class basic_node_list;
	template <typename>
	friend class node_list;
	friend class element;

	node()
	{
		init();
	}

	node([[maybe_unused]] const node &n)
	{
		init();
	}

	node([[maybe_unused]] node &&n) noexcept
	{
		init();
	}

	friend void swap(node &a, node &b) noexcept
	{
		if (a.m_next == &a) // a empty?
		{
			if (b.m_next != &b) // b empty?
			{
				a.m_next = b.m_next;
				a.m_prev = b.m_prev;
				b.init();
			}
		}
		else if (b.m_next == &b)
		{
			b.m_next = a.m_next;
			b.m_prev = a.m_prev;
			a.init();
		}
		else
		{
			std::swap(a.m_next, b.m_next);
			std::swap(a.m_prev, b.m_prev);
		}

		a.m_next->m_prev = a.m_prev->m_next = &a;
		b.m_next->m_prev = b.m_prev->m_next = &b;
	}

  protected:
	void init()
	{
		m_next = m_prev = this;
	}

	element_container *m_parent = nullptr;
	node *m_next{};
	node *m_prev{};

	/** @endcond */
};

// --------------------------------------------------------------------
// Basic node list is a private class, it is the base class
// for node_list

/** @cond */

class basic_node_list
{
  protected:
	struct node_list_header : public node
	{
		[[nodiscard]] constexpr node_type type() const override { return node_type::header; }

		void write(std::ostream & /* os */, format_info /* fmt */) const override {}
		[[nodiscard]] std::string str() const override { return {}; }

		friend void swap(node_list_header &a, node_list_header &b) noexcept
		{
			swap(static_cast<node &>(a), static_cast<node &>(b));

			for (node *n = a.m_next; n != &a; n = n->next())
				n->parent(a.m_parent);

			for (node *n = b.m_next; n != &b; n = n->next())
				n->parent(b.m_parent);
		}
	};

  private:
	node_list_header m_header_node;
	node *m_header = nullptr;
	bool m_owner = false;

	template <typename T>
	friend class node_list;

  protected:
	explicit basic_node_list(element_container *e)
		: m_header(&m_header_node)
	{
		m_header_node.parent(e);
	}

  public:
	basic_node_list(basic_node_list &&nl) = delete;
	basic_node_list &operator=(const basic_node_list &nl) = delete;
	basic_node_list &operator=(basic_node_list &&nl) = delete;
	virtual ~basic_node_list() = default;

	ZEEM_API bool operator==(const basic_node_list &b) const;

	/// \brief remove all nodes
	ZEEM_API virtual void clear();

  protected:
	friend void swap(basic_node_list &a, basic_node_list &b) noexcept
	{
		if (a.m_header != &a.m_header_node and b.m_header != &b.m_header_node)
			std::swap(a.m_header, b.m_header);
		else
		{
			assert((a.m_header != &a.m_header_node) == (b.m_header != &b.m_header_node));
			swap(a.m_header_node, b.m_header_node);
		}
	}

  protected:
	// proxy methods for every insertion

	ZEEM_API virtual node *insert_impl(const node *p, std::unique_ptr<node> n);

	ZEEM_API node *erase_impl(node *n);
};

/** @endcond */

// --------------------------------------------------------------------

/**
 * \brief generic iterator class.
 *
 * We can have iterators that point to nodes, elements and attributes.
 * Iterating over nodes is simply following next/prev. But iterating
 * elements is a bit more difficult, since you then have to skip nodes
 * in between that are not an element, like comments or text.
 *
 * This iterator is used for iterators over elements, attributes and
 * simply all nodes
 */

template <typename T>
class iterator_impl
{
  public:
	/** @cond */

	template <typename T2>
	friend class iterator_impl;

	/** @endcond */

	/** @cond */

	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = T;
	using pointer = value_type *;
	using reference = value_type &;
	using difference_type = std::ptrdiff_t;

	ZEEM_API iterator_impl() = default;

	// NOLINTBEGIN(hicpp-explicit-conversions)
	ZEEM_API iterator_impl(node *current)
		: m_current(current)
	{
		skip();
	}

	ZEEM_API iterator_impl(const node *current)
		requires(std::is_const_v<value_type>)
		: iterator_impl(const_cast<node *>(current))
	{
	}

	ZEEM_API iterator_impl(const iterator_impl &i) = default;

	/**
	 * @brief Copy constructor
	 *
	 * This copy constructor allows to copy from the same value_type
	 * and from derived types. That means that you can assign an
	 * iterator pointing to an element to a new iterator pointing
	 * to nodes. But vice versa is not possible.
	 */
	template <typename Iterator>
		requires(std::is_base_of_v<value_type, typename Iterator::value_type>)
	ZEEM_API iterator_impl(const Iterator &i)
		: m_current(const_cast<node *>(i.m_current))
	{
		skip();
	}

	// NOLINTEND(hicpp-explicit-conversions)

	ZEEM_API iterator_impl &operator=(iterator_impl i)
	{
		m_current = i.m_current;
		return *this;
	}

	template <typename Iterator>
		requires(std::is_base_of_v<value_type, typename Iterator::value_type>)
	ZEEM_API iterator_impl &operator=(const Iterator &i)
	{
		m_current = i.m_current;
		return *this;
	}

	ZEEM_API reference operator*() const { return *static_cast<value_type *>(m_current); }
	ZEEM_API pointer operator->() const { return static_cast<value_type *>(m_current); }

	ZEEM_API iterator_impl &operator++()
	{
		m_current = m_current->next();
		skip();
		return *this;
	}

	ZEEM_API iterator_impl operator++(int)
	{
		iterator_impl iter(*this);
		operator++();
		return iter;
	}

	ZEEM_API iterator_impl &operator--()
	{
		m_current = m_current->prev();
		if constexpr (std::is_same_v<std::remove_cv_t<value_type>, element>)
		{
			while (m_current->type() != node_type::element and m_current->type() != node_type::header)
				m_current = m_current->prev();
		}

		return *this;
	}

	ZEEM_API iterator_impl operator--(int)
	{
		iterator_impl iter(*this);
		operator--();
		return iter;
	}

	template <typename IteratorType>
		requires NodeType<typename IteratorType::node_type>
	ZEEM_API bool operator==(const IteratorType &other) const
	{
		return m_current == other.m_current;
	}

	ZEEM_API bool operator==(const iterator_impl &other) const
	{
		return m_current == other.m_current;
	}

	template <NodeType T2>
	ZEEM_API bool operator==(const T2 *n) const { return m_current == n; }

	ZEEM_API explicit operator pointer() const { return static_cast<pointer>(m_current); }

  private:
	using node_base_type = std::conditional_t<std::is_const_v<T>, const node, node>;

	void skip()
	{
		if constexpr (std::is_same_v<std::remove_cv_t<value_type>, element>)
		{
			while (m_current != nullptr and m_current->type() != node_type::element and m_current->type() != node_type::header)
				m_current = m_current->next();
		}
	}

	node_base_type *m_current = nullptr;
	/** @endcond */
};

// --------------------------------------------------------------------

/**
 * @brief An abstract base class for lists of type \a T
 *
 * This base class should offer all methods required for a
 * SequenceContainer.
 *
 * This class is not exported.
 *
 * Note that this class can act as a real container, which
 * stores data, or it can act as a view on another node_list
 * optionally changing what is made visible.
 *
 * An element derives from node_list<element>, so it exposes
 * access to all its children of type element. However, since
 * node_lists store pointers to nodes, the list can contain
 * more than just element nodes. To access these other nodes
 * you can use a node_list<node> constructed with an element
 * as parameter. This node_list<node> will expose all nodes
 * in the element.
 *
 * @tparam T The type of node contained, either element, attribute or node
 */

template <typename T = node>
class node_list : public basic_node_list
{
  public:
	/** @cond */

	using value_type = T;
	using allocator_type = std::allocator<value_type>;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;

	/** @endcond */

	/**
	 * @brief Construct a new node list for an element_container \a e
	 *
	 * @param e The element_container
	 */
  private:
	node_list(element_container *e); // NOLINT(hicpp-explicit-conversions)
	friend class element_container;
	friend class attribute_set;

  public:
	/// @brief The iterator class
	using iterator = iterator_impl<value_type>;
#if not defined(_MSC_VER)
	static_assert(std::input_iterator<iterator>);
#endif

	/// @brief The const iterator class
	using const_iterator = iterator_impl<const value_type>;
#if not defined(_MSC_VER)
	static_assert(std::input_iterator<const_iterator>);
#endif

	/// @brief Return an iterator to the first element
	ZEEM_API [[nodiscard]] iterator begin() { return iterator(m_header->m_next); }
	/// @brief Return an iterator past the last element
	ZEEM_API [[nodiscard]] iterator end() { return iterator(m_header); }

	/// @brief Return a const iterator to the first element
	ZEEM_API [[nodiscard]] const_iterator cbegin() const { return const_iterator(m_header->m_next); }
	/// @brief Return a const iterator past the last element
	ZEEM_API [[nodiscard]] const_iterator cend() const { return const_iterator(m_header); }

	/// @brief Return a const iterator to the first element
	ZEEM_API [[nodiscard]] const_iterator begin() const { return const_iterator(m_header->m_next); }
	/// @brief Return a const iterator past the last element
	ZEEM_API [[nodiscard]] const_iterator end() const { return const_iterator(m_header); }

	/// @brief Return a reference to the first element
	ZEEM_API [[nodiscard]] value_type &front() { return *begin(); }
	/// @brief Return a reference to the first element
	ZEEM_API [[nodiscard]] const value_type &front() const { return *begin(); }

	/// @brief Return a reference to the last element
	ZEEM_API [[nodiscard]] value_type &back() { return *std::prev(end()); }
	/// @brief Return a reference to the last element
	ZEEM_API [[nodiscard]] const value_type &back() const { return *std::prev(end()); }

	/// @brief The size of the visible items
	/// @return The count of items visible
	ZEEM_API [[nodiscard]] std::size_t size() const { return std::distance(begin(), end()); }
	/// @brief Whether the list contains any items
	ZEEM_API [[nodiscard]] bool empty() const { return size() == 0; }
	/// @brief Whether the list contains any items
	ZEEM_API explicit operator bool() const { return not empty(); }

	/// \brief insert a copy of \a e
	ZEEM_API iterator insert(const_iterator pos, const value_type &e);

	/// \brief insert a copy of \a e at position \a pos, moving its data
	ZEEM_API iterator insert(const_iterator pos, value_type &&e);

	/// \brief construct a new node using arguments provided in \a a

	// When users try to emplace/insert e.g. a cdata node in an element
	// this will fail, since they need to use the nodes() variant.

	template <typename... Args>
	ZEEM_API iterator insert(const_iterator p, Args &&...args)
		requires(sizeof...(Args) > 1 or not std::is_base_of_v<node, std::remove_cvref_t<Args>...>)
	{
		return insert_impl(p, std::make_unique<value_type>(std::forward<Args>(args)...));
	}

	/// \brief insert \a count copies of \a n at position \a pos
	ZEEM_API iterator insert(const_iterator pos, std::size_t count, const value_type &n)
	{
		iterator p(const_cast<value_type *>(&*pos));
		while (count-- > 0)
			p = insert(p, n);
		return p;
	}

	/// \brief insert copies of the nodes from \a first to \a last at position \a pos
	template <typename InputIter>
	ZEEM_API iterator insert(const_iterator pos, InputIter first, InputIter last)
	{
		iterator p(const_cast<value_type *>(&*pos));
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
	ZEEM_API iterator insert(const_iterator pos, std::initializer_list<value_type> nodes)
	{
		return insert(pos, nodes.begin(), nodes.end());
	}

	/// \brief replace content with copies of the nodes from \a first to \a last
	template <typename InputIter>
	ZEEM_API void assign(InputIter first, InputIter last)
	{
		basic_node_list::clear();
		insert(begin(), first, last);
	}

	/// \brief emplace an element at position \a p using arguments \a args
	template <typename... Args>
	ZEEM_API iterator emplace(const_iterator p, Args &&...args)
	{
		return insert(p, std::forward<Args>(args)...);
	}

	/// \brief emplace an element at the front using arguments \a args
	template <typename... Args>
	ZEEM_API iterator emplace_front(Args &&...args)
	{
		return emplace(begin(), std::forward<Args>(args)...);
	}

	/// \brief emplace an element at the back using arguments \a args
	template <typename... Args>
	ZEEM_API iterator emplace_back(Args &&...args)
	{
		return emplace(end(), std::forward<Args>(args)...);
	}

	/// \brief erase the node at \a pos
	ZEEM_API iterator erase(const_iterator pos)
	{
		return erase_impl(pos);
	}

	/// \brief erase the nodes from \a first to \a last
	ZEEM_API iterator erase(iterator first, iterator last)
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
	ZEEM_API void pop_front()
	{
		erase(begin());
	}

	/// \brief erase the last node
	ZEEM_API void pop_back()
	{
		erase(std::prev(end()));
	}

	/// \brief move the value_type \a e to the front of this value_type.
	ZEEM_API void push_front(value_type &&e)
	{
		emplace(begin(), std::forward<value_type>(e));
	}

	/// \brief copy the value_type \a e to the front of this value_type.
	ZEEM_API void push_front(const value_type &e)
	{
		emplace(begin(), e);
	}

	/// \brief move the value_type \a e to the back of this value_type.
	ZEEM_API void push_back(value_type &&e)
	{
		emplace(end(), std::forward<value_type>(e));
	}

	/// \brief copy the value_type \a e to the back of this value_type.
	ZEEM_API void push_back(const value_type &e)
	{
		emplace(end(), e);
	}

	/// \brief Sort the nodes
	template <typename Pred>
	ZEEM_API void sort(const Pred &pred);

  protected:
	/** @cond */

	using basic_node_list::insert_impl;

	node *insert_impl(const_iterator pos, std::unique_ptr<node> n)
	{
		return insert_impl(&*pos, std::move(n));
	}

	using basic_node_list::erase_impl;

	node *erase_impl(iterator pos)
	{
		return basic_node_list::erase_impl(&*pos);
	}
	friend T;

	/** @endcond */
};

// --------------------------------------------------------------------

/**
 * @brief internal class as base class for element and document
 *
 * Both element and document can have a list of child nodes and
 * both are nodes implementing the namespace routines e.g.
 *
 * However, element has attributes whereas document does not.
 * And document has the constraint that it can have at most
 * one child element. But since the rest is so similar they
 * have a common base class: element_container.
 *
 * element_container is not exported.
 */

ZEEM_EXPORT class element_container : public node, public node_list<element>
{
  public:
	/// @brief Default constructor
	ZEEM_API element_container()
		: node_list<element>(this)
	{
	}

	/// @brief Copy constructor
	ZEEM_API element_container(const element_container &e)
		: node(e)
		, node_list<element>(this)
	{
		auto a = nodes();
		auto b = e.nodes();
		a.assign(b.begin(), b.end());
	}

	/// @brief Destructor
	ZEEM_API ~element_container() override
	{
		clear();
	}

	// --------------------------------------------------------------------

	/** @cond */
	ZEEM_API friend void swap(element_container &a, element_container &b) noexcept
	{
		swap(static_cast<node_list<element> &>(a), static_cast<node_list<element> &>(b));
	}
	/** @endcond */

	// --------------------------------------------------------------------
	// children

	/**
	 * @brief This method allows access to the nodes not visible using
	 * the regular interface of this class itself.
	 *
	 * @return node_list<> The node_list for nodes of all types
	 */
	ZEEM_API node_list<> nodes() { return { this }; }

	/**
	 * @brief This method allows read access to the nodes not visible using
	 * the regular interface of this class itself.
	 *
	 * @return node_list<> The node_list for nodes of all types
	 */
	ZEEM_API [[nodiscard]] const node_list<> nodes() const { return node_list<node>(const_cast<element_container *>(this)); }

	/// \brief will return the concatenation of str() from all child nodes
	ZEEM_API [[nodiscard]] std::string str() const override;

	/// \brief return the elements that match XPath \a path.
	///
	/// If you need to find other classes than xml::element, of if your XPath
	/// contains variables, you should create a zeem::xpath object and use
	/// its evaluate method.
	ZEEM_API [[nodiscard]] std::vector<element *> find(std::string_view path) const;

	/// \brief return the first element that matches XPath \a path.
	///
	/// If you need to find other classes than xml::element, of if your XPath
	/// contains variables, you should create a zeem::xpath object and use
	/// its evaluate method.
	ZEEM_API [[nodiscard]] iterator find_first(std::string_view path);
	/// \brief return the first element that matches XPath \a path.
	///
	/// If you need to find other classes than xml::element, of if your XPath
	/// contains variables, you should create a zeem::xpath object and use
	/// its evaluate method.
	ZEEM_API [[nodiscard]] const_iterator find_first(std::string_view path) const;

	// With prepared xpaths:

	/// \brief return the elements that match XPath \a path.
	ZEEM_API [[nodiscard]] std::vector<element *> find(const xpath &path, const context &ctxt) const;

	/// \brief return the first element that matches XPath \a path.
	ZEEM_API [[nodiscard]] iterator find_first(const xpath &path, const context &ctxt);
	/// \brief return the first element that matches XPath \a path.
	ZEEM_API [[nodiscard]] const_iterator find_first(const xpath &path, const context &ctxt) const;

	/** @cond */
	ZEEM_API void write(std::ostream &os, format_info fmt) const override;
	/** @endcond */
};

// --------------------------------------------------------------------
// internal node base class for storing text

/**
 * @brief An abstract base class for nodes that contain text
 *
 */

ZEEM_EXPORT class node_with_text : public node
{
  protected:
	/** @cond */

	node_with_text() = default;

	explicit node_with_text(std::string s)
		: m_text(std::move(s))
	{
	}

	node_with_text(const node_with_text &n) = default;
	node_with_text(node_with_text &&n) = default;

  public:
	ZEEM_API friend void swap(node_with_text &a, node_with_text &b) noexcept
	{
		std::swap(a.m_text, b.m_text);
	}

	/** @endcond */

	/// \brief return the text content
	ZEEM_API [[nodiscard]] std::string str() const override { return m_text; }

	/// \brief return the text content, same as str()
	ZEEM_API [[nodiscard]] virtual std::string get_text() const { return m_text; }

	/// \brief set the text content
	ZEEM_API virtual void set_text(std::string text) { m_text = std::move(text); }

	/// @brief Compare two nodes with text for equality
	ZEEM_API bool equals(const node *n) const override
	{
		return type() == n->type() and
		       static_cast<const node_with_text *>(n)->m_text == m_text;
	}

	/** @cond */

  protected:
	void append_text(std::string_view txt)
	{
		m_text += txt;
	}

	std::string m_text;
	/** @endcond */
};

// --------------------------------------------------------------------

/**
 * @brief A node containing a XML comment
 *
 */

ZEEM_EXPORT class comment final : public node_with_text
{
  public:
	ZEEM_API [[nodiscard]] constexpr node_type type() const override { return node_type::comment; }

	/// @brief default constructor
	ZEEM_API explicit comment(std::string text = {})
		: node_with_text(std::move(text))
	{
	}

	/// @brief copy constructor
	ZEEM_API comment(const comment &c) = default;

	/// @brief move constructor
	ZEEM_API comment(comment &&c) noexcept
	{
		swap(*this, c);
	}

	/// @brief assignment operator
	ZEEM_API comment &operator=(comment c) noexcept
	{
		swap(*this, c);
		return *this;
	}

	/// \brief compare nodes for equality
	ZEEM_API bool equals(const node *n) const override
	{
		return this == n or (n->type() == node_type::comment and node_with_text::equals(n));
	}

	/** @cond */
	ZEEM_API void write(std::ostream &os, format_info fmt) const override;
	/** @endcond */
};

// --------------------------------------------------------------------
/**
 * @brief A node containing a XML processing instruction (like e.g. \<?php ?\>)
 *
 */

ZEEM_EXPORT class processing_instruction final : public node_with_text
{
  public:
	ZEEM_API [[nodiscard]] constexpr node_type type() const override { return node_type::processing_instruction; }

	/// @brief default constructor
	ZEEM_API processing_instruction() = default;

	/// \brief constructor with parameters
	///
	/// This constructs a processing instruction with the specified parameters
	/// \param target	The target, this will follow the <? characters, e.g. `php` will generate <?php ... ?>
	/// \param text		The text inside this node, e.g. the PHP code.
	ZEEM_API processing_instruction(std::string target, std::string text)
		: node_with_text(std::move(text))
		, m_target(std::move(target))
	{
	}

	/// @brief copy constructor
	ZEEM_API processing_instruction(const processing_instruction &pi) = default;

	/// @brief move constructor
	ZEEM_API processing_instruction(processing_instruction &&pi) noexcept = default;

	/// @brief assignment operator
	ZEEM_API processing_instruction &operator=(processing_instruction pi) noexcept
	{
		swap(*this, pi);
		return *this;
	}

	/** @cond */
	ZEEM_API friend void swap(processing_instruction &a, processing_instruction &b) noexcept
	{
		swap(static_cast<node_with_text &>(a), static_cast<node_with_text &>(b));
		std::swap(a.m_target, b.m_target);
	}
	/** @endcond */

	/// \brief return the qname which is the same as the target in this case
	ZEEM_API [[nodiscard]] std::string get_qname() const override { return m_target; }

	/// \brief return the target
	ZEEM_API [[nodiscard]] std::string get_target() const { return m_target; }

	/// \brief set the target
	ZEEM_API void set_target(std::string target) { m_target = std::move(target); }

	/// \brief compare nodes for equality
	ZEEM_API bool equals(const node *n) const override
	{
		return this == n or
		       (n->type() == node_type::processing_instruction and
				   node_with_text::equals(n) and
				   m_target == static_cast<const processing_instruction *>(n)->m_target);
	}

	/** @cond */
	ZEEM_API void write(std::ostream &os, format_info fmt) const override;

  private:
	std::string m_target;

	/** @endcond */
};

// --------------------------------------------------------------------
/**
 * @brief A node containing text.
 *
 */

class text final : public node_with_text
{
  public:
	ZEEM_API [[nodiscard]] constexpr node_type type() const override { return node_type::text; }

	/// @brief default constructor
	ZEEM_API explicit text(std::string text = {})
		: node_with_text(std::move(text))
	{
	}

	/// @brief copy constructor
	ZEEM_API text(const text &t) = default;

	/// @brief move constructor
	ZEEM_API text(text &&t) noexcept = default;

	/// @brief assignment operator
	ZEEM_API text &operator=(text txt) noexcept
	{
		swap(*this, txt);
		return *this;
	}

	/// \brief append \a text to the stored text
	ZEEM_API void append(std::string_view text) { append_text(text); }

	/// \brief compare nodes for equality
	ZEEM_API bool equals(const node *n) const override;

	/// \brief returns true if this text contains only whitespace characters
	ZEEM_API [[nodiscard]] bool is_space() const;

	/** @cond */
	ZEEM_API void write(std::ostream &os, format_info fmt) const override;
	/** @endcond */
};

// --------------------------------------------------------------------
/**
 * @brief A node containing the contents of a CDATA section. Normally, these nodes are
 * converted to text nodes but you can specify to preserve them when parsing a document.
 *
 */

ZEEM_EXPORT class cdata final : public node_with_text
{
  public:
	ZEEM_API [[nodiscard]] constexpr node_type type() const override { return node_type::cdata; }

	/// @brief default constructor
	ZEEM_API explicit cdata(std::string s = {})
		: node_with_text(std::move(s))
	{
	}

	/// @brief copy constructor
	ZEEM_API cdata(const cdata &cd) = default;

	/// @brief move constructor
	ZEEM_API cdata(cdata &&cd) noexcept = default;

	/// @brief assignment operator
	ZEEM_API cdata &operator=(cdata cd) noexcept
	{
		swap(*this, cd);
		return *this;
	}

	/// \brief append \a text to the stored text
	ZEEM_API void append(std::string_view text) { append_text(text); }

	/// \brief compare nodes for equality
	ZEEM_API bool equals(const node *n) const override
	{
		return this == n or (n->type() == node_type::cdata and node_with_text::equals(n));
	}

	/** @cond */
	ZEEM_API void write(std::ostream &os, format_info fmt) const override;
	/** @endcond */
};

// --------------------------------------------------------------------
/**
 * @brief An attribute is a node, has an element as parent, but is not a child of this parent (!)
 *
 */

class attribute final : public node
{
  public:
	ZEEM_API [[nodiscard]] constexpr node_type type() const override { return node_type::attribute; }

	/// @brief constructor
	/// @param qname The qualified name
	/// @param value The value
	/// @param id Flag to indicate if this is an ID attribute
	ZEEM_API attribute(std::string_view qname, std::string_view value, bool id = false)
		: m_qname(qname)
		, m_value(value)
		, m_id(id)
	{
	}

	/// @brief copy constructor
	ZEEM_API attribute(const attribute &attr) = default;

	/// @brief move constructor
	ZEEM_API attribute(attribute &&attr) noexcept
		: node(std::forward<attribute>(attr))
	{
		swap(*this, attr);
	}

	/// @brief assignment operator
	ZEEM_API attribute &operator=(attribute attr) noexcept
	{
		swap(*this, attr);
		return *this;
	}

	/** @cond */
	ZEEM_API friend void swap(attribute &a, attribute &b) noexcept
	{
		std::swap(a.m_qname, b.m_qname);
		std::swap(a.m_value, b.m_value);
		std::swap(a.m_id, b.m_id);
	}
	/** @endcond */

	/// @brief Attributes can be sorted
	ZEEM_API friend std::strong_ordering operator<=>(const attribute &a, const attribute &b)
	{
		if (auto cmp = (a.m_qname <=> b.m_qname); cmp != 0)
			return cmp;
		if (auto cmp = (a.m_id <=> b.m_id); cmp != 0)
			return cmp;
		return a.m_value <=> b.m_value;
	}

	/// @brief Compare two attributes for equality
	ZEEM_API bool operator==(const attribute &rhs) const
	{
		return equals(&rhs);
	}

	/// @brief Get the qualified name for this attribute
	ZEEM_API [[nodiscard]] std::string get_qname() const override { return m_qname; }

	/// @brief Set the qualified name to \a qn
	ZEEM_API void set_qname(std::string qn) noexcept override { m_qname = std::move(qn); }

	using node::set_qname;

	/// \brief Is this attribute an xmlns attribute?
	ZEEM_API [[nodiscard]] bool is_namespace() const
	{
		return m_qname.starts_with("xmlns") and (m_qname.length() == 5 or m_qname[5] == ':');
	}

	/// @brief Return the value of this attribute
	ZEEM_API [[nodiscard]] std::string value() const { return m_value; }

	/// @brief Set the value of this attribute to \a v
	ZEEM_API void set_value(std::string v) { m_value = std::move(v); }

	/// @brief Set the value of this attribute to \a v
	ZEEM_API void set_value(std::string_view v) { m_value = v; }

	/// \brief same as value, but checks to see if this really is a namespace attribute
	ZEEM_API [[nodiscard]] std::string uri() const;

	/// @brief Returns the value of this attribute
	ZEEM_API [[nodiscard]] std::string str() const override { return m_value; }

	/// \brief compare nodes for equality
	ZEEM_API bool equals(const node *n) const override
	{
		bool result = false;
		if (type() == n->type())
		{
			auto an = static_cast<const attribute *>(n);
			result = an->m_id == m_id and an->m_qname == m_qname and an->m_value == m_value;
		}
		return result;
	}

	/// \brief returns whether this attribute is an ID attribute, as defined in an accompanying DTD
	ZEEM_API [[nodiscard]] bool is_id() const { return m_id; }

	/// \brief support for structured binding
	template <std::size_t N>
	ZEEM_API [[nodiscard]] decltype(auto) get() const
	{
		if constexpr (N == 0)
			return get_local_name();
		else if constexpr (N == 1)
			return value();
	}

	/** @cond */
	ZEEM_API void write(std::ostream &os, format_info fmt) const override;

  private:
	std::string m_qname, m_value;
	bool m_id{};
	/** @endcond */
};

// --------------------------------------------------------------------
/**
 * @brief set of attributes and name_spaces. Is a node_list but with a set interface
 *
 */

ZEEM_EXPORT class attribute_set : public node_list<attribute>
{
  public:
	/// @brief constructor to create an attribute_set for an element
	ZEEM_API explicit attribute_set(element_container *el)
		: node_list(el)
	{
	}

	/// @brief destructor
	ZEEM_API ~attribute_set() override
	{
		clear();
	}

	/** @cond */
	ZEEM_API friend void swap(attribute_set &a, attribute_set &b) noexcept
	{
		swap(static_cast<node_list<attribute> &>(a), static_cast<node_list<attribute> &>(b));
	}
	/** @endcond */

	/// \brief return true if the attribute with name \a key is defined
	ZEEM_API [[nodiscard]] bool contains(std::string_view key) const
	{
		return find(key) != end();
	}

	/// \brief return const_iterator to the attribute with name \a key
	ZEEM_API [[nodiscard]] const_iterator find(std::string_view key) const
	{
		for (auto i = begin(); i != end(); ++i)
		{
			if (i->get_qname() == key)
				return i;
		}
		return end();
	}

	/// \brief return iterator to the attribute with name \a key
	ZEEM_API iterator find(std::string_view key)
	{
		return iterator{ const_cast<const attribute_set &>(*this).find(key) };
	}

	/// \brief emplace a newly constructed attribute with argumenst \a args
	template <typename... Args>
	ZEEM_API std::pair<iterator, bool> emplace(Args &&...args)
	{
		return emplace(value_type{ std::forward<Args>(args)... });
	}

	/// \brief emplace an attribute move constructed from \a a
	/// \return returns a std::pair with an iterator pointing to the inserted attribute
	/// and a boolean indicating if this attribute was inserted instead of replaced.
	ZEEM_API std::pair<iterator, bool> emplace(value_type &&a)
	{
		bool inserted = false;

		auto i = find(a.get_qname());

		if (i != node_list::end())
			*i = std::forward<value_type>(a); // move assign value of a
		else
		{
			i = iterator{ node_list::insert_impl(node_list::end(), std::make_unique<attribute>(std::forward<value_type>(a))) };
			inserted = true;
		}

		return std::make_pair(i, inserted);
	}

	using node_list::erase;

	/// \brief remove attribute with name \a key
	ZEEM_API size_type erase(std::string_view key)
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
/**
 * @brief the element class modelling a XML element
 *
 * element is the most important zeem::node object. It encapsulates a
 * XML element as found in the XML document. It has a qname, can have children,
 * attributes and a namespace.
 */

class element final : public element_container
{
  public:
	ZEEM_API [[nodiscard]] constexpr node_type type() const override { return node_type::element; }

	/// @brief default constructor
	ZEEM_API element()
		: m_attributes(this)
	{
	}

	/// @brief constructor taking a \a qname
	ZEEM_API explicit element(std::string_view qname)
		: m_qname(qname)
		, m_attributes(this)
	{
	}

	/// @brief constructor taking a \a qname and a list of \a attributes
	ZEEM_API element(std::string_view qname, std::initializer_list<attribute> attributes)
		: m_qname(qname)
		, m_attributes(this)
	{
		m_attributes.assign(attributes.begin(), attributes.end());
	}

	/// @brief constructor taking a \a qname and a list of child elements
	ZEEM_API element(std::string_view qname, std::initializer_list<element> il)
		: m_qname(qname)
		, m_attributes(this)
	{
		assign(il.begin(), il.end());
	}

	/// @brief copy constructor
	ZEEM_API element(const element &e)
		: element_container(e)
		, m_qname(e.m_qname)
		, m_attributes(this)
	{
		m_attributes.assign(e.m_attributes.begin(), e.m_attributes.end());
	}

	/// @brief move constructor
	ZEEM_API element(element &&e) noexcept
		: m_attributes(this)
	{
		swap(*this, e);
	}

	/// @brief assignment operator
	ZEEM_API element &operator=(element e) noexcept
	{
		swap(*this, e);
		return *this;
	}

	/** @cond */
	ZEEM_API friend void swap(element &a, element &b) noexcept
	{
		// swap(static_cast<node&>(a), static_cast<node&>(b));
		swap(static_cast<element_container &>(a), static_cast<element_container &>(b));

		std::swap(a.m_qname, b.m_qname);
		swap(a.m_attributes, b.m_attributes);
	}
	/** @endcond */

	using node::set_qname;

	/// @brief Return the qualified name
	ZEEM_API [[nodiscard]] std::string get_qname() const override { return m_qname; }

	/// @brief Set the qualified name to \a qn
	ZEEM_API void set_qname(std::string qn) noexcept override { m_qname = std::move(qn); }

	/// \brief content of a xml:lang attribute of this element, or its nearest ancestor
	ZEEM_API [[nodiscard]] std::string lang() const override;

	/// \brief content of the xml:id attribute, or the attribute that was defined to be
	/// of type ID by the DOCTYPE.
	ZEEM_API [[nodiscard]] std::string id() const;

	/// @brief Compare two elements for equality
	ZEEM_API bool operator==(const element &e) const
	{
		return equals(&e);
	}

	/// @brief Compare two elements for equality
	ZEEM_API bool equals(const node *n) const override;

	// --------------------------------------------------------------------
	// attribute support

	/// \brief return the set of attributes for this element
	ZEEM_API attribute_set &attributes() { return m_attributes; }

	/// \brief return the set of attributes for this element
	ZEEM_API [[nodiscard]] const attribute_set &attributes() const { return m_attributes; }

	// --------------------------------------------------------------------

	/// \brief return the URI of the namespace for \a prefix
	ZEEM_API [[nodiscard]] std::string namespace_for_prefix(std::string_view prefix) const override;

	/// \brief return the prefix for the XML namespace with uri \a uri.
	/// \return The result is a pair of a std::string containing the actual prefix value
	/// and a boolean indicating if the namespace was found at all, needed since empty prefixes
	/// are allowed.
	ZEEM_API [[nodiscard]] std::pair<std::string, bool> prefix_for_namespace(std::string_view uri) const override;

	/// \brief move this element and optionally everyting beneath it to the
	///        specified namespace/prefix
	///
	/// \param prefix				The new prefix name
	/// \param uri					The new namespace uri
	/// \param recursive			Apply this to the child nodes as well
	/// \param including_attributes	Move the attributes to this new namespace as well
	ZEEM_API void move_to_name_space(const std::string &prefix, std::string_view uri,
		bool recursive, bool including_attributes);

	// --------------------------------------------------------------------

	/// \brief write the element to \a os
	ZEEM_API friend std::ostream &operator<<(std::ostream &os, const element &e);
	// 	friend class document;

	/// \brief return the concatenation of the content of all enclosed zeem::text nodes
	ZEEM_API [[nodiscard]] std::string get_content() const;

	/// \brief replace all existing child text nodes with a new single text node containing \a content
	ZEEM_API void set_content(std::string content);

	/// \brief return the value of attribute name \a qname or the empty string if not found
	ZEEM_API [[nodiscard]] std::string get_attribute(std::string_view qname) const;

	/// \brief set the value of attribute named \a qname to the value \a value
	ZEEM_API void set_attribute(std::string_view qname, std::string_view value);

	/// \brief The set_text method replaces any text node with the new text (call set_content)
	ZEEM_API void set_text(std::string s);

	/// The add_text method checks if the last added child is a text node,
	/// and if so, it appends the string to this node's value. Otherwise,
	/// it adds a new text node child with the new text.
	ZEEM_API void add_text(std::string s);

	/// To combine all adjacent child text nodes into one
	ZEEM_API void flatten_text();

	/** @cond */
	ZEEM_API void write(std::ostream &os, format_info fmt) const override;

  private:
	std::string m_qname;
	attribute_set m_attributes;
	/** @endcond */
};

// --------------------------------------------------------------------
/** @cond */
template <typename T>
node_list<T>::node_list(element_container *e)
	: basic_node_list(e)
{
	if constexpr (std::is_same_v<value_type, node>)
		m_header = e->m_header;
}

template <>
ZEEM_INLINE auto node_list<element>::insert(const_iterator pos, const element &e) -> iterator
{
	return iterator{ insert_impl(pos, std::make_unique<element>(e)) };
}

/// \brief insert a copy of \a e at position \a pos, moving its data
template <>
ZEEM_INLINE auto node_list<element>::insert(const_iterator pos, element &&e) -> iterator
{
	return iterator{ insert_impl(pos, std::make_unique<element>(std::forward<value_type>(e))) };
}

template <>
ZEEM_INLINE auto node_list<attribute>::insert(const_iterator pos, const attribute &e) -> iterator
{
	return iterator{ insert_impl(pos, std::make_unique<attribute>(e)) };
}

/// \brief insert a copy of \a e at position \a pos, moving its data
template <>
ZEEM_INLINE auto node_list<attribute>::insert(const_iterator pos, attribute &&e) -> iterator
{
	return iterator{ insert_impl(pos, std::make_unique<attribute>(std::forward<value_type>(e))) };
}

// NOLINTBEGIN(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-static-cast-downcast)
template <>
ZEEM_INLINE auto node_list<node>::insert(const_iterator pos, const value_type &e) -> iterator
{
	switch (e.type())
	{
		case node_type::element:
			return insert_impl(pos, std::make_unique<element>(static_cast<const element &>(e)));
		case node_type::text:
			return insert_impl(pos, std::make_unique<text>(static_cast<const text &>(e)));
		case node_type::attribute:
			return insert_impl(pos, std::make_unique<attribute>(static_cast<const attribute &>(e)));
		case node_type::comment:
			return insert_impl(pos, std::make_unique<comment>(static_cast<const comment &>(e)));
		case node_type::cdata:
			return insert_impl(pos, std::make_unique<cdata>(static_cast<const cdata &>(e)));
		case node_type::processing_instruction:
			return insert_impl(pos, std::make_unique<processing_instruction>(static_cast<const processing_instruction &>(e)));
		default:
			throw exception("internal error");
	}
}

/// \brief insert a copy of \a e at position \a pos, moving its data
template <>
ZEEM_INLINE auto node_list<node>::insert(const_iterator pos, value_type &&e) -> iterator
{
	switch (e.type())
	{
		case node_type::element:
			return insert_impl(pos, std::make_unique<element>(std::forward<element &&>(static_cast<element &&>(e))));
		case node_type::text:
			return insert_impl(pos, std::make_unique<text>(std::forward<text &&>(static_cast<text &&>(e))));
		case node_type::attribute:
			return insert_impl(pos, std::make_unique<attribute>(std::forward<attribute &&>(static_cast<attribute &&>(e))));
		case node_type::comment:
			return insert_impl(pos, std::make_unique<comment>(std::forward<comment &&>(static_cast<comment &&>(e))));
		case node_type::cdata:
			return insert_impl(pos, std::make_unique<cdata>(std::forward<cdata &&>(static_cast<cdata &&>(e))));
		case node_type::processing_instruction:
			return insert_impl(pos, std::make_unique<processing_instruction>(std::forward<processing_instruction &&>(static_cast<processing_instruction &&>(e))));
		default:
			throw exception("internal error");
	}
}

// NOLINTEND(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-static-cast-downcast)

template <typename T>
template <typename Pred>
void node_list<T>::sort(const Pred &pred)
{
	std::vector<node *> t;
	for (auto n = m_header->m_next; n != m_header; n = n->m_next)
		t.push_back(n);

	std::sort(t.begin(), t.end(), [pred](node *a, node *b)
		{ return pred(static_cast<T &>(*a), static_cast<T &>(*b)); });

	auto p = m_header;
	for (auto n : t)
	{
		n->m_prev = p;
		p->m_next = n;
		p = n;
	}
	p->m_next = m_header;
	m_header->m_prev = p;
}

/** @endcond */

// --------------------------------------------------------------------

/**
 * \brief This method fixes namespace attribute when transferring an element
 * 	   from one document to another (replaces prefixes e.g.)
 *
 * When moving an element from one document to another, we need to fix the
 * namespaces, make sure the destination has all the namespace specifications
 * required by the element and make sure the prefixes used are correct.
 * \param e		The element that is being transferred
 * \param source	The (usually) document element that was the source
 * \param dest		The (usually) document element that is the destination
 */

ZEEM_EXPORT ZEEM_API void fix_namespaces(element &e, const element &source, const element &dest);

} // namespace zeem

// --------------------------------------------------------------------
// structured binding support
/** @cond */

template <>
struct std::tuple_size<::zeem::attribute>
	: public std::integral_constant<std::size_t, 2>
{
};

template <>
struct std::tuple_element<0, ::zeem::attribute>
{
	using type = decltype(std::declval<::zeem::attribute>().name());
};

template <>
struct std::tuple_element<1, ::zeem::attribute>
{
	using type = decltype(std::declval<::zeem::attribute>().value());
};

/** @endcond */
