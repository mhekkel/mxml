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
/// definition of the serializer classes used to (de-)serialize XML data.

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cinttypes>
#include <map>
#include <optional>
#include <string>
#include <system_error>

#include <experimental/type_traits>

export module mxml:serialize;

import :node;

namespace mxml
{

// --------------------------------------------------------------------

// --------------------------------------------------------------------
/// \brief A template boilerplate for conversion of basic types to or
/// from strings.
///
/// Each specialization should provide a static to_string and a from_string
/// method as well as a type_name method. This type_name is used in e.g.
/// constructing WSDL files.

export template <typename T>
struct value_serializer;

/// @ref value_serializer implementation for booleans
template <>
struct value_serializer<bool>
{
	static constexpr std::string type_name() { return "xsd:boolean"; }
	static constexpr std::string_view to_string(bool value) { return value ? "true" : "false"; }
	static constexpr bool from_string(std::string_view value) { return value == "true" or value == "1" or value == "yes"; }
};

/// @ref value_serializer implementation for std::strings
template <>
struct value_serializer<std::string>
{
	static constexpr std::string type_name() { return "xsd:string"; }
	static std::string_view to_string(std::string_view value) { return value; }
	static const std::string &to_string(const std::string &value) { return value; }
	static std::string_view from_string(std::string_view value) { return value; }
};

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

		auto r = std::from_chars(value.data(), value.data() + value.length(), result);

		if (r.ec != std::errc{} or r.ptr != value.data() + value.length())
			throw std::system_error(std::make_error_code(r.ec), "Error converting value '" + std::string{ value } + "' to type " + derived_type_name());

		return result;
	}
};

/// @ref value_serializer implementation for small int8_t
template <>
struct value_serializer<int8_t> : char_conv_serializer<int8_t>
{
	static constexpr std::string type_name() { return "xsd:byte"; }
};

/// @ref value_serializer implementation for uint8_t
template <>
struct value_serializer<uint8_t> : char_conv_serializer<uint8_t>
{
	static constexpr std::string type_name() { return "xsd:unsignedByte"; }
};

/// @ref value_serializer implementation for int16_t
template <>
struct value_serializer<int16_t> : char_conv_serializer<int16_t>
{
	static constexpr std::string type_name() { return "xsd:short"; }
};

/// @ref value_serializer implementation for uint16_t
template <>
struct value_serializer<uint16_t> : char_conv_serializer<uint16_t>
{
	static constexpr std::string type_name() { return "xsd:unsignedShort"; }
};

/// @ref value_serializer implementation for int32_t
template <>
struct value_serializer<int32_t> : char_conv_serializer<int32_t>
{
	static constexpr std::string type_name() { return "xsd:int"; }
};

/// @ref value_serializer implementation for uint32_t
template <>
struct value_serializer<uint32_t> : char_conv_serializer<uint32_t>
{
	static constexpr std::string type_name() { return "xsd:unsignedInt"; }
};

/// @ref value_serializer implementation for int64_t
template <>
struct value_serializer<int64_t> : char_conv_serializer<int64_t>
{
	static constexpr std::string type_name() { return "xsd:long"; }
};

/// @ref value_serializer implementation for uint64_t
template <>
struct value_serializer<uint64_t> : char_conv_serializer<uint64_t>
{
	static constexpr std::string type_name() { return "xsd:unsignedLong"; }
};

/// @ref value_serializer implementation for float
template <>
struct value_serializer<float> : char_conv_serializer<float>
{
	static constexpr std::string type_name() { return "xsd:float"; }
};

/// @ref value_serializer implementation for double
template <>
struct value_serializer<double> : char_conv_serializer<double>
{
	static constexpr std::string type_name() { return "xsd:double"; }
};

/// \brief value_serializer for enum values
///
/// This class is used to (de-)serialize enum values. To map enum
/// values to a string you should use the singleton instance
/// accessible through instance() and then call the operator()
/// members assinging each of the enum values with their respective
/// string.
///
/// A recent addition is the init() call to initialize the instance

template <typename T>
	requires std::is_enum_v<T>
struct value_serializer<T>
{
	std::string m_type_name;

	using value_map_type = std::map<T, std::string>;
	using value_map_value_type = typename value_map_type::value_type;

	value_map_type m_value_map;

	/// \brief Initialize a new instance of value_serializer for this enum, with name and a set of name/value pairs
	static void init(std::string_view name, std::initializer_list<value_map_value_type> values)
	{
		instance(name).m_value_map = value_map_type(values);
	}

	/// \brief Initialize a new anonymous instance of value_serializer for this enum with a set of name/value pairs
	static void init(std::initializer_list<value_map_value_type> values)
	{
		instance().m_value_map = value_map_type(values);
	}

	static value_serializer &instance(std::string_view name = {})
	{
		static value_serializer s_instance;
		if (not name.empty() and s_instance.m_type_name.empty())
			s_instance.m_type_name = name;
		return s_instance;
	}

	value_serializer &operator()(T v, std::string_view name)
	{
		m_value_map[v] = name;
		return *this;
	}

	value_serializer &operator()(std::string_view name, T v)
	{
		m_value_map[v] = name;
		return *this;
	}

	static std::string type_name()
	{
		return instance().m_type_name;
	}

	static std::string to_string(T value)
	{
		return instance().m_value_map[value];
	}

	static T from_string(std::string_view value)
	{
		T result = {};
		for (auto &t : instance().m_value_map)
			if (t.second == value)
			{
				result = t.first;
				break;
			}
		return result;
	}

	static bool empty()
	{
		return instance().m_value_map.empty();
	}
};

// --------------------------------------------------------------------
// date/time support
// We're using Howard Hinands date functions here. If available...

#if __has_include(<date/date.h>) and 0

/// \brief to_string/from_string for std::chrono::system_clock::time_point
/// time is always assumed to be UTC
/// For a specification, see https://www.iso20022.org/standardsrepository/type/ISODateTime

template <>
struct value_serializer<std::chrono::system_clock::time_point>
{
	using time_type = std::chrono::system_clock::time_point;

	static constexpr std::string type_name() { return "xsd:dateTime"; }

	/// to_string the time as YYYY-MM-DDThh:mm:ssZ (zero UTC offset)
	static std::string to_string(const time_type &v)
	{
		return date::format("%FT%TZ", v);
	}

	/// from_string according to ISO8601 rules.
	/// If Zulu time is specified, then the parsed xsd:dateTime is returned.
	/// If an UTC offset is present, then the offset is subtracted from the xsd:dateTime, this yields UTC.
	/// If no UTC offset is present, then the xsd:dateTime is assumed to be local time and converted to UTC.
	static time_type from_string(const std::string &s)
	{
		time_type result;

		std::regex kRX(R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}(?::\d{2}(?:\.\d+)?)?(Z|[-+]\d{2}:\d{2})?)");
		std::smatch m;

		if (not std::regex_match(s, m, kRX))
			throw std::runtime_error("Invalid date format");

		std::istringstream is(s);

		if (m[1].matched)
		{
			if (m[1] == "Z")
				date::from_stream(is, "%FT%TZ", result);
			else
				date::from_stream(is, "%FT%T%0z", result);
		}
		else
			date::from_stream(is, "%FT%T", result);

		if (is.bad() or is.fail())
			throw std::runtime_error("invalid formatted date");

		return result;
	}
};

/// \brief to_string/from_string for date::sys_days
/// For a specification, see https://www.iso20022.org/standardsrepository/type/ISODateTime

template <>
struct value_serializer<date::sys_days>
{
	static constexpr std::string type_name() { return "xsd:date"; }

	/// to_string the date as YYYY-MM-DD
	static std::string to_string(const date::sys_days &v)
	{
		std::ostringstream ss;
		date::to_stream(ss, "%F", v);
		return ss.str();
	}

	/// from_string according to ISO8601 rules.
	static date::sys_days from_string(const std::string &s)
	{
		date::sys_days result;

		std::istringstream is(s);
		date::from_stream(is, "%F", result);

		if (is.bad() or is.fail())
			throw std::runtime_error("invalid formatted date");

		return result;
	}
};

#endif

// --------------------------------------------------------------------

export template <typename T>
using serialize_value_t = decltype(std::declval<value_serializer<T> &>().from_string(std::declval<const std::string &>()));

export template <typename T, typename Archive>
using serialize_function = decltype(std::declval<T &>().serialize(std::declval<Archive &>(), std::declval<unsigned long>()));

export template <typename T, typename Archive, typename = void>
struct has_serialize : std::false_type
{
};

export template <typename T, typename Archive>
struct has_serialize<T, Archive, typename std::enable_if_t<std::is_class_v<T>>>
{
	static constexpr bool value = std::experimental::is_detected_v<serialize_function, T, Archive>;
};

export template <typename T, typename S>
inline constexpr bool has_serialize_v = has_serialize<T, S>::value;

export template <typename T, typename S, typename = void>
struct is_serializable_array_type : std::false_type
{
};

export template <typename T>
using value_type_t = typename T::value_type;

export template <typename T>
using iterator_t = typename T::iterator;

export template <typename T>
using std_string_npos_t = decltype(T::npos);

/// Struct used to detect whether type \a T is serializable
export template <typename T, typename S>
struct is_serializable_type
{
	using value_type = std::remove_const_t<typename std::remove_reference_t<T>>;
	static constexpr bool value =
		std::experimental::is_detected_v<serialize_value_t, value_type> or
		has_serialize_v<value_type, S>;
};

export template <typename T, typename S>
inline constexpr bool is_serializable_type_v = is_serializable_type<T, S>::value;

export template <typename T, typename S>
struct is_serializable_array_type<T, S,
	std::enable_if_t<
		std::experimental::is_detected_v<value_type_t, T> and
		std::experimental::is_detected_v<iterator_t, T> and
		not std::experimental::is_detected_v<std_string_npos_t, T>>>
{
	static constexpr bool value = is_serializable_type_v<typename T::value_type, S>;
};

export template <typename T, typename S>
inline constexpr bool is_serializable_array_type_v = is_serializable_array_type<T, S>::value;

// --------------------------------------------------------------------

export struct serializer;
export struct deserializer;

template <typename T>
class name_value_pair
{
  public:
	name_value_pair(std::string_view name, T &value)
		: m_name(name)
		, m_value(value)
	{
	}

	name_value_pair(const name_value_pair &) = default;
	name_value_pair(name_value_pair &&) = default;
	name_value_pair &operator=(const name_value_pair &) = default;
	name_value_pair &operator=(name_value_pair &&) = default;

	const std::string &name() const { return m_name; }

	// T &value() { return m_value; }
	T &value() const { return m_value; }

  private:
	std::string m_name;
	T &m_value;
};

template <typename T> class element_nvp : public name_value_pair<T>
{
  public:
	element_nvp(std::string_view name, T &value)
		: name_value_pair<T>(name, value)
	{
	}
};
template <typename T> class attribute_nvp : public name_value_pair<T>
{
  public:
	attribute_nvp(std::string_view name, T &value)
		: name_value_pair<T>(name, value)
	{
	}
};

export template <typename T>
constexpr attribute_nvp<T> make_attribute_nvp(std::string_view name, T &value)
{
	return attribute_nvp(name, value);
}

export template <typename T>
constexpr element_nvp<T> make_element_nvp(std::string_view name, T &value)
{
	return element_nvp(name, value);
}

/// serializer, deserializer and schema_creator are classes that can be used
/// to initiate the serialization. They are the Archive classes that are
/// the first parameter to the templated function 'serialize' in the classes
/// that can be serialized. (See boost::serialization for more info).

/// serializer is the class that initiates the serialization process.

export struct serializer
{
	serializer(element_container &node)
		: m_node(node)
	{
	}

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

	element_container &m_node;
};

/// deserializer is the class that initiates the deserialization process.

export struct deserializer
{
	deserializer(const element_container &node)
		: m_node(node)
	{
	}

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

	const element_container &m_node;
};

export using type_map = std::map<std::string, element>;

// --------------------------------------------------------------------
/// schema_creator is used by zeep::dispatcher to create schema files.

struct schema_creator
{
	schema_creator(type_map &types, element_container &node)
		: m_node(node)
		, m_types(types)
	{
	}

	template <typename T>
	schema_creator &operator&(const name_value_pair<T> &rhs)
	{
		return rhs.to_attribute ? add_attribute(rhs.name(), rhs.value()) : add_element(rhs.name(), rhs.value());
	}

	template <typename T>
	schema_creator &add_element(std::string_view name, const T &value);

	template <typename T>
	schema_creator &add_attribute(std::string_view name, const T &value);

	element_container &m_node;
	type_map &m_types;
	std::string m_prefix = "ns";
};

// --------------------------------------------------------------------

template <typename T>
struct type_serializer;

template <typename T, size_t N>
struct type_serializer<T[N]>
{
	using value_type = std::remove_cvref_t<T>;
	using type_serializer_type = type_serializer<value_type>;

	static constexpr std::string type_name() { return type_serializer_type::type_name(); }

	static void serialize_child(element_container &n, std::string_view name, const value_type (&value)[N])
	{
		for (const value_type &v : value)
			type_serializer_type::serialize_child(n, name, v);
	}

	static void deserialize_child(const element_container &n, std::string_view name, value_type (&value)[N])
	{
		size_t ix = 0;
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

	static element schema(std::string_view name, const std::string &prefix)
	{
		element result = type_serializer_type::schema(name, prefix);
		result.set_attribute("minOccurs", std::to_string(N));
		result.set_attribute("maxOccurs", std::to_string(N));
		return result;
	}

	static void register_type(type_map &types)
	{
		type_serializer_type::register_type(types);
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

	static element schema(const std::string &name, const std::string &prefix)
	{
		return {
			"xsd:element",
			{ { "name", name },
				{ "type", prefix + ':' + type_name() },
				{ "minOccurs", "1" },
				{ "maxOccurs", "1" } }
		};
	}

	static void register_type(type_map &types)
	{
		element n("xsd:simpleType",
			{ { "name", type_name() } });

		element restriction("xsd:restriction", { { "base", "xsd:string" } });

		for (auto &e : value_serializer_type::instance().m_name_mapping)
		{
			restriction.emplace_back(
				"xsd:enumeration",
				{ { "value", e.second } });
		}

		n.emplace_back(std::move(restriction));
		types.emplace(type_name(), std::move(n));
	}
};

template <typename T>
	requires has_serialize_v<T, serializer>
struct type_serializer<T>
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;

	// the name of this type
	std::string m_type_name;

	static std::string type_name() { return instance().m_type_name.c_str(); }
	void type_name(std::string_view name) { m_type_name = name; }

	static type_serializer &instance()
	{
		static type_serializer s_instance{ typeid(value_type).name() };
		return s_instance;
	}

	static void serialize_child(element_container &n, std::string_view name, const value_type &value)
	{
		if (name.empty() or name == ".")
		{
			serializer sr(n);
			const_cast<value_type &>(value).serialize(sr, 0Ul);
		}
		else
		{
			element *e = n.emplace_back(name);
			serializer sr(*e);
			const_cast<value_type &>(value).serialize(sr, 0Ul);
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

	static element schema(const std::string &name, const std::string &prefix)
	{
		return {
			"xsd:element",
			{ { "name", name },
				{ "type", prefix + ':' + type_name() },
				{ "minOccurs", "1" },
				{ "maxOccurs", "1" } }
		};
	}

	static void register_type(type_map &types)
	{
		element el("xsd:element", { { "name", type_name() } });
		element *type = el.emplace_back("xsd:complexType");
		element *seq = type->emplace_back("xsd:sequence");

		schema_creator schema(types, *seq);

		value_type v;
		v.serialize(schema, 0);

		types.emplace(type_name(), std::move(el));
	}
};

template <typename T>
struct type_serializer<std::optional<T>>
{
	using value_type = T;
	using container_type = std::optional<value_type>;
	using type_serializer_type = type_serializer<value_type>;

	static constexpr std::string type_name() { return type_serializer_type::type_name(); }

	static void serialize_child(element_container &n, std::string_view name, const container_type &value)
	{
		if (value)
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

	static element schema(const std::string &name, const std::string &prefix)
	{
		return {
			"xsd:element",
			{ { "name", name },
				{ "type", prefix + ':' + type_name() },
				{ "minOccurs", "0" },
				{ "maxOccurs", "1" } }
		};
	}

	static void register_type(type_map &types)
	{
		type_serializer_type::register_type(types);
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
	using container_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using value_type = value_type_t<container_type>;
	using type_serializer_type = type_serializer<value_type>;

	static constexpr std::string type_name() { return type_serializer_type::type_name(); }

	static void serialize_child(element_container &n, std::string_view name, const container_type &value)
	{
		for (const value_type &v : value)
			type_serializer_type::serialize_child(n, name, v);
	}

	template <size_t N>
	static auto deserialize_array(const element_container &n, std::string_view name,
		std::array<value_type, N> &value, priority_tag<2>)
	{
		size_t ix = 0;
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
	static auto deserialize_array(const element_container &n, std::string_view name, A &arr, priority_tag<1>)
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

	static void deserialize_array(const element_container &n, std::string_view name, container_type &arr, priority_tag<0>)
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

	static element schema(const std::string &name, std::string_view /*prefix*/)
	{
		std::string type = type_name();

		return {
			"xsd:element",
			{ { "name", name },
				{ "type", type },
				{ "minOccurs", "0" },
				{ "maxOccurs", "unbounded" } }
		};
	}

	static void register_type(type_map &types)
	{
		type_serializer_type::register_type(types);
	}
};

template <typename T>
struct type_serializer
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using value_serializer_type = value_serializer<value_type>;

	static constexpr std::string type_name() { return value_serializer_type::type_name(); }

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

	static element schema(const std::string &name, std::string_view /*prefix*/)
	{
		std::string type = type_name();

		return {
			"xsd:element",
			{ { "name", name },
				{ "type", type },
				{ "minOccurs", "1" },
				{ "maxOccurs", "1" } }
		};
	}

	static void register_type(type_map & /*types*/)
	{
	}
};

// And finally, the implementation of serializer, deserializer and schema_creator.

template <typename T>
serializer &serializer::serialize_element(const T &value)
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using type_serializer = type_serializer<value_type>;

	type_serializer::serialize_child(m_node, "", value);

	return *this;
}

template <typename T>
serializer &serializer::serialize_element(std::string_view name, const T &value)
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using type_serializer = type_serializer<value_type>;

	type_serializer::serialize_child(m_node, name, value);

	return *this;
}

template <typename T>
serializer &serializer::serialize_attribute(std::string_view name, const T &value)
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using type_serializer = type_serializer<value_type>;

	if (m_node.type() == node_type::element)
		static_cast<element &>(m_node).attributes().emplace(name, type_serializer::serialize_value(value));

	return *this;
}

template <typename T>
deserializer &deserializer::deserialize_element(T &value)
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using type_serializer = type_serializer<value_type>;

	type_serializer::deserialize_child(m_node, "", value);

	return *this;
}

template <typename T>
deserializer &deserializer::deserialize_element(std::string_view name, T &value)
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using type_serializer = type_serializer<value_type>;

	type_serializer::deserialize_child(m_node, name, value);

	return *this;
}

template <typename T>
deserializer &deserializer::deserialize_attribute(std::string_view name, T &value)
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using type_serializer = type_serializer<value_type>;

	if (m_node.type() == node_type::element)
	{
		std::string attr = static_cast<const element &>(m_node).get_attribute(name);
		if (not attr.empty())
			value = type_serializer::deserialize_value(attr);
	}
	return *this;
}

template <typename T>
schema_creator &schema_creator::add_element(std::string_view name, const T & /*value*/)
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using type_serializer = type_serializer<value_type>;

	m_node.emplace_back(type_serializer::schema(name, m_prefix));

	std::string type_name = type_serializer::type_name();

	// might be known already
	if (m_types.find(type_name) == m_types.end())
		type_serializer::register_type(m_types);

	return *this;
}

template <typename T>
schema_creator &schema_creator::add_attribute(std::string_view name, const T & /*value*/)
{
	using value_type = typename std::remove_const_t<typename std::remove_reference_t<T>>;
	using type_serializer = type_serializer<value_type>;

	std::string type_name = type_serializer::type_name();

	m_node.parent()->emplace_back(
		element("xsd:attribute",
			{ { "name", name },
				{ "type", type_name } }));

	if (m_types.find(type_name) == m_types.end())
		type_serializer::register_type(m_types);

	return *this;
}

} // namespace mxml
