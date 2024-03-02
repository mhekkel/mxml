#define CATCH_CONFIG_RUNNER

#if CATCH22
# include <catch2/catch.hpp>
#else
# include <catch2/catch_all.hpp>
#endif

#include <array>
#include <exception>
#include <deque>
#include <filesystem>
#include <iostream>
#include <system_error>

// import mxml;
#include "mxml.ixx"

std::filesystem::path gTestDir = std::filesystem::current_path();

int main(int argc, char *argv[])
{
	Catch::Session session; // There must be exactly one instance

	// Build a new parser on top of Catch2's
#if CATCH22
	using namespace Catch::clara;
#else
	// Build a new parser on top of Catch2's
	using namespace Catch::Clara;
#endif

	auto cli = session.cli()                                // Get Catch2's command line parser
	           | Opt(gTestDir, "data-dir")                  // bind variable to a new option, with a hint string
	                 ["-D"]["--data-dir"]                   // the option names it will respond to
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
	int i;
	std::string s;

	template <class Archive>
	void serialize(Archive &ar, unsigned long /*v*/)
	{
		// clang-format off
		ar & mxml::make_element_nvp("i", i)
		   & mxml::make_element_nvp("s", s);
		// clang-format on
	}

	bool operator==(const st_1 &rhs) const { return i == rhs.i and s == rhs.s; }
};

typedef std::vector<st_1> v_st_1;

TEST_CASE("serializer_1")
{
	using namespace mxml;
	using namespace mxml::literals;

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
	int8_t a;
	float b;
	std::string c;

	bool operator==(const S &s) const { return a == s.a and b == s.b and c == s.c; }

	template <typename Archive>
	void serialize(Archive &ar, unsigned long /*version*/)
	{
		// clang-format off
		ar & mxml::make_element_nvp("a", a)
		   & mxml::make_element_nvp("b", b)
		   & mxml::make_element_nvp("c", c);
		// clang-format on
	}
};

TEST_CASE("serializer_2")
{
	using namespace mxml;
	using namespace mxml::literals;

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
	using namespace mxml;

	st_1 s1{ 1, "aap" };

	document doc;
	doc.serialize("s1", s1);
	CHECK((std::ostringstream() << doc).str() == "<s1><i>1</i><s>aap</s></s1>");

	doc.clear();
	to_xml(doc, "s1", s1);

	CHECK((std::ostringstream() << doc).str() == "<s1><i>1</i><s>aap</s></s1>");

	st_1 s2;
	doc.deserialize("s1", s2);

	CHECK(s1 == s2);
}

struct S_arr
{
	std::vector<int> vi;
	std::deque<S> ds;

	template <typename Archive>
	void serialize(Archive &ar, unsigned long)
	{
		// clang-format off
		ar & mxml::make_element_nvp("vi", vi)
		   & mxml::make_element_nvp("ds", ds);
		// clang-format on
	}
};

TEST_CASE("test_serialize_arrays")
{
	using namespace mxml;

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
	using namespace mxml;

	S_arr sa{
		{ 1, 2, 3, 4 },
		{ { 1, 0.5f, "aap" },
			{ 2, 1.5f, "noot" } }
	};

	document doc;
	doc.serialize("test", sa);

	S_arr sa2;
	doc.deserialize("test", sa2);

	CHECK(sa.vi == sa2.vi);
	CHECK(sa.ds == sa2.ds);
}

TEST_CASE("serialize_arrays_2")
{
	using namespace mxml;
	using namespace mxml::literals;

	element e("test");

	int i[] = { 1, 2, 3 };

	serializer sr(e);
	sr.serialize_element("i", i);

	CHECK((std::ostringstream() << e).str() == R"(<test><i>1</i><i>2</i><i>3</i></test>)");
}

TEST_CASE("serialize_container_1")
{
	using namespace mxml;
	using namespace mxml::literals;

	element e("test");

	std::array<int, 3> i = { 1, 2, 3 };

	serializer sr(e);
	sr.serialize_element("i", i);

	std::array<int, 3> j;
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
	void serialize(Archive &ar, unsigned long)
	{
		ar &mxml::make_element_nvp("e", m_e);
	}
};

TEST_CASE("test_s_2")
{
	using namespace mxml;
	value_serializer<E>::instance("my-enum")(E::aap, "aap")(E::noot, "noot")(E::mies, "mies");

	std::vector<E> e = { E::aap, E::noot, E::mies };

	document doc;
	// cannot create more than one root element in a doc:
	CHECK_THROWS_AS(doc.serialize("test", e), mxml::exception);

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
	doc2.serialize("s", se);

	CHECK((std::ostringstream() << doc2).str() == "<s><e>aap</e></s>");
}

TEST_CASE("test_s_3")
{
	using namespace mxml;
	value_serializer<int8_t> s8;

	CHECK(s8.type_name() == "xsd:byte");

	CHECK(s8.from_string("1") == 1);
	CHECK_THROWS_AS(s8.from_string("128"), std::system_error);
	CHECK_THROWS_AS(s8.from_string("x"), std::system_error);
}

TEST_CASE("test_s_4")
{
	using namespace mxml;
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
	using namespace mxml;
	using namespace mxml::literals;

	std::optional<std::string> s;

	document doc;
	// doc.serialize("test", s);

	// CHECK(doc == "<test/>"_xml);

	s.emplace("aap");
	doc.clear();
	doc.serialize("test", s);

	CHECK(doc == "<test>aap</test>"_xml);

	s.reset();

	doc.deserialize("test", s);

	CHECK((bool)s);
	CHECK(*s == "aap");
}

TEST_CASE("test_schema")
{
	using namespace mxml;
	using namespace mxml::literals;

	element schema;
	type_map types;

	// schema_creator
}

// struct date_t1
// {
// 	date::sys_days sd;

// 	template <typename Archive>
// 	void serialize(Archive &ar, unsigned long)
// 	{
// 		ar &make_nvp("d", sd);
// 	}
// };

// TEST_CASE("test_date_1")
// {
// 	using namespace mxml::literals;
// 	using namespace date;

// 	auto doc = "<d>2022-12-06</d>"_xml;

// 	mxml::deserializer ds(*doc.root());

// 	date_t1 t1;
// 	ds.deserialize_element(t1);

// 	CHECK(t1.sd == 2022_y / 12 / 6);
// }

// TEST_CASE("test_date_2")
// {
// 	using namespace mxml::literals;
// 	using namespace date;

// 	date_t1 t1{ 1966_y / 6 / 27 };

// 	mxml::document doc;
// 	mxml::serializer s(doc);

// 	s.serialize_element(t1);

// 	CHECK(doc == "<d>1966-06-27</d>"_xml);
// 	if (not(doc == "<d>1966-06-27</d>"_xml))
// 		std::cout << std::setw(2) << doc << '\n';
// }

// struct time_t1
// {
// 	std::chrono::system_clock::time_point st;

// 	template <typename Archive>
// 	void serialize(Archive &ar, unsigned long)
// 	{
// 		ar &make_nvp("t", st);
// 	}
// };

// TEST_CASE("test_time_1")
// {
// 	using namespace mxml::literals;
// 	using namespace date;

// 	auto doc = "<t>2022-12-06T00:01:02.34Z</t>"_xml;

// 	mxml::deserializer ds(*doc.root());

// 	time_t1 t1;
// 	ds.deserialize_element(t1);

// 	CHECK(t1.st == sys_days{ 2022_y / 12 / 6 } + 0h + 1min + 2.34s);
// }

// TEST_CASE("test_time_2")
// {
// 	using namespace mxml::literals;
// 	using namespace date;

// 	time_t1 t1{ sys_days{ 2022_y / 12 / 6 } + 1h + 2min + 3s };

// 	mxml::document doc;
// 	mxml::serializer s(doc);

// 	s.serialize_element(t1);

// 	auto ti = doc.find_first("//t");
// 	BOOST_ASSERT(ti != doc.end());

// 	auto ti_c = ti->get_content();

// 	std::regex rx(R"(^2022-12-06T01:02:03(\.0+)?Z$)");

// 	CHECK(std::regex_match(ti_c, rx));
// }

// TEST_CASE("test_s_2")
// {
// 	st_1 s1 = { 1, "aap" };

// 	v_st_1 v1;
// 	v1.push_back(s1);
// 	v1.push_back(s1);

// 	xml::document doc;
// 	CHECK_THROWS_AS(doc.serialize("v1", v1), exception);
// }

// TEST_CASE("test_s_3")
// {
// 	st_1 st[] = { { 1, "aap" }, { 2, "noot" } };

// 	v_st_1 v1;
// 	v1.push_back(st[0]);
// 	v1.push_back(st[1]);

// 	xml::document doc("<v1/>");
// 	xml::serializer sr(doc.front());
// 	sr.serialize_element("s1", v1);

// 	stringstream s;
// 	s << doc;

// 	cout << s.str() << endl;

// 	CHECK(s.str(), "<v1><s1><i>1</i><s>aap</s></s1><s1><i>2</i><s>noot</s></s1></v1>");

// 	v_st_1 v2;
// 	CHECK_THROWS_AS(doc.deserialize("v1", v2), exception);

// 	xml::deserializer dr(doc.front());
// 	dr.deserialize_element("s1", v2);

// 	CHECK(v1 == v2);
// }

// struct st_2
// {
// 	vector<string>	s;

// 	template<class Archive>
// 	void serialize(Archive& ar, unsigned long v)
// 	{
// 		ar & mxml::make_element_nvp("i", s);
// 	}
// };

// TEST_CASE("test_s_4")
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
