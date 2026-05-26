/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 Maarten L. Hekkelman
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

/* code */

#define CATCH_CONFIG_RUNNER

#include "zeem.hpp"

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
	from_xml(doc, "test", i);

	CHECK(i == 42);

	document doc2;
	to_xml(doc2, "test", i);

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
	from_xml(doc, "test", s);

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
	from_xml(doc, "s1", s2);

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
	from_xml(e, "i", ii2);

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
	from_xml(doc, "test", sa2);

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

	from_xml(doc, "test", s);

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
	from_xml(doc, t1);

	CHECK(t1.sd == 2022y / 12 / 6);
}

TEST_CASE("test_date_2")
{
	using namespace zeem::literals;
	using namespace std::chrono;
	using namespace std::chrono_literals;

	date_t1 t1{ 1966y / 6 / 27 };

	zeem::document doc;
	to_xml(doc, t1);

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
		from_xml(doc, t1);

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
	to_xml(doc, t1);

	auto ti = doc.find_first("//t");
	REQUIRE(ti != doc.end());

	auto ti_c = ti->get_content();

	std::regex rx(R"(^2022-12-06T01:02:03(\.0+)?Z$)");

	CHECK(std::regex_match(ti_c, rx));

	time_t1 t2;
	from_xml(doc, t2);

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
	from_xml(doc, t1);

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
	from_xml(doc, t1);

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
	// CHECK_THROWS_AS(zeem::from_xml(doc, "v1", v2), zeem::exception);

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