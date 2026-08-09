// Copyright (c) 2024-2026 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

/**
 * @file
 * definition of the serializer classes used to (de-)serialize XML data.
 */

#ifndef ZEEM_CXX_MODULE
# include "zeem/config.hpp"
# include "zeem/detail/charconv.hpp"
# include "zeem/node.hpp"
# include "zeem/parser.hpp"
# include "zeem/xpath.hpp"

# if ZEEM_USE_DATE_H
#  include <date/date.h>
#  include <date/tz.h>
# endif

# include <algorithm>
# include <array>
# include <charconv>
# include <chrono>
# include <exception>
# include <map>
# include <optional>
# include <regex>
# include <source_location>
# include <stdexcept>
# include <string>
# include <system_error>
# include <type_traits>
#endif

namespace zeem
{

// --------------------------------------------------------------------
/// \brief A template boilerplate for conversion of basic types to or
/// from strings.
///
/// Each specialization should provide a static to_string and a from_string
/// method

ZEEM_EXPORT template <typename T>
struct value_serializer;

/// @ref value_serializer implementation for booleans
template <>
struct value_serializer<bool>
{
	static constexpr std::string type_name() { return "xsd:boolean"; }
	static constexpr std::string to_string(bool value) { return value ? "true" : "false"; }
	static constexpr bool from_string(std::string_view value) { return value == "true" or value == "1" or value == "yes"; }
};

/// @ref value_serializer implementation for std::strings
template <>
struct value_serializer<std::string>
{
	static constexpr std::string type_name() { return "xsd:string"; }
	static constexpr std::string to_string(std::string value) { return value; }
	static constexpr std::string from_string(std::string_view value) { return std::string{ value }; }
};

/// @ref value_serializer implementation for numbers
template <typename T>
struct char_conv_serializer
{
	using value_type = T;

	static constexpr std::string derived_type_name()
	{
		using value_serializer_type = value_serializer<value_type>;
		return value_serializer_type::type_name();
	}

	static std::string to_string(value_type value)
	{
		char b[32];
		if (auto r = std::to_chars(b, b + sizeof(b), value); r.ec == std::errc{})
			return { b, r.ptr };
		else
			throw std::system_error(std::make_error_code(r.ec), "Error converting value to string for type " + derived_type_name());
	}

	static value_type from_string(std::string_view value)
	{
		value_type result{};

		auto r = from_chars(value.data(), value.data() + value.length(), result);
		if (r.ec != std::errc{} or r.ptr != value.data() + value.length())
			throw std::system_error(std::make_error_code(r.ec), "Error converting value '" + std::string{ value } + "' to type " + derived_type_name());

		return result;
	}
};

/// @ref value_serializer implementation for small int8_t
template <>
struct value_serializer<int8_t> : char_conv_serializer<int8_t>
{
	static std::string type_name() { return "xsd:byte"; }
};

/// @ref value_serializer implementation for uint8_t
template <>
struct value_serializer<uint8_t> : char_conv_serializer<uint8_t>
{
	static std::string type_name() { return "xsd:unsignedByte"; }
};

/// @ref value_serializer implementation for int16_t
template <>
struct value_serializer<int16_t> : char_conv_serializer<int16_t>
{
	static std::string type_name() { return "xsd:short"; }
};

/// @ref value_serializer implementation for uint16_t
template <>
struct value_serializer<uint16_t> : char_conv_serializer<uint16_t>
{
	static std::string type_name() { return "xsd:unsignedShort"; }
};

/// @ref value_serializer implementation for int32_t
template <>
struct value_serializer<int32_t> : char_conv_serializer<int32_t>
{
	static std::string type_name() { return "xsd:int"; }
};

/// @ref value_serializer implementation for uint32_t
template <>
struct value_serializer<uint32_t> : char_conv_serializer<uint32_t>
{
	static std::string type_name() { return "xsd:unsignedInt"; }
};

/// @ref value_serializer implementation for int64_t
template <>
struct value_serializer<int64_t> : char_conv_serializer<int64_t>
{
	static std::string type_name() { return "xsd:long"; }
};

/// @ref value_serializer implementation for uint64_t
template <>
struct value_serializer<uint64_t> : char_conv_serializer<uint64_t>
{
	static std::string type_name() { return "xsd:unsignedLong"; }
};

/// @ref value_serializer implementation for float
template <>
struct value_serializer<float> : char_conv_serializer<float>
{
	static std::string type_name() { return "xsd:float"; }
};

/// @ref value_serializer implementation for double
template <>
struct value_serializer<double> : char_conv_serializer<double>
{
	static std::string type_name() { return "xsd:double"; }
};

/**
 * \brief value_serializer for enum values
 *
 * This class is used to (de-)serialize enum values. To map enum
 * values to a string you should use the singleton instance
 * accessible through instance() and then call the operator()
 * members assinging each of the enum values with their respective
 * string.
 *
 * A recent addition is the init() call to initialize the instance
 */

/** @cond */
template <typename T>
	requires std::is_enum_v<T>
struct value_serializer<T>
{
	using value_map_type = std::map<T, std::string>;
	using value_map_value_type = typename value_map_type::value_type;

  private:
	std::string m_type_name;
	value_map_type m_value_map;

	value_serializer(std::string name, value_map_type values)
		: m_type_name(std::move(name))
		, m_value_map(std::move(values))
	{
	}

  public:
	/// \brief Initialize a new instance of value_serializer for this enum, with name and a set of name/value pairs
	static void init(std::string_view name, value_map_type values)
	{
		create(std::string{ name }, std::move(values));
	}

	/// \brief Initialize a new anonymous instance of value_serializer for this enum with a set of name/value pairs
	static void init(value_map_type values)
	{
		create("", std::move(values));
	}

	/// \brief Return the singleton instance (must be initialized first via init())
	static value_serializer &instance()
	{
		return create({}, {});
	}

	/// \brief Return the singleton instance, setting the type name
	static value_serializer &instance(std::string_view name)
	{
		return create(std::string{ name }, {});
	}

  private:
	static value_serializer &create(std::string name, value_map_type values)
	{
		static value_serializer s_instance(std::move(name), std::move(values));
		return s_instance;
	}

  public:

	value_serializer &operator()(T v, std::string_view name)
	{
		m_value_map[v] = name;
		return *this;
	}

	value_serializer &operator()(std::string name, T v)
	{
		m_value_map[v] = std::move(name);
		return *this;
	}

	static std::string type_name()
	{
		return instance().m_type_name;
	}

	static std::string to_string(T value)
	{
		return instance().m_value_map.at(value);
	}

	static T from_string(std::string_view value)
	{
		for (auto &t : instance().m_value_map)
		{
			if (t.second == value)
				return t.first;
		}
		throw std::invalid_argument(std::format("{} is not valid for enum {}", value, type_name()));
	}

	static bool empty()
	{
		return instance().m_value_map.empty();
	}

	std::vector<std::string> values() const
	{
		std::vector<std::string> result;
		for (const auto &[_, value] : m_value_map)
			result.emplace_back(value);
		return result;
	}
};

/** @endcond */

// --------------------------------------------------------------------
// date/time support

/// \brief to_string/from_string for std::chrono::system_clock::time_point
/// time is always assumed to be UTC
/// For a specification, see https://www.iso20022.org/standardsrepository/type/ISODateTime

template <>
struct value_serializer<std::chrono::system_clock::time_point>
{
	using time_type = std::chrono::system_clock::time_point;

	static std::string type_name() { return "xsd:dateTime"; }

	/// to_string the time as YYYY-MM-DDThh:mm:ssZ (zero UTC offset)
	static std::string to_string(const time_type &v)
	{
		return std::format("{0:%F}T{0:%T}Z", v);
	}

	/// from_string according to ISO8601 rules.
	/// If Zulu time is specified, then the parsed xsd:dateTime is returned.
	/// If an UTC offset is present, then the offset is added to the xsd:dateTime, this yields UTC.
	/// If no UTC offset is present, then the xsd:dateTime is assumed to be local time and converted to UTC.
	static time_type from_string(std::string_view s)
	{
		time_type result;

		static const std::regex kRX(R"(^(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}(?::\d{2}(?:\.\d+)?)?)(Z|([-+]\d{2})(?::(\d{2}))?)?)");
		std::cmatch m;

		if (not std::regex_match(s.data(), s.data() + s.length(), m, kRX))
			throw std::runtime_error("Invalid date format");

		// std::istringstream is{ m[1] };

		struct membuf : public std::streambuf
		{
			membuf(std::string_view s)
			{
				auto text = const_cast<char *>(s.data());
				this->setg(text, text, text + s.size());
			}
		} buffer(s);
		std::istream is(&buffer);

#if ZEEM_USE_DATE_H
		if (m[1].length() == 16)
			date::from_stream(is, "%FT%H:%M", result);
		else if (m[2].matched and m[2] != "Z")
			date::from_stream(is, "%FT%T%z", result);
		else
			date::from_stream(is, "%FT%T", result);

		if (auto zone = date::current_zone(); not m[2].matched and zone)
		{
			auto info = zone->get_info(result);
			result -= info.offset;
		}
#else
		if (m[1].length() == 16)
			std::chrono::from_stream(is, "%FT%H:%M", result);
		else if (m[2].matched and m[2] != "Z")
			std::chrono::from_stream(is, "%FT%T%z", result);
		else
			std::chrono::from_stream(is, "%FT%T", result);

		if (auto zone = std::chrono::current_zone(); not m[2].matched and zone)
		{
			auto info = zone->get_info(result);
			result -= info.offset;
		}
#endif

		if (is.bad() or is.fail())
			throw std::runtime_error("invalid formatted date");

		return result;
	}
};

/// \brief to_string/from_string for std::chrono::sys_days
/// For a specification, see https://www.iso20022.org/standardsrepository/type/ISODateTime

template <>
struct value_serializer<std::chrono::sys_days>
{
	static std::string type_name() { return "xsd:date"; }

	/// to_string the date as YYYY-MM-DD
	static std::string to_string(const std::chrono::sys_days &v)
	{
		return std::format("{:%F}", v);
	}

	/// from_string according to ISO8601 rules.
	static std::chrono::sys_days from_string(std::string_view s)
	{
		std::chrono::sys_days result;

		std::stringstream is;
		is << s;

#if ZEEM_USE_DATE_H
		date::from_stream(is, "%F", result);
#else
		std::chrono::from_stream(is, "%F", result);
#endif

		if (is.bad() or is.fail())
			throw std::runtime_error("invalid formatted date");

		return result;
	}
};

/** @cond */

ZEEM_EXPORT template <typename T>
using serialize_value_t = decltype(std::declval<value_serializer<T> &>().from_string(std::declval<std::string_view>()));

ZEEM_EXPORT template <typename T, typename Archive>
using serialize_function = decltype(std::declval<T &>().serialize(std::declval<Archive &>(), std::declval<uint64_t>()));

ZEEM_EXPORT template <typename T, typename Archive, typename = void>
struct has_serialize : std::false_type
{
};

template <typename T, typename Archive>
	requires(std::is_class_v<T>)
struct has_serialize<T, Archive>
{
	static constexpr bool value = is_detected_v<serialize_function, T, Archive>;
};

ZEEM_EXPORT template <typename T, typename S>
ZEEM_INLINE constexpr bool has_serialize_v = has_serialize<T, S>::value;

ZEEM_EXPORT template <typename T, typename S, typename = void>
struct is_serializable_array_type : std::false_type
{
};

ZEEM_EXPORT template <typename T>
using value_type_t = typename T::value_type;

ZEEM_EXPORT template <typename T>
using iterator_t = typename T::iterator;

ZEEM_EXPORT template <typename T>
using std_string_npos_t = decltype(T::npos);

/// Struct used to detect whether type \a T is serializable
ZEEM_EXPORT template <typename T, typename S>
struct is_serializable_type
{
	using value_type = std::remove_cvref_t<T>;
	static constexpr bool value =
		is_detected_v<serialize_value_t, value_type> or
		has_serialize_v<value_type, S>;
};

ZEEM_EXPORT template <typename T, typename S>
ZEEM_INLINE constexpr bool is_serializable_type_v = is_serializable_type<T, S>::value;

template <typename T, typename S>
	requires(
		is_detected_v<value_type_t, T> and
		is_detected_v<iterator_t, T> and
		not is_detected_v<std_string_npos_t, T>)
struct is_serializable_array_type<T, S>
{
	static constexpr bool value = is_serializable_type_v<typename T::value_type, S>;
};

ZEEM_EXPORT template <typename T, typename S>
ZEEM_INLINE constexpr bool is_serializable_array_type_v = is_serializable_array_type<T, S>::value;

/** @endcond */
// --------------------------------------------------------------------

ZEEM_EXPORT struct serializer;
ZEEM_EXPORT struct deserializer;

/**
 * @brief base struct to capture named values in a structure for serializing
 */
ZEEM_EXPORT template <typename T>
class name_value_pair
{
  public:
	/// @brief constructor
	name_value_pair(std::string name, T &value)
		: m_name(std::move(name))
		, m_value(value)
	{
	}

	/** @cond */
	name_value_pair(const name_value_pair &) = default;
	name_value_pair(name_value_pair &&) = default;
	name_value_pair &operator=(const name_value_pair &) = default;
	name_value_pair &operator=(name_value_pair &&) = default;
	/** @endcond */

	[[nodiscard]] const std::string &name() const { return m_name; }
	[[nodiscard]] T &value() const { return m_value; }

	/** @cond */
  private:
	std::string m_name;
	T &m_value;
	/** @endcond */
};

/// @brief name value pair to create elements
ZEEM_EXPORT template <typename T>
class element_nvp : public name_value_pair<T>
{
  public:
	element_nvp(std::string name, T &value)
		: name_value_pair<T>(std::move(name), value)
	{
	}
};

/// @brief name value pair to create attributes
ZEEM_EXPORT template <typename T>
class attribute_nvp : public name_value_pair<T>
{
  public:
	attribute_nvp(std::string name, T &value)
		: name_value_pair<T>(std::move(name), value)
	{
	}
};

/**
 * @brief Create a name/value pair for serializing to and from an XML element
 */
ZEEM_EXPORT template <typename T>
constexpr attribute_nvp<T> make_attribute_nvp(std::string name, T &value)
{
	return attribute_nvp(std::move(name), value);
}

/**
 * @brief Create a name/value pair for serializing to and from an XML attribute
 */
ZEEM_EXPORT template <typename T>
constexpr element_nvp<T> make_element_nvp(std::string name, T &value)
{
	return element_nvp(std::move(name), value);
}

/**
 * serializer and deserializer are classes that can be used
 * to initiate the serialization. They are the Archive classes that are
 * the first parameter to the templated function 'serialize' in the classes
 * that can be serialized. (See boost::serialization for more info).
 */

/**
 * @brief serializer is the class that initiates the serialization process.
 */

struct serializer
{
	/// @brief constructor, write to \a node
	explicit serializer(element_container &node)
		: m_node(node)
	{
	}

	/** @cond */

	template <typename T>
	serializer &operator&(const element_nvp<T> &rhs)
	{
		return serialize_element(rhs.name(), rhs.value());
	}

	template <typename T>
	serializer &operator&(const attribute_nvp<T> &rhs)
	{
		return serialize_attribute(rhs.name(), rhs.value());
	}

	template <typename T>
	serializer &serialize_element(const T &data);

	template <typename T>
	serializer &serialize_element(std::string_view name, const T &data);

	template <typename T>
	serializer &serialize_attribute(std::string_view name, const T &data);

  private:
	element_container &m_node;

	/** @endcond */
};

/**
 * @brief deserializer is the class that initiates the deserialization process.
 *
 */

struct deserializer
{
	/// @brief constructor, read from \a node
	explicit deserializer(const element_container &node)
		: m_node(node)
	{
	}

	/** @cond */

	template <typename T>
	deserializer &operator&(const element_nvp<T> &rhs)
	{
		return deserialize_element(rhs.name(), rhs.value());
	}

	template <typename T>
	deserializer &operator&(const attribute_nvp<T> &rhs)
	{
		return deserialize_attribute(rhs.name(), rhs.value());
	}

	template <typename T>
	deserializer &deserialize_element(T &data);

	template <typename T>
	deserializer &deserialize_element(std::string_view name, T &data);

	template <typename T>
	deserializer &deserialize_attribute(std::string_view name, T &data);

  private:
	const element_container &m_node;

	/** @endcond */
};

/**
 * @brief This type_map contains the complex types collected by the @ref schema_creator
 */

ZEEM_EXPORT using type_map = std::map<std::string, element>;

/**
 * @brief schema creator is used to create XML Schema's for data that is serialized or deserialized.
 */

ZEEM_EXPORT struct schema_creator
{
	schema_creator()
		: schema_creator(std::make_unique<type_map>(), std::make_unique<element>(element{ "xsd:schema", { { "xmlns:xsd", "http://www.w3.org/2001/XMLSchema" } } }))
	{
	}

	schema_creator(type_map &types, element &schema)
		: m_schema(schema)
		, m_types(types)
	{
	}

	void set_ns_prefix(std::string ns_prefix)
	{
		m_ns_prefix = std::move(ns_prefix);
	}

	template <typename T>
	schema_creator &operator&(const element_nvp<T> &rhs)
	{
		return add_element(rhs.name(), rhs.value());
	}

	template <typename T>
	schema_creator &operator&(const attribute_nvp<T> &rhs)
	{
		return add_attribute(rhs.name(), rhs.value());
	}

	template <typename T>
	schema_creator &add_element(std::string_view name, const T &value);

	template <typename T>
	schema_creator &add_attribute(std::string_view name, const T &value);

	document schema(std::string name) const
	{
		document doc(R"(<xsd:schema xmlns:xsd="http://www.w3.org/2001/XMLSchema"/>)");
		auto e = doc.child()->emplace_back(element{ "xsd:element", { { "name", name } } });
		auto t = e->emplace_back("xsd:complexType");
		auto s = t->emplace_back("xsd:sequence");

		for (auto &e : m_schema)
			s->emplace_back(e);

		for (const auto &[_, type] : m_types)
			doc.child()->emplace_back(type);

		return doc;
	}

  private:
	schema_creator(std::unique_ptr<type_map> types, std::unique_ptr<element> schema)
		: schema_creator(*types, *schema)
	{
		m_schema_store = std::move(schema);
		m_types_store = std::move(types);
	}

	std::unique_ptr<element> m_schema_store;
	std::unique_ptr<type_map> m_types_store;

	element &m_schema;
	type_map &m_types;

	std::string m_ns_prefix;
};

// --------------------------------------------------------------------

/// \brief Return a prefixed type name
inline std::string get_prefixed_type_name(const std::string &prefix, std::string type_name)
{
	if (prefix.empty() or type_name.find(':') != std::string::npos)
		return type_name;
	else
	 	return prefix + ':' + type_name;
}

/**
 * @brief Type serializer objects can serialize various types,
 * each has its own template specialization.
 */

ZEEM_EXPORT template <typename T>
struct type_serializer;

/** @cond */

template <typename T, std::size_t N>
struct type_serializer<T[N]>
{
	using value_type = std::remove_cvref_t<T>;
	using type_serializer_type = type_serializer<value_type>;

	static std::string type_name() { return type_serializer_type::type_name(); }

	static void serialize_child(element_container &n, std::string_view name, const value_type (&value)[N])
	{
		for (const value_type &v : value)
			type_serializer_type::serialize_child(n, name, v);
	}

	static void deserialize_child(const element_container &n, std::string_view name, value_type (&value)[N])
	{
		std::size_t ix = 0;
		for (auto &e : n)
		{
			if (e.name() != name)
				continue;

			value_type v = {};
			type_serializer_type::deserialize_child(e, ".", v);

			value[ix] = std::move(v);
			++ix;

			if (ix >= N)
				break;
		}
	}

	static element schema(std::string_view name, std::string prefix)
	{
		return element{
			"xsd:element",
			{ //
				{ "name", name },
				{ "type", get_prefixed_type_name(prefix, type_serializer_type::type_name()) },
				{ "minOccurs", std::to_string(N) },
				{ "maxOccurs", std::to_string(N) } }
		};
	}

	static void register_type(type_map &types, std::string prefix)
	{
		type_serializer_type::register_type(types, prefix);
	}
};

template <typename T>
	requires std::is_enum_v<T>
struct type_serializer<T>
	: public value_serializer<T>
{
	using value_type = T;
	using value_serializer_type = value_serializer<T>;
	using value_serializer_type::type_name;

	static std::string serialize_value(const T &value)
	{
		return value_serializer_type::to_string(value);
	}

	static T deserialize_value(std::string_view value)
	{
		return value_serializer_type::from_string(value);
	}

	static void serialize_child(element_container &n, std::string_view name, const value_type &value)
	{
		if (name.empty() or name == ".")
		{
			if (n.type() == node_type::element)
				static_cast<element &>(n).set_content(value_serializer_type::to_string(value));
		}
		else
			n.emplace_back(name)->set_content(value_serializer_type::to_string(value));
	}

	static void deserialize_child(const element_container &n, std::string_view name, value_type &value)
	{
		value = value_type();

		if (name.empty() or name == ".")
		{
			if (n.type() == node_type::element)
				value = value_serializer_type::from_string(static_cast<const element &>(n).get_content());
		}
		else
		{
			auto e = std::find_if(n.begin(), n.end(), [name](auto &e)
				{ return e.name() == name; });
			if (e != n.end())
				value = value_serializer_type::from_string(e->get_content());
		}
	}

	static element schema(std::string_view name, std::string prefix)
	{
		return element{
			"xsd:element",
			{ //
				{ "name", name },
				{ "type", get_prefixed_type_name(prefix, value_serializer_type::type_name()) },
				{ "minOccurs", "1" },
				{ "maxOccurs", "1" } }
		};
	}

	static void register_type(type_map &types, std::string prefix)
	{
		element n("xsd:simpleType", { { "name", value_serializer_type::type_name() } });

		element restriction("xsd:restriction", { { "base", "xsd:string" } });

		for (std::string v : value_serializer_type::instance().values())
			restriction.emplace_back(element{ "xsd:enumeration", { { "value", v } } });

		n.emplace_back(std::move(restriction));
		types[type_name()] = std::move(n);
	}
};

// code to serialize structs.
// struct_serializer_archive is a helper class to be used as Archive

template <typename Archive, typename T>
struct struct_serializer
{
	static void serialize(Archive &stream, T &data)
	{
		data.serialize(stream, 0U);
	}
};

template <typename T>
	requires has_serialize_v<T, serializer>
struct type_serializer<T>
{
	using value_type = std::remove_cvref_t<T>;

	// the name of this type
	std::string m_type_name;

	static std::string type_name() { return instance().m_type_name; }
	void type_name(std::string_view name) { m_type_name = name; }

	static type_serializer &instance()
	{
		static type_serializer s_instance{ value_type::type_name() };
		return s_instance;
	}

	static void serialize_child(element_container &n, std::string_view name, const value_type &value)
	{
		if (name.empty() or name == ".")
		{
			serializer sr(n);
			const_cast<value_type &>(value).serialize(sr, 0UL);
		}
		else
		{
			element *e = static_cast<element *>(n.emplace_back(name));
			serializer sr(*e);
			const_cast<value_type &>(value).serialize(sr, 0UL);
		}
	}

	static void deserialize_child(const element_container &n, std::string_view name, value_type &value)
	{
		value = value_type();

		if (name.empty() or name == ".")
		{
			deserializer sr(n);
			value.serialize(sr, 0UL);
		}
		else
		{
			auto e = std::find_if(n.begin(), n.end(), [name](auto &e)
				{ return e.name() == name; });
			if (e != n.end())
			{
				deserializer sr(*e);
				value.serialize(sr, 0UL);
			}
		}
	}

	static element schema(std::string_view name, std::string prefix)
	{
		return element{
			"xsd:element",
			{ //
				{ "name", name },
				{ "type", get_prefixed_type_name(prefix, value_type::type_name()) },
				{ "minOccurs", "1" },
				{ "maxOccurs", "1" } }
		};
	}

	static void register_type(type_map &types, std::string prefix)
	{
		auto name = value_type::type_name();

		element n("xsd:complexType", { { "name", name } });

		element sequence("xsd:sequence");
		using archive = struct_serializer<schema_creator, value_type>;
		schema_creator schema(types, sequence);
		schema.set_ns_prefix(prefix);

		value_type v;
		archive::serialize(schema, v);

		n.emplace_back(std::move(sequence));

		types[name] = std::move(n);
	}
};

template <typename T>
struct type_serializer<std::optional<T>>
{
	using value_type = T;
	using container_type = std::optional<value_type>;
	using type_serializer_type = type_serializer<value_type>;

	static std::string type_name() { return type_serializer_type::type_name(); }

	static void serialize_child(element_container &n, std::string_view name, const container_type &value)
	{
		if (value.has_value())
			type_serializer_type::serialize_child(n, name, *value);
	}

	static void deserialize_child(const element_container &n, std::string_view name, container_type &value)
	{
		for (auto &e : n)
		{
			if (e.name() != name)
				continue;

			value_type v = {};
			type_serializer_type::deserialize_child(e, ".", v);
			value.emplace(std::move(v));
		}
	}

	static element schema(std::string_view name, std::string prefix)
	{
		return element{
			"xsd:element",
			{ //
				{ "name", name },
				{ "type", get_prefixed_type_name(prefix, type_serializer_type::type_name()) },
				{ "minOccurs", "0" },
				{ "maxOccurs", "1" } }
		};
	}

	static void register_type(type_map &types, std::string prefix)
	{
		type_serializer_type::register_type(types, prefix);
	}
};

// nice trick to enforce order in template selection
template <unsigned N>
struct priority_tag /** @cond */ : priority_tag<N - 1> /** @endcond */
{
};

template <>
struct priority_tag<0>
{
};

template <typename T>
	requires is_serializable_array_type_v<T, serializer>
struct type_serializer<T>
{
	using container_type = std::remove_cvref_t<T>;
	using value_type = value_type_t<container_type>;
	using type_serializer_type = type_serializer<value_type>;

	static std::string type_name() { return type_serializer_type::type_name(); }

	static void serialize_child(element_container &n, std::string_view name, const container_type &value)
	{
		for (const value_type &v : value)
			type_serializer_type::serialize_child(n, name, v);
	}

	template <std::size_t N>
	static auto deserialize_array(const element_container &n, std::string_view name,
		std::array<value_type, N> &value, [[maybe_unused]] priority_tag<2> pt)
	{
		std::size_t ix = 0;
		for (auto &e : n)
		{
			if (e.name() != name)
				continue;

			value_type v = {};
			type_serializer_type::deserialize_child(e, ".", v);

			value[ix] = std::move(v);
			++ix;

			if (ix >= N)
				break;
		}
	}

	template <typename A>
	static auto deserialize_array(const element_container &n, std::string_view name, A &arr, [[maybe_unused]] priority_tag<1> pt)
		-> decltype(arr.reserve(std::declval<typename container_type::size_type>()),
			void())
	{
		arr.reserve(n.size());

		for (auto &e : n)
		{
			if (e.name() != name)
				continue;

			value_type v = {};
			type_serializer_type::deserialize_child(e, ".", v);

			arr.emplace_back(std::move(v));
		}
	}

	static void deserialize_array(const element_container &n, std::string_view name, container_type &arr, [[maybe_unused]] priority_tag<0> pt)
	{
		for (auto &e : n)
		{
			if (e.name() != name)
				continue;

			value_type v = {};
			type_serializer_type::deserialize_child(e, ".", v);

			arr.emplace_back(std::move(v));
		}
	}

	static void deserialize_child(const element_container &n, std::string_view name, container_type &value)
	{
		type_serializer::deserialize_array(n, name, value, priority_tag<2>{});
	}

	template <std::size_t N>
	static element schema_array(std::string_view name, std::string prefix,
		[[maybe_unused]] const std::array<value_type, N> &value, [[maybe_unused]] priority_tag<1> pt)
	{
		return element{
			"xsd:element",
			{ //
				{ "name", name },
				{ "type", get_prefixed_type_name(prefix, type_serializer_type::type_name()) },
				{ "minOccurs", std::to_string(N) },
				{ "maxOccurs", std::to_string(N) } }
		};
	}

	static element schema_array(std::string_view name, std::string prefix,
		[[maybe_unused]] const container_type &arr, [[maybe_unused]] priority_tag<0> pt)
	{
		return element{
			"xsd:element",
			{ //
				{ "name", name },
				{ "type", get_prefixed_type_name(prefix, type_serializer_type::type_name()) },
				{ "minOccurs", "0" },
				{ "maxOccurs", "unbounded" } }
		};
	}

	static element schema(std::string_view name, std::string prefix)
	{
		return schema_array(name, prefix, container_type{}, priority_tag<1>());
	}

	static void register_type(type_map &types, std::string prefix)
	{
		type_serializer_type::register_type(types, prefix);
	}
};

template <typename T>
struct type_serializer
{
	using value_type = std::remove_cvref_t<T>;
	using value_serializer_type = value_serializer<value_type>;

	static std::string type_name() { return value_serializer_type::type_name(); }

	static std::string serialize_value(const T &value)
	{
		return value_serializer_type::to_string(value);
	}

	static T deserialize_value(std::string_view value)
	{
		return value_serializer_type::from_string(value);
	}

	static void serialize_child(element_container &n, std::string_view name, const value_type &value)
	{
		if (name.empty() or name == ".")
		{
			if (n.type() == node_type::element)
				static_cast<element &>(n).set_content(value_serializer_type::to_string(value));
		}
		else
			n.emplace_back(name)->set_content(value_serializer_type::to_string(value));
	}

	static void deserialize_child(const element_container &n, std::string_view name, value_type &value)
	{
		value = {};

		if (name.empty() or name == ".")
		{
			if (n.type() == node_type::element)
				value = value_serializer_type::from_string(static_cast<const element &>(n).get_content());
		}
		else
		{
			auto e = std::find_if(n.begin(), n.end(), [name](auto &e)
				{ return e.name() == name; });
			if (e != n.end())
				value = value_serializer_type::from_string(e->get_content());
		}
	}

	static element schema(std::string_view name, std::string prefix)
	{
		return element{
			"xsd:element",
			{ { "name", name },
				{ "type", get_prefixed_type_name(prefix, value_serializer_type::type_name()) },
				{ "minOccurs", "1" },
				{ "maxOccurs", "1" } }
		};
	}

	static void register_type(type_map &, std::string_view)
	{
	}
};

// And finally, the implementation of serializer, deserializer and schema_creator.

template <typename T>
serializer &serializer::serialize_element(const T &value)
{
	using value_type = std::remove_cvref_t<T>;
	using type_serializer = type_serializer<value_type>;

	type_serializer::serialize_child(m_node, "", value);

	return *this;
}

template <typename T>
serializer &serializer::serialize_element(std::string_view name, const T &value)
{
	using value_type = std::remove_cvref_t<T>;
	using type_serializer = type_serializer<value_type>;

	type_serializer::serialize_child(m_node, name, value);

	return *this;
}

template <typename T>
serializer &serializer::serialize_attribute(std::string_view name, const T &value)
{
	using value_type = std::remove_cvref_t<T>;
	using type_serializer = type_serializer<value_type>;

	if (m_node.type() == node_type::element)
		static_cast<element &>(m_node).attributes().emplace(name, type_serializer::serialize_value(value));

	return *this;
}

template <typename T>
deserializer &deserializer::deserialize_element(T &value)
{
	using value_type = std::remove_cvref_t<T>;
	using type_serializer = type_serializer<value_type>;

	type_serializer::deserialize_child(m_node, "", value);

	return *this;
}

template <typename T>
deserializer &deserializer::deserialize_element(std::string_view name, T &value)
{
	using value_type = std::remove_cvref_t<T>;
	using type_serializer = type_serializer<value_type>;

	type_serializer::deserialize_child(m_node, name, value);

	return *this;
}

template <typename T>
deserializer &deserializer::deserialize_attribute(std::string_view name, T &value)
{
	using value_type = std::remove_cvref_t<T>;
	using type_serializer = type_serializer<value_type>;

	if (m_node.type() == node_type::element)
	{
		std::string attr = static_cast<const element &>(m_node).get_attribute(name);
		if (not attr.empty())
			value = type_serializer::deserialize_value(attr);
	}
	return *this;
}

// Schema creation
template <typename T>
schema_creator &schema_creator::add_element(std::string_view name, const T & /* value */)
{
	using value_type = std::remove_cv_t<T>;
	using type_serializer = type_serializer<value_type>;

	m_schema.emplace_back(type_serializer::schema(name, m_ns_prefix));

	std::string type_name = type_serializer::type_name();

	// we might be known already
	if (m_types.find(type_name) == m_types.end())
		type_serializer::register_type(m_types, m_ns_prefix);

	return *this;
}

template <typename T>
schema_creator &schema_creator::add_attribute(std::string_view name, const T & /* value */)
{
	using value_type = std::remove_cv_t<T>;
	using type_serializer = type_serializer<value_type>;

	element n("xsd:attribute");

	std::string type_name = type_serializer::type_name();

	n.set_attribute("name", name);
	n.set_attribute("type", type_name);

	if (m_types.find(type_name) == m_types.end())
		type_serializer::register_type(m_types, m_ns_prefix);

	assert(m_schema.parent() != nullptr);
	if (m_schema.parent() != nullptr)
		m_schema.parent()->emplace_back(std::move(n));

	return *this;
}

/** @endcond */

// --------------------------------------------------------------------
// Convenience routines

namespace detail
{
	ZEEM_EXPORT template <typename T>
	concept ElementContainer = std::is_base_of_v<zeem::element_container, T>;

	/**
	 * @brief Write out \a value into XML into document or element \a e
	 */
	ZEEM_EXPORT template <typename T>
	void to_xml(zeem::element_container &e, const T &value)
	{
		serializer sr(e);
		sr.serialize_element(value);
	}

	/**
	 * @brief Write out \a value into XML into document or element \a e
	 * using \a name as name for the element to create.
	 */

	ZEEM_EXPORT template <typename T>
	void to_xml(zeem::element_container &e, std::string_view name, const T &value)
	{
		serializer sr(e);
		sr.serialize_element(name, value);
	}

	// Using customization point objects
	/** @cond */

	struct to_xml_fn
	{
		template <typename T>
		auto operator()(ElementContainer auto &e, T &&val) const
			noexcept(noexcept(to_xml(e, std::forward<T>(val))))
				-> decltype(to_xml(e, std::forward<T>(val)))
		{
			return to_xml(e, std::forward<T>(val));
		}

		template <typename T>
		auto operator()(ElementContainer auto &e, std::string_view name, T &&val) const
			noexcept(noexcept(to_xml(e, name, std::forward<T>(val))))
				-> decltype(to_xml(e, name, std::forward<T>(val)))
		{
			return to_xml(e, name, std::forward<T>(val));
		}
	};

	/** @endcond */

	/**
	 * @brief Read in \a value from the XML in document or element \a e
	 */

	ZEEM_EXPORT template <typename T>
	void from_xml(const zeem::element_container &e, T &value)
	{
		deserializer dsr(e);
		dsr.deserialize_element(value);
	}

	/**
	 * @brief Read in \a value from the XML in document or element \a e
	 * using \a name as name for the element to use.
	 */

	ZEEM_EXPORT template <typename T>
	void from_xml(const zeem::element_container &e, std::string_view name, T &value)
	{
		deserializer dsr(e);
		dsr.deserialize_element(name, value);
	}

	/** @cond */

	struct from_xml_fn
	{
		template <typename T>
		auto operator()(const ElementContainer auto &e, T &&val) const
			noexcept(noexcept(from_xml(e, std::forward<T>(val))))
				-> decltype(from_xml(e, std::forward<T>(val)))
		{
			return from_xml(e, std::forward<T>(val));
		}

		template <typename T>
		auto operator()(const ElementContainer auto &e, std::string_view name, T &&val) const
			noexcept(noexcept(from_xml(e, name, std::forward<T>(val))))
				-> decltype(from_xml(e, name, std::forward<T>(val)))
		{
			return from_xml(e, name, std::forward<T>(val));
		}
	};

	/** @endcond */

} // namespace detail

/// @brief The customization point object for to_xml
ZEEM_EXPORT inline constexpr detail::to_xml_fn to_xml{};

/// @brief The customization point object for from_xml
ZEEM_EXPORT inline constexpr detail::from_xml_fn from_xml{};

} // namespace zeem
