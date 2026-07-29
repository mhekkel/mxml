// Copyright (c) 2026 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

#define CATCH_CONFIG_RUNNER

#include <cassert>
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <string>

#if ZEEM_CXX_MODULE
import zeem;
#else
# include "zeem/zeem.hpp"
#endif

// #include "zeem.ixx"

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

enum class E
{
	one,
	two,
	three
};

struct S
{
	constexpr static std::string type_name() { return "S"; }

	int a;
	float b;
	double c;
	bool d;
	std::string e;
	std::optional<int> f;
	std::vector<double> g;
	E h;
	std::array<short, 4> i;

	template <typename Archive>
	void serialize(Archive &ar, unsigned long)
	{
		// clang-format off
		ar & zeem::make_element_nvp("a", a)
		   & zeem::make_element_nvp("b", b)
		   & zeem::make_element_nvp("c", c)
		   & zeem::make_element_nvp("d", d)
		   & zeem::make_element_nvp("e", e)
		   & zeem::make_element_nvp("f", f)
		   & zeem::make_element_nvp("g", g)
		   & zeem::make_element_nvp("h", h)
		   & zeem::make_element_nvp("i", i);
		// clang-format on
	}
};

TEST_CASE("schema-1")
{
	zeem::value_serializer<E>::init("E", { { E::one, "one" },
											 { E::two, "two" },
											 { E::three, "three" } });

	using namespace zeem::literals;

	zeem::schema_creator sc;

	SECTION("element declarations")
	{
		sc.add_element("one", int{});

		float f[2];
		sc.add_element("two", f);

		std::vector<std::string> s;
		sc.add_element("three", s);

		auto test = R"(<xsd:schema xmlns:xsd="http://www.w3.org/2001/XMLSchema">
			<xsd:element name="one" type="xsd:int" minOccurs="1" maxOccurs="1"/>
			<xsd:element name="two" type="xsd:float" minOccurs="2" maxOccurs="2"/>
			<xsd:element name="three" type="xsd:string" minOccurs="0" maxOccurs="unbounded"/>
		</xsd:schema>)"_xml;

		auto xsd = sc.schema("doc");
		std::cout << std::setw(2) << xsd << '\n'; 

		CHECK(xsd == test);
	}

	SECTION("enum simple type")
	{
		E e;
		sc.add_element("one", e);

		auto test = R"(
		<xsd:schema xmlns:xsd="http://www.w3.org/2001/XMLSchema">
			<xsd:element name="one" type="E" minOccurs="1" maxOccurs="1"/>
			<xsd:simpleType name="E">
				<xsd:restriction base="xsd:string">
					<xsd:enumeration value="one"/>
					<xsd:enumeration value="two"/>
					<xsd:enumeration value="three"/>
				</xsd:restriction>
			</xsd:simpleType>
		</xsd:schema>)"_xml;

		auto xsd = sc.schema("doc");
		std::cout << std::setw(2) << xsd << '\n'; 

		CHECK(xsd == test);
	}

	SECTION("struct complex type")
	{
		S s;

		sc.add_element("s", s);

		auto test = R"(
		<xsd:schema xmlns:xsd="http://www.w3.org/2001/XMLSchema">
			<xsd:element name="s" type="S" minOccurs="1" maxOccurs="1"/>
			<xsd:complexType name="S">
				<xsd:sequence>
					<xsd:element name="a" type="xsd:int" minOccurs="1" maxOccurs="1"/>
					<xsd:element name="b" type="xsd:float" minOccurs="1" maxOccurs="1"/>
					<xsd:element name="c" type="xsd:double" minOccurs="1" maxOccurs="1"/>
					<xsd:element name="d" type="xsd:boolean" minOccurs="1" maxOccurs="1"/>
					<xsd:element name="e" type="xsd:string" minOccurs="1" maxOccurs="1"/>
					<xsd:element name="f" type="xsd:int" minOccurs="0" maxOccurs="1"/>
					<xsd:element name="g" type="xsd:double" minOccurs="0" maxOccurs="unbounded"/>
					<xsd:element name="h" type="E" minOccurs="1" maxOccurs="1"/>
					<xsd:element name="i" type="xsd:short" minOccurs="4" maxOccurs="4"/>
				</xsd:sequence>
			</xsd:complexType>
		</xsd:schema>)"_xml;

		auto xsd = sc.schema("doc");
		std::cout << std::setw(2) << xsd << '\n'; 

		CHECK(xsd == test);
	}
}