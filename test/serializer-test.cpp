// Copyright (c) 2026 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

/* code */

#define CATCH_CONFIG_RUNNER

#if ZEEM_USE_DATE_H
# include <date/tz.h>
#endif

#include <array>
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#if ZEEM_CXX_MODULE
import zeem;
#else
#include "zeem/zeem.hpp"
#endif

std::filesystem::path gTestDir;

int main(int argc, char *argv[])
{
	gTestDir = std::filesystem::current_path();

	Catch::Session session; // There must be exactly one instance

	// Build a new parser on top of Catch2's
	using namespace Catch::Clara;

	auto cli = session.cli();                        // Get Catch2's command line parser
	cli |= Opt(gTestDir, "data-dir")                 // bind variable to a new option, with a hint string
		["-D"]["--data-dir"]                         // the option names it will respond to
		("The directory containing the data files"); // description string for the help output

	// Now pass the new composite back to Catch2 so it uses that
	session.cli(cli);

	// Let Catch2 (using Clara) parse the command line
	int returnCode = session.applyCommandLine(argc, argv);
	if (returnCode != 0) // Indicates a command line error
		return returnCode;

	return session.run();
}

// --------------------------------------------------------------------

struct st_1
{
	int i{};
	std::string s;

	template <class Archive>
	void serialize(Archive &ar, uint64_t /*v*/)
	{
		// clang-format off
		ar & zeem::make_element_nvp("i", i)
		   & zeem::make_element_nvp("s", s);
		// clang-format on
	}

	bool operator==(const st_1 &rhs) const { return i == rhs.i and s == rhs.s; }
};

using v_st_1 = std::vector<st_1>;

TEST_CASE("serializer_1")
{
	using namespace zeem;
	using namespace zeem::literals;

	auto doc = R"(<test>42</test>)"_xml;

	int32_t i = -1;
	zeem::from_xml(doc, "test", i);

	CHECK(i == 42);

	document doc2;
	zeem::to_xml(doc2, "test", i);

	CHECK(doc == doc2);
}

struct S
{
	int8_t a{};
	float b{};
	std::string c;

	bool operator==(const S &s) const { return a == s.a and b == s.b and c == s.c; }

	template <typename Archive>
	void serialize(Archive &ar, uint64_t /*version*/)
	{
		// clang-format off
		ar & zeem::make_element_nvp("a", a)
		   & zeem::make_element_nvp("b", b)
		   & zeem::make_element_nvp("c", c);
		// clang-format on
	}
};

TEST_CASE("serializer_2")
{
	using namespace zeem;
	using namespace zeem::literals;

	auto doc = R"(<test><a>1</a><b>0.2</b><c>aap</c></test>)"_xml;
	S s;
	zeem::from_xml(doc, "test", s);

	CHECK(s.a == 1);
	CHECK(std::to_string(s.b).substr(0, 3) == "0.2");
	CHECK(s.c == "aap");

	document doc2;
	to_xml(doc2, "test", s);

	CHECK(doc == doc2);
}

TEST_CASE("test_s_1")
{
	using namespace zeem;

	st_1 s1{ 1, "aap" };

	document doc;
	to_xml(doc, "s1", s1);
	CHECK((std::ostringstream() << doc).str() == "<s1><i>1</i><s>aap</s></s1>");

	doc.clear();
	to_xml(doc, "s1", s1);

	CHECK((std::ostringstream() << doc).str() == "<s1><i>1</i><s>aap</s></s1>");

	st_1 s2;
	zeem::from_xml(doc, "s1", s2);

	CHECK(s1 == s2);
}

struct S_arr
{
	std::vector<int> vi;
	std::deque<S> ds;

	template <typename Archive>
	void serialize(Archive &ar, [[maybe_unused]] uint64_t version)
	{
		// clang-format off
		ar & zeem::make_element_nvp("vi", vi)
		   & zeem::make_element_nvp("ds", ds);
		// clang-format on
	}
};

TEST_CASE("test_serialize_arrays")
{
	using namespace zeem;

	std::vector<int> ii{ 1, 2, 3, 4 };

	element e("test");
	to_xml(e, "i", ii);

	CHECK((std::ostringstream() << e).str() == "<test><i>1</i><i>2</i><i>3</i><i>4</i></test>");

	document doc;
	doc.insert(doc.begin(), e); // copy

	std::vector<int> ii2;
	zeem::from_xml(e, "i", ii2);

	CHECK(ii == ii2);
}

TEST_CASE("test_serialize_arrays2")
{
	using namespace zeem;

	S_arr sa{
		{ 1, 2, 3, 4 },
		{ { 1, 0.5f, "aap" },
			{ 2, 1.5f, "noot" } }
	};

	document doc;
	to_xml(doc, "test", sa);

	S_arr sa2;
	zeem::from_xml(doc, "test", sa2);

	CHECK(sa.vi == sa2.vi);
	CHECK(sa.ds == sa2.ds);
}

TEST_CASE("serialize_arrays_2")
{
	using namespace zeem;
	using namespace zeem::literals;

	element e("test");

	int i[] = { 1, 2, 3 };

	serializer sr(e);
	sr.serialize_element("i", i);

	CHECK((std::ostringstream() << e).str() == R"(<test><i>1</i><i>2</i><i>3</i></test>)");
}

TEST_CASE("serialize_container_1")
{
	using namespace zeem;
	using namespace zeem::literals;

	element e("test");

	std::array<int, 3> i = { 1, 2, 3 };

	serializer sr(e);
	sr.serialize_element("i", i);

	std::array<int, 3> j{};
	deserializer dsr(e);
	dsr.deserialize_element("i", j);

	CHECK(i == j);

	CHECK((std::ostringstream() << e).str() == R"(<test><i>1</i><i>2</i><i>3</i></test>)");
}

enum class E
{
	aap,
	noot,
	mies
};

struct Se
{
	E m_e;

	template <typename Archive>
	void serialize(Archive &ar, [[maybe_unused]] uint64_t version)
	{
		ar &zeem::make_element_nvp("e", m_e);
	}
};

TEST_CASE("test_s_2")
{
	using namespace zeem;
	value_serializer<E>::instance("my-enum")(E::aap, "aap")(E::noot, "noot")(E::mies, "mies");

	std::vector<E> e = { E::aap, E::noot, E::mies };

	document doc;
	// cannot create more than one root element in a doc:
	CHECK_THROWS_AS(to_xml(doc, "test", e), zeem::exception);

	element test("test");
	serializer sr(test);
	sr.serialize_element("e", e);

	std::vector<E> e2;

	deserializer dsr(test);
	dsr.deserialize_element("e", e2);

	CHECK(e == e2);

	CHECK((std::ostringstream() << test).str() == "<test><e>aap</e><e>noot</e><e>mies</e></test>");

	Se se{ E::aap };

	document doc2;
	to_xml(doc2, "s", se);

	CHECK((std::ostringstream() << doc2).str() == "<s><e>aap</e></s>");
}

TEST_CASE("test_s_3")
{
	using namespace zeem;
	value_serializer<int8_t> s8;

	CHECK(s8.type_name() == "xsd:byte");

	CHECK(s8.from_string("1") == 1);
	CHECK_THROWS_AS(s8.from_string("128"), std::system_error);
	CHECK_THROWS_AS(s8.from_string("x"), std::system_error);
}

TEST_CASE("test_s_4")
{
	using namespace zeem;
	value_serializer<uint8_t> s8;

	CHECK(s8.type_name() == "xsd:unsignedByte");

	CHECK(s8.from_string("1") == 1);
	CHECK(s8.from_string("128") == 128);
	CHECK(s8.from_string("255") == 255);
	CHECK_THROWS_AS(s8.from_string("256"), std::system_error);
	CHECK_THROWS_AS(s8.from_string("x"), std::system_error);
}

TEST_CASE("test_optional")
{
	using namespace zeem;
	using namespace zeem::literals;

	std::optional<std::string> s;

	document doc;
	// doc.serialize("test", s);

	// CHECK(doc == "<test/>"_xml);

	s.emplace("aap");
	doc.clear();
	to_xml(doc, "test", s);

	CHECK(doc == "<test>aap</test>"_xml);

	s.reset();

	zeem::from_xml(doc, "test", s);

	CHECK((bool)s);
	CHECK(s.value_or("") == "aap");
}

struct date_t1
{
	std::chrono::sys_days sd;

	template <typename Archive>
	void serialize(Archive &ar, [[maybe_unused]] uint64_t version)
	{
		ar &zeem::make_element_nvp("d", sd);
	}
};

TEST_CASE("test_date_1")
{
	using namespace zeem::literals;
	using namespace std::chrono;
	using namespace std::chrono_literals;

	auto doc = "<d>2022-12-06</d>"_xml;

	date_t1 t1;
	zeem::from_xml(doc, t1);

	CHECK(t1.sd == 2022y / 12 / 6);
}

TEST_CASE("test_date_2")
{
	using namespace zeem::literals;
	using namespace std::chrono;
	using namespace std::chrono_literals;

	date_t1 t1{ 1966y / 6 / 27 };

	zeem::document doc;
	zeem::to_xml(doc, t1);

	CHECK(doc == "<d>1966-06-27</d>"_xml);
}

struct time_t1
{
	std::chrono::system_clock::time_point st;

	template <typename Archive>
	void serialize(Archive &ar, [[maybe_unused]] uint64_t version)
	{
		ar &zeem::make_element_nvp("t", st);
	}
};

TEST_CASE("test_time_1")
{
	using namespace zeem::literals;
	using namespace std::chrono;
	using namespace std::literals;

	auto t0 = sys_days{ 2022y / 12 / 6 } + 0h + 1min + 2.34s;

	for (auto &doc : {
		"<t>2022-12-06T00:01:02.34Z</t>"_xml,
		"<t>2022-12-06T00:01:02.34+00:00</t>"_xml,
		"<t>2022-12-06T02:01:02.34+02:00</t>"_xml
	})
	{
		time_t1 t1;
		zeem::from_xml(doc, t1);

		if (t1.st == t0)
			continue;

		std::clog << "Error: " << t1.st << '\n';

		CHECK((t1.st == t0) == true);
	}
}

TEST_CASE("test_time_2")
{
	using namespace zeem::literals;
	using namespace std::literals;
	using namespace std::chrono;

	time_t1 t1{ sys_days{ 2022y / 12 / 6 } + 1h + 2min + 3s };

	zeem::document doc;
	zeem::to_xml(doc, t1);

	auto ti = doc.find_first("//t");
	REQUIRE(ti != doc.end());

	auto ti_c = ti->get_content();

	std::regex rx(R"(^2022-12-06T01:02:03(\.0+)?Z$)");

	CHECK(std::regex_match(ti_c, rx));

	time_t1 t2;
	zeem::from_xml(doc, t2);

	CHECK(t2.st == t1.st);
}

TEST_CASE("test_time_3")
{
	using namespace zeem::literals;
	using namespace std::chrono;
	using namespace std::literals;

	// No time zone specification, so this is converted to UTC using the local time_zone as offset
	auto doc = "<t>2022-12-06T00:01:02</t>"_xml;

	time_t1 t1;
	zeem::from_xml(doc, t1);

	auto t2 = sys_days{ 2022y / 12 / 6 } + 1min + 2s;

#if ZEEM_USE_DATE_H
	CHECK((t1.st == t2 - date::current_zone()->get_info(t2).offset) == true);
#else
	CHECK((t1.st == t2 - current_zone()->get_info(t2).offset) == true);
#endif
}

TEST_CASE("test_time_4")
{
	using namespace zeem::literals;
	using namespace std::chrono;
	using namespace std::literals;

	// No time zone specification, so this local time is converted to UTC
	auto doc = "<t>2026-04-24T08:14Z</t>"_xml;

	time_t1 t1;
	zeem::from_xml(doc, t1);

#if ZEEM_USE_DATE_H
	auto t2 = date::zoned_time(date::current_zone(), sys_days{ 2026y / 4 / 24 } + 8h + 14min).get_sys_time();
#else
	auto t2 = zoned_time(current_zone(), sys_days{ 2026y / 4 / 24 } + 8h + 14min).get_sys_time();
#endif

	CHECK((t1.st == t2) == true);
}


TEST_CASE("test_s_5")
{
	st_1 s1 = { 1, "aap" };

	v_st_1 v1;
	v1.push_back(s1);
	v1.push_back(s1);

	zeem::document doc;
	CHECK_THROWS_AS(zeem::to_xml(doc, "v1", v1), zeem::exception);
}

TEST_CASE("test_s_6")
{
	st_1 st[] = { { 1, "aap" }, { 2, "noot" } };

	v_st_1 v1;
	v1.push_back(st[0]);
	v1.push_back(st[1]);

	zeem::document doc("<v1/>");
	zeem::to_xml(doc.front(), "s1", v1);

	CHECK((std::ostringstream() << doc).str() == "<v1><s1><i>1</i><s>aap</s></s1><s1><i>2</i><s>noot</s></s1></v1>");

	v_st_1 v2;
	// CHECK_THROWS_AS(zeem::zeem::from_xml(doc, "v1", v2), zeem::exception);

	zeem::from_xml(doc.front(), "s1", v2);

	CHECK(v1 == v2);
}

struct st_2
{
	std::vector<std::string> s;

	template <class Archive>
	void serialize(Archive &ar, [[maybe_unused]] uint64_t version)
	{
		// clang-format off
		ar & zeem::make_element_nvp("i", s);
		// clang-format on
	}
};

// TEST_CASE("test_s_7")
// {
// 	st_2 s1;
// 	s1.s.push_back("aap");
// 	s1.s.push_back("noot");

// 	xml::document doc;
// 	doc.serialize("st2", s1);

// 	stringstream s;
// 	s << doc;

// 	CHECK(s.str(), "<st2><s>aap</s><s>noot</s></st2>");

// 	st_2 s2;
// 	doc.deserialize("st2", s2);

// 	CHECK(s1.s == s2.s);
// }

// TEST_CASE("type-1")
// {
// 	using namespace zeem;

// 	type_map types;
// 	schema_creator sc(types, )
// }

// --------------------------------------------------------------------
// Negative / error-path tests

TEST_CASE("ser_err_int16")
{
	zeem::value_serializer<int16_t> s;

	CHECK(s.from_string("0") == 0);
	CHECK(s.from_string("-1") == -1);
	CHECK(s.from_string("32767") == 32767);
	CHECK(s.from_string("-32768") == -32768);
	CHECK_THROWS_AS(s.from_string("32768"), std::system_error);
	CHECK_THROWS_AS(s.from_string("-32769"), std::system_error);
	CHECK_THROWS_AS(s.from_string("x"), std::system_error);
}

TEST_CASE("ser_err_uint16")
{
	zeem::value_serializer<uint16_t> s;

	CHECK(s.from_string("0") == 0);
	CHECK(s.from_string("65535") == 65535);
	CHECK_THROWS_AS(s.from_string("65536"), std::system_error);
	CHECK_THROWS_AS(s.from_string("-1"), std::system_error);
	CHECK_THROWS_AS(s.from_string("x"), std::system_error);
}

TEST_CASE("ser_err_int32")
{
	zeem::value_serializer<int32_t> s;

	CHECK(s.from_string("0") == 0);
	CHECK(s.from_string("2147483647") == 2147483647);
	CHECK(s.from_string("-2147483648") == -2147483648);
	CHECK_THROWS_AS(s.from_string("2147483648"), std::system_error);
	CHECK_THROWS_AS(s.from_string("-2147483649"), std::system_error);
	CHECK_THROWS_AS(s.from_string("x"), std::system_error);
}

TEST_CASE("ser_err_uint32")
{
	zeem::value_serializer<uint32_t> s;

	CHECK(s.from_string("0") == 0);
	CHECK(s.from_string("4294967295") == 4294967295);
	CHECK_THROWS_AS(s.from_string("4294967296"), std::system_error);
	CHECK_THROWS_AS(s.from_string("-1"), std::system_error);
	CHECK_THROWS_AS(s.from_string("x"), std::system_error);
}

TEST_CASE("ser_err_int64")
{
	zeem::value_serializer<int64_t> s;

	CHECK(s.from_string("0") == 0);
	CHECK(s.from_string("9223372036854775807") == INT64_MAX);
	CHECK(s.from_string("-9223372036854775808") == INT64_MIN);
	CHECK_THROWS_AS(s.from_string("x"), std::system_error);
}

TEST_CASE("ser_err_uint64")
{
	zeem::value_serializer<uint64_t> s;

	CHECK(s.from_string("0") == 0);
	CHECK(s.from_string("18446744073709551615") == UINT64_MAX);
	CHECK_THROWS_AS(s.from_string("-1"), std::system_error);
	CHECK_THROWS_AS(s.from_string("x"), std::system_error);
}

TEST_CASE("ser_err_float")
{
	zeem::value_serializer<float> s;

	CHECK(s.from_string("0") == 0.0f);
	CHECK(s.from_string("3.14") != 0.0f);
	CHECK(s.from_string("-1.5") < 0.0f);
	CHECK_THROWS_AS(s.from_string("x"), std::system_error);
	CHECK_THROWS_AS(s.from_string("abc"), std::system_error);
}

TEST_CASE("ser_err_double")
{
	zeem::value_serializer<double> s;

	CHECK(s.from_string("0") == 0.0);
	CHECK(s.from_string("3.14159") != 0.0);
	CHECK(s.from_string("-2.5") < 0.0);
	CHECK_THROWS_AS(s.from_string("x"), std::system_error);
	CHECK_THROWS_AS(s.from_string("abc"), std::system_error);
}

TEST_CASE("ser_err_bool")
{
	zeem::value_serializer<bool> s;

	CHECK(s.from_string("true") == true);
	CHECK(s.from_string("false") == false);
	CHECK(s.from_string("1") == true);
	CHECK(s.from_string("0") == false);
	CHECK(s.from_string("yes") == true);

	// "no" is not recognized as false — it returns the default (false)
	CHECK(s.from_string("no") == false);

	// unrecognized strings return the default-initialized value (false)
	CHECK(s.from_string("") == false);
	CHECK(s.from_string("maybe") == false);
	CHECK(s.from_string("TRUE") == false);
	CHECK(s.from_string("FALSE") == false);

	CHECK(s.to_string(true) == "true");
	CHECK(s.to_string(false) == "false");
}

TEST_CASE("ser_err_enum_unknown")
{
	using namespace zeem;

	// register a fresh enum for this test
	enum class color : int { red, green, blue };

	value_serializer<color>::init("color",
		{ { color::red, "red" }, { color::green, "green" }, { color::blue, "blue" } });

	// deserialize a string that doesn't match any registered enum value
	CHECK_THROWS_AS(value_serializer<color>::from_string("nonexistent"), std::invalid_argument);
	CHECK_THROWS_AS(value_serializer<color>::from_string(""), std::invalid_argument);

	// valid values work
	CHECK(value_serializer<color>::from_string("red") == color::red);
	CHECK(value_serializer<color>::from_string("green") == color::green);
	CHECK(value_serializer<color>::from_string("blue") == color::blue);
}

TEST_CASE("ser_err_date_invalid")
{
	using namespace zeem;
	using namespace zeem::literals;

	auto doc = "<d>not-a-date</d>"_xml;

	std::chrono::sys_days sd;
	CHECK_THROWS_AS(
		zeem::from_xml(doc, "d", sd),
		std::runtime_error);

	auto doc2 = "<d></d>"_xml;
	CHECK_THROWS_AS(
		zeem::from_xml(doc2, "d", sd),
		std::runtime_error);
}

TEST_CASE("ser_err_datetime_invalid")
{
	using namespace zeem;
	using namespace zeem::literals;

	std::chrono::system_clock::time_point tp;

	auto doc = "<t>not-a-time</t>"_xml;
	CHECK_THROWS_AS(
		zeem::from_xml(doc, "t", tp),
		std::runtime_error);

	auto doc2 = "<t></t>"_xml;
	CHECK_THROWS_AS(
		zeem::from_xml(doc2, "t", tp),
		std::runtime_error);
}

TEST_CASE("ser_err_deser_missing_element")
{
	using namespace zeem;
	using namespace zeem::literals;

	// from_xml with a name that doesn't exist: value is zero-initialized
	auto doc = R"(<root><other>42</other></root>)"_xml;

	int32_t i = -1;
	zeem::from_xml(*doc.child(), "missing", i);
	CHECK(i == 0); // zero-initialized since element not found

	std::string s = "default";
	zeem::from_xml(*doc.child(), "missing", s);
	CHECK(s.empty()); // empty string since element not found
}

TEST_CASE("ser_err_deser_wrong_type")
{
	using namespace zeem;
	using namespace zeem::literals;

	// text "hello" where int32_t is expected
	auto doc = R"(<val>hello</val>)"_xml;

	int32_t i = 0;
	CHECK_THROWS_AS(zeem::from_xml(doc, "val", i), std::system_error);
}

TEST_CASE("ser_err_deser_empty_element")
{
	using namespace zeem;
	using namespace zeem::literals;

	// empty element where numeric type is expected
	auto doc = R"(<val></val>)"_xml;

	int32_t i = 0;
	CHECK_THROWS_AS(zeem::from_xml(doc, "val", i), std::system_error);
}

TEST_CASE("ser_err_document_two_roots")
{
	zeem::document doc;
	doc.emplace("first");

	// to_xml on document with existing root should throw
	int x = 1;
	CHECK_THROWS_AS(zeem::to_xml(doc, "second", x), zeem::exception);
}

TEST_CASE("ser_err_deser_partial_number")
{
	using namespace zeem;
	using namespace zeem::literals;

	// "42abc" — partial parse should fail since from_chars requires full consumption
	auto doc = R"(<val>42abc</val>)"_xml;

	int32_t i = 0;
	CHECK_THROWS_AS(zeem::from_xml(doc, "val", i), std::system_error);
}

TEST_CASE("ser_err_deser_negative_into_unsigned")
{
	using namespace zeem;
	using namespace zeem::literals;

	auto doc = R"(<val>-1</val>)"_xml;

	uint32_t u = 0;
	CHECK_THROWS_AS(zeem::from_xml(doc, "val", u), std::system_error);
}

TEST_CASE("ser_err_optional_empty")
{
	using namespace zeem;
	using namespace zeem::literals;

	// missing element for optional — should remain empty
	auto doc = R"(<root></root>)"_xml;

	std::optional<int32_t> opt;
	zeem::from_xml(doc, "val", opt);
	CHECK_FALSE(opt.has_value());
}

TEST_CASE("ser_err_deser_float_from_int")
{
	using namespace zeem;
	using namespace zeem::literals;

	auto doc = R"(<val>not-a-number</val>)"_xml;

	float f = 0.0f;
	CHECK_THROWS_AS(zeem::from_xml(doc, "val", f), std::system_error);
}

TEST_CASE("ser_err_deser_attribute_missing")
{
	using namespace zeem;
	using namespace zeem::literals;

	// deserializing a non-existent attribute should leave value at default
	auto doc = R"(<root><child a="1"></child></root>)"_xml;

	auto &child = *doc.child()->begin();
	int32_t val = -1;
	zeem::deserializer dsr(child);
	dsr.deserialize_attribute("missing", val);
	CHECK(val == -1);
}

// TEST_CASE("ser_err_schema_attribute")
// {
// 	using namespace zeem;
// 	using namespace zeem::literals;

// 	// schema_creator with an attribute nvp: verify it doesn't crash
// 	// when used on an element whose parent exists
// 	auto doc = "<root><child></child></root>"_xml;
// 	auto &child = *doc.child()->begin();

// 	zeem::type_map types;
// 	zeem::element seq("xsd:sequence");
// 	child.nodes().emplace_back(std::move(seq));

// 	zeem::type_map types2;
// 	auto &seq3 = static_cast<zeem::element &>(*std::prev(child.nodes().end()));
// 	zeem::schema_creator sc(types2, seq3);

// 	int32_t dummy{};
// 	sc & zeem::make_attribute_nvp("x", dummy);
// }

TEST_CASE("ser_err_roundtrip_int8_overflow")
{
	using namespace zeem;
	using namespace zeem::literals;

	auto doc = R"(<val>128</val>)"_xml;

	int8_t v = 0;
	CHECK_THROWS_AS(zeem::from_xml(doc, "val", v), std::system_error);
}

TEST_CASE("ser_err_roundtrip_uint8_overflow")
{
	using namespace zeem;
	using namespace zeem::literals;

	auto doc = R"(<val>256</val>)"_xml;

	uint8_t v = 0;
	CHECK_THROWS_AS(zeem::from_xml(doc, "val", v), std::system_error);
}

TEST_CASE("ser_err_to_xml_attribute")
{
	using namespace zeem;

	// serialize_attribute on a non-element node should be a no-op
	zeem::document doc;
	doc.emplace("root");

	zeem::serializer sr(doc);
	int32_t x = 42;
	sr.serialize_attribute("a", x);

	// attribute should NOT have been added (document is not an element)
	CHECK(doc.child()->attributes().empty());
}

TEST_CASE("ser_err_deser_attribute_empty")
{
	using namespace zeem;
	using namespace zeem::literals;

	auto doc = "<root><child></child></root>"_xml;
	auto &child = *doc.child()->begin();

	// deserialize_attribute from non-existent attribute — value stays default
	int32_t val = 99;
	zeem::deserializer dsr(child);
	dsr.deserialize_attribute("nonexistent", val);
	CHECK(val == 99);
}

// --------------------------------------------------------------------

enum class Fruit { apple, banana, cherry };
enum class Priority : int { low = -1, medium = 0, high = 1 };

TEST_CASE("value_serializer_enum")
{
	using namespace zeem;

	value_serializer<Fruit>::init("Fruit",
		{ { Fruit::apple, "apple" }, { Fruit::banana, "banana" }, { Fruit::cherry, "cherry" } });

	SECTION("type_name returns registered name")
	{
		CHECK(value_serializer<Fruit>::type_name() == "Fruit");
	}

	SECTION("to_string round-trip")
	{
		CHECK(value_serializer<Fruit>::to_string(Fruit::apple) == "apple");
		CHECK(value_serializer<Fruit>::to_string(Fruit::banana) == "banana");
		CHECK(value_serializer<Fruit>::to_string(Fruit::cherry) == "cherry");
	}

	SECTION("from_string round-trip")
	{
		CHECK(value_serializer<Fruit>::from_string("apple") == Fruit::apple);
		CHECK(value_serializer<Fruit>::from_string("banana") == Fruit::banana);
		CHECK(value_serializer<Fruit>::from_string("cherry") == Fruit::cherry);
	}

	SECTION("to_string throws for unregistered value")
	{
		CHECK_THROWS_AS(value_serializer<Fruit>::to_string(static_cast<Fruit>(99)), std::out_of_range);
	}

	SECTION("from_string throws for invalid string")
	{
		CHECK_THROWS_AS(value_serializer<Fruit>::from_string("orange"), std::invalid_argument);
		CHECK_THROWS_AS(value_serializer<Fruit>::from_string(""), std::invalid_argument);
		CHECK_THROWS_AS(value_serializer<Fruit>::from_string("APPLE"), std::invalid_argument);
	}

	SECTION("values returns all registered strings")
	{
		auto vals = value_serializer<Fruit>::instance().values();
		REQUIRE(vals.size() == 3);
		CHECK(vals[0] == "apple");
		CHECK(vals[1] == "banana");
		CHECK(vals[2] == "cherry");
	}

	SECTION("operator() chaining with (value, name)")
	{
		value_serializer<Priority>::instance()(Priority::low, "low")(Priority::medium, "medium")(Priority::high, "high");

		CHECK(value_serializer<Priority>::to_string(Priority::low) == "low");
		CHECK(value_serializer<Priority>::to_string(Priority::medium) == "medium");
		CHECK(value_serializer<Priority>::to_string(Priority::high) == "high");

		CHECK(value_serializer<Priority>::from_string("low") == Priority::low);
		CHECK(value_serializer<Priority>::from_string("medium") == Priority::medium);
		CHECK(value_serializer<Priority>::from_string("high") == Priority::high);
	}

	SECTION("operator() chaining with (name, value)")
	{
		value_serializer<Priority>::instance("prio")
			("negative", Priority::low)
			("zero", Priority::medium)
			("positive", Priority::high);

		CHECK(value_serializer<Priority>::to_string(Priority::low) == "negative");
		CHECK(value_serializer<Priority>::from_string("positive") == Priority::high);
	}

	SECTION("round-trip via to_xml and from_xml")
	{
		using namespace zeem::literals;

		element e("test");
		to_xml(e, "fruit", Fruit::banana);

		Fruit result = Fruit::apple;
		from_xml(e, "fruit", result);
		CHECK(result == Fruit::banana);

		CHECK((std::ostringstream() << e).str() == "<test><fruit>banana</fruit></test>");
	}

	SECTION("round-trip vector of enums")
	{
		std::vector<Fruit> input = { Fruit::cherry, Fruit::apple, Fruit::banana };

		element e("test");
		serializer sr(e);
		sr.serialize_element("fruits", input);

		std::vector<Fruit> output;
		deserializer dsr(e);
		dsr.deserialize_element("fruits", output);

		CHECK(input == output);
	}
}