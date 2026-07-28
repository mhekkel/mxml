// Copyright (c) 2026 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

#define CATCH_CONFIG_RUNNER

#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <compare>
#include <cstddef>
#include <filesystem>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>

#if ZEEM_CXX_MODULE
import zeem;
#else
#include "zeem/zeem.hpp"
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

TEST_CASE("test-0")
{
	using namespace zeem;

	attribute_set attr(nullptr);
	attr.emplace("a", "1");

	attribute_set a2(nullptr);

	swap(attr, a2);

	REQUIRE(attr.empty());
}

TEST_CASE("test_1")
{
	zeem::element n("test");

	CHECK(n.get_local_name() == "test");

	SECTION("insert")
	{
		auto i1 = n.insert(n.end(), zeem::element("c1"));

		CHECK(i1->get_local_name() == "c1");
		CHECK(i1->empty());
		CHECK(i1->size() == 0);
		CHECK(n.size() == 1);
		CHECK(n.front().get_local_name() == "c1");
		for (auto &e : n)
			CHECK(e.parent() == &n);

		auto i2 = n.insert(n.end(), zeem::element("c2"));

		CHECK(i2->get_local_name() == "c2");
		CHECK(i2->empty());
		CHECK(i2->size() == 0);
		CHECK(n.size() == 2);
		CHECK(n.front().get_local_name() == "c1");
		CHECK(n.back().get_local_name() == "c2");
		for (auto &e : n)
			CHECK(e.parent() == &n);

		auto i3 = n.insert(n.begin(), zeem::element("c0"));
		CHECK(i3->get_local_name() == "c0");
		CHECK(i3->empty());
		CHECK(i3->size() == 0);
		CHECK(n.size() == 3);
		CHECK(n.front().get_local_name() == "c0");
		CHECK(n.back().get_local_name() == "c2");

		zeem::element c3("c3");
		auto i4 = n.insert(n.end(), c3);
		CHECK(i4->get_local_name() == "c3");
		CHECK(i4->empty());
		CHECK(i4->size() == 0);
		CHECK(n.size() == 4);
		CHECK(n.front().get_local_name() == "c0");
		CHECK(n.back().get_local_name() == "c3");

		for (auto &e : n)
		{
			CHECK(e.parent() == &n);
			CHECK(e.empty());
			CHECK(e.size() == 0);
		}

		for (int i = 0; auto &e : n)
		{
			CHECK(e.get_local_name() == "c" + std::to_string(i));
			++i;
		}

		// CHECK(find(n.begin(), n.end(), i1) != n.end());

		auto n2 = n;

		CHECK(n2.size() == 4);
		CHECK(n2.get_local_name() == "test");
		CHECK(n2.front().get_local_name() == "c0");
		CHECK(n2.back().get_local_name() == "c3");
		for (auto &e : n2)
			CHECK(e.parent() == &n2);

		for (int i = 0; auto &e : n2)
		{
			CHECK(e.get_local_name() == "c" + std::to_string(i));
			++i;
		}

		auto n3(std::move(n2));

		CHECK(n2.get_local_name().empty()); // NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved)
		CHECK(n2.empty());
		CHECK(n3.get_local_name() == "test");
		CHECK(n3.size() == 4);
		CHECK(n3.front().get_local_name() == "c0");
		CHECK(n3.back().get_local_name() == "c3");
		for (auto &e : n3)
			CHECK(e.parent() == &n3);

		for (int i = 0; auto &e : n3)
		{
			CHECK(e.get_local_name() == "c" + std::to_string(i));
			++i;
		}

		zeem::element n4;
		n4 = std::move(n3);

		CHECK(n3.empty()); // NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved)
		CHECK(n4.size() == 4);
		CHECK(n4.front().get_local_name() == "c0");
		CHECK(n4.back().get_local_name() == "c3");
		for (auto &e : n4)
			CHECK(e.parent() == &n4);

		for (int i = 0; auto &e : n4)
		{
			CHECK(e.get_local_name() == "c" + std::to_string(i));
			++i;
		}

		// erase

		for (int i = 4; i > 0; --i)
		{
			n4.erase(n4.begin());
			CHECK(n4.size() == i - 1UL);
		}
		CHECK(n4.empty());

		for (int i = 4; i > 0; --i)
		{
			n.erase(std::prev(n.end()));
			CHECK(n.size() == i - 1UL);
		}
		CHECK(n.empty());
	}

	SECTION("emplace")
	{
		auto t = n.emplace(n.end(), "c1");

		CHECK(t->get_local_name() == "c1");
		CHECK(n.size() == 1);
		CHECK(n.front().get_local_name() == "c1");
		for (auto &e : n)
			CHECK(e.parent() == &n);

		auto t2 = n.emplace_back("c2");

		CHECK(t2->get_local_name() == "c2");
		CHECK(n.size() == 2);
		CHECK(n.front().get_local_name() == "c1");
		CHECK(n.back().get_local_name() == "c2");
		for (auto &e : n)
			CHECK(e.parent() == &n);

		auto t3 = n.emplace_front("c0");
		CHECK(t3->get_local_name() == "c0");
		CHECK(n.size() == 3);
		CHECK(n.front().get_local_name() == "c0");
		CHECK(n.back().get_local_name() == "c2");
		for (auto &e : n)
			CHECK(e.parent() == &n);

		std::ostringstream os;
		os << n;

		CHECK(os.str() == "<test><c0/><c1/><c2/></test>");
	}

	// auto &t = n.emplace(n.end(), "c1");

	// CHECK(t.name() == "c1");
	// CHECK(n.size() == 1);
	// CHECK(n.front().name() == "c1");

	// auto &t2 = n.emplace_back("c2");

	// CHECK(t2.name() == "c2");
	// CHECK(n.size() == 2);
	// CHECK(n.front().name() == "c1");
	// CHECK(n.back().name() == "c2");

	// auto &t3 = n.emplace_front("c0");
	// CHECK(t3.name() == "c0");
	// CHECK(n.size() == 3);
	// CHECK(n.front().name() == "c0");
	// CHECK(n.back().name() == "c2");
}

TEST_CASE("attr-1")
{
	using namespace zeem;

	element e("test");
	e.set_attribute("1", "one");
	e.set_attribute("2", "two");
	e.set_attribute("3", "3");
	e.set_attribute("3", "three");

	std::ostringstream os;
	os << e;

	CHECK(os.str() == R"(<test 1="one" 2="two" 3="three"/>)");
}

TEST_CASE("xml_1")
{
	zeem::element n("data", { { "attr1", "value-1" }, { "attr2", "value-2" } });

	CHECK(n.get_local_name() == "data");
	CHECK(n.attributes().empty() == false);
	CHECK(n.attributes().size() == 2);
	CHECK(n.attributes().begin() != n.attributes().end());

	std::size_t i = 0;
	for (auto &[name, value] : n.attributes())
	{
		switch (i++)
		{
			case 0:
				CHECK(name == "attr1");
				CHECK(value == "value-1");
				break;

			case 1:
				CHECK(name == "attr2");
				CHECK(value == "value-2");
				break;
			
			default:;
		}
	}

	std::ostringstream s;
	s << n;

	CHECK(s.str() == R"(<data attr1="value-1" attr2="value-2"/>)");

	std::ostringstream s2;
	s2 << std::setw(2) << std::setiosflags(std::ios_base::left) << n << '\n';

	const char *test = R"(<data attr1="value-1"
      attr2="value-2"/>
)";

	CHECK(s2.str() == test);

	// n.validate();
}

TEST_CASE("xml_2")
{
	zeem::element e("test");
	e.nodes().emplace_back(zeem::comment("commentaar"));
	auto i = e.nodes().begin();
	CHECK(i == e.nodes().begin());
	CHECK(i != e.nodes().end());
	CHECK(i->str() == "commentaar");

	auto j = e.begin();
	CHECK(j == e.begin());
	CHECK(j == e.end());
}

TEST_CASE("xml_3")
{
	zeem::element e("test");

	zeem::element a("aap");

	e.nodes().emplace(e.end(), a);
	CHECK(a.get_local_name() == "aap");
	CHECK((std::ostringstream() << e).str() == R"(<test><aap/></test>)");

	e.nodes().emplace(e.end(), std::move(a));
	CHECK(a.get_local_name() == ""); // NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved)
	CHECK((std::ostringstream() << e).str() == R"(<test><aap/><aap/></test>)");

	zeem::element b("noot");
	// zeem::node &n = b;

	// e.nodes().emplace(e.end(), n);
	CHECK(e.nodes().emplace(e.end(), b)->get_local_name() == "noot");
	CHECK((std::ostringstream() << e).str() == R"(<test><aap/><aap/><noot/></test>)");

	const auto &n2 = b;
	CHECK(e.nodes().emplace(e.end(), n2)->get_local_name() == "noot");
	CHECK((std::ostringstream() << e).str() == R"(<test><aap/><aap/><noot/><noot/></test>)");

	auto &&n3 = std::move(b);
	CHECK(e.nodes().emplace(e.end(), std::move(n3))->get_local_name() == "noot"); // NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved)
	CHECK(b.get_local_name() == ""); // NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved)
	CHECK((std::ostringstream() << e).str() == R"(<test><aap/><aap/><noot/><noot/><noot/></test>)");

	e.attributes().emplace("attr1", "value1");

	std::ostringstream s;
	s << e;
	CHECK(s.str() == R"(<test attr1="value1"><aap/><aap/><noot/><noot/><noot/></test>)");
}

TEST_CASE("xml_attributes_1")
{
	using namespace zeem::literals;

	auto doc = R"(<test xmlns:m="http://www.hekkelman.com">
<t1 m:a="v"/>
</test>)"_xml;

	auto &t = doc.child()->front();

	for (auto &a : t.attributes())
	{
		CHECK(a.get_local_name() == "a");
		CHECK(a.get_qname() == "m:a");
		CHECK(a.get_ns() == "http://www.hekkelman.com");
	}

	for (auto a : t.attributes()) // NOLINT
	{
		CHECK(a.get_local_name() == "a");
		CHECK(a.get_qname() == "m:a");

		// the attribute was copied and thus lost namespace information
		CHECK(a.get_ns() != "http://www.hekkelman.com");
	}
}

TEST_CASE("xml_emplace")
{
	zeem::element e("test");

	e.emplace_back("test2", std::initializer_list<zeem::attribute>{ { "a1", "v1" }, { "a2", "v2" } });

	std::ostringstream s;
	s << e;
	CHECK(s.str() == R"(<test><test2 a1="v1" a2="v2"/></test>)");

	e.emplace_front("test1", std::initializer_list<zeem::attribute>{ { "a1", "v1" }, { "a2", "v2" } });

	std::ostringstream s2;
	s2 << e;
	CHECK(s2.str() == R"(<test><test1 a1="v1" a2="v2"/><test2 a1="v1" a2="v2"/></test>)");
}

TEST_CASE("xml_4")
{
	zeem::element e("test");
	e.emplace_back(zeem::element("test2", { { "attr1", "een" }, { "attr2", "twee" } }));

	std::ostringstream s;
	s << e;
	CHECK(s.str() == R"(<test><test2 attr1="een" attr2="twee"/></test>)");
}

TEST_CASE("xml_5_compare")
{
	zeem::element a("test", { { "a", "v1" }, { "b", "v2" } });
	zeem::element b("test", { { "b", "v2" }, { "a", "v1" } });

	CHECK(a == b);
}

TEST_CASE("xml_container_and_iterators")
{
	zeem::element e("test");

	zeem::element n("a");
	e.insert(e.begin(), std::move(n));
	e.back().set_content("aap ");

	e.emplace_back("b")->set_content("noot ");
	e.emplace_back("c")->set_content("mies");

	CHECK(e.size() == 3);
	CHECK(not e.empty());

	CHECK(e.front().parent() == &e);
	CHECK(e.back().parent() == &e);

	CHECK(e.begin() != e.end());

	CHECK(e.str() == "aap noot mies");

	e.erase(std::next(e.begin()));
	CHECK(e.str() == "aap mies");

	std::ostringstream s1;
	s1 << std::setw(2) << std::left << e << '\n';
	CHECK(s1.str() == R"(<test>
  <a>aap </a>
  <c>mies</c>
</test>
)");

	// e.validate();

	std::ostringstream s2;
	s2 << e;
	CHECK(s2.str() == R"(<test><a>aap </a><c>mies</c></test>)");

	e.pop_front();
	CHECK(e.size() == 1);
	CHECK(e.front().get_local_name() == "c");

	e.emplace_front("aa");
	CHECK(e.size() == 2);
	CHECK(e.front().get_local_name() == "aa");

	e.pop_back();
	CHECK(e.size() == 1);
	CHECK(e.back().get_local_name() == "aa");
	CHECK(e.front().get_local_name() == "aa");

	e.pop_back();
	CHECK(e.empty());

	// e.validate();
}

TEST_CASE("xml_copy")
{
	zeem::element e("test", { { "a", "een" }, { "b", "twee" } });

	e.push_back(e);
	e.push_back(e);

	zeem::element c("c", { { "x", "0" } });
	c.push_back(e);
	c.push_front(e);

	zeem::element c2 = c;

	CHECK(c == c2);
}

TEST_CASE("xml_copy2")
{
	zeem::element e("test", { { "a", "een" }, { "b", "twee" } });
	e.emplace_back("x1");
	e.nodes().emplace_back(zeem::comment("bla"));
	e.emplace_back("x2");

	CHECK((std::ostringstream() << e).str() == R"(<test a="een" b="twee"><x1/><!--bla--><x2/></test>)");

	auto e1 = e;

	zeem::element c1("test");
	c1.emplace_back(std::move(e));

	auto c2 = c1;

	zeem::element c3("test");
	for (auto &n : c1)
		c3.emplace_back(std::move(n));

	CHECK(c2 == c3);

	zeem::element e2("test", { { "a", "een" }, { "b", "twee" } });
	for (auto &n : c2.front().nodes())
		e2.nodes().emplace_back(n);

	CHECK(e2 == e1);
}

TEST_CASE("xml_iterators")
{
	zeem::element e("test");
	for (int i = 0; i < 10; ++i)
		e.emplace_back("n")->set_content(std::to_string(i));

	auto bi = e.begin();
	auto ei = e.end();

	for (int i = 0; i < 10; ++i)
	{
		auto i1 = bi;
		std::advance(i1, i);

		CHECK(i1->get_content() == std::to_string(i));

		auto i2 = ei;
		std::advance(i2, -i - 1);
		CHECK(i2->get_content() == std::to_string(9 - i));
	}
}

TEST_CASE("xml_iterators_2")
{
	zeem::element e("test");
	for (int i = 0; i < 10; ++i)
		e.emplace_back("n")->set_content(std::to_string(i));

	auto bi = e.begin();
	auto ei = e.end();

	for (int i = 0; i < 10; ++i)
	{
		auto bii = bi;
		std::advance(bii, i);
		CHECK(bii->get_content() == std::to_string(i));

		auto eii = ei;
		std::advance(eii, -i - 1);
		CHECK(eii->get_content() == std::to_string(9 - i));
	}

	// std::vector<zeem::node *> nodes;
	// for (auto &n : e.nodes())
	// 	nodes.push_back(&n);

	// CHECK(nodes.size() == 10);

	// for (int i = 0; i < 10; ++i)
	// {
	// 	zeem::element *el = dynamic_cast<zeem::element_container *>(nodes[i]);
	// 	CHECK(el != nullptr);
	// 	CHECK(el->get_content() == std::to_string(i));
	// }
}

TEST_CASE("xml_attributes")
{
	zeem::element e("test", { { "a", "1" }, { "b", "2" } });

	auto &attr = e.attributes();

	CHECK(attr.contains("a"));
	CHECK(attr.contains("b"));
	CHECK(not attr.contains("c"));

	CHECK(attr.find("a")->value() == "1");
	CHECK(attr.find("b")->value() == "2");
	CHECK(attr.find("c") == attr.end());

	auto i = attr.emplace("c", "3");

	CHECK(attr.contains("c"));
	CHECK(attr.find("c") == i.first);
	CHECK(attr.find("c")->value() == "3");
	CHECK(i.second == true);

	i = attr.emplace("c", "3a");

	CHECK(attr.contains("c"));
	CHECK(attr.find("c") == i.first);
	CHECK(attr.find("c")->value() == "3a");
	CHECK(i.second == false);
}

TEST_CASE("xml_doc")
{
	zeem::document doc;

	zeem::element e("test", { { "a", "1" }, { "b", "2" } });
	doc.emplace(std::move(e));

	zeem::document doc2(R"(<test a="1" b="2"/>)");

	CHECK(doc == doc2);

	using namespace zeem::literals;

	auto doc3 = R"(<test a="1" b="2"/>)"_xml;
	CHECK(doc == doc3);

	auto doc4 = R"(<l1><l2><l3><l4/></l3></l2></l1>)"_xml;

	CHECK_FALSE(doc4.empty());

	auto l1 = *doc4.child();
	CHECK(l1.get_qname() == "l1");
	CHECK(l1.size() == 1);

	auto l2 = l1.front();
	CHECK(l2.get_qname() == "l2");
	CHECK(l2.size() == 1);

	auto l3 = l2.front();
	CHECK(l3.get_qname() == "l3");
	CHECK(l3.size() == 1);

	auto l4 = l3.front();
	CHECK(l4.get_qname() == "l4");
	CHECK(l4.empty());

	auto i = l3.find_first("./l4");
	REQUIRE(i != l3.end());
	l3.erase(i);

	CHECK(l3.empty());

	i = l1.find_first(".//l3");
	CHECK(i != l1.end());

	CHECK_THROWS_AS(l1.erase(i), zeem::exception);

	l1.erase(l1.begin());

	CHECK(l1.empty());
}

TEST_CASE("xml_doc2")
{
	zeem::document doc;
	doc.emplace("first");
	CHECK_THROWS_AS(doc.emplace("second"), zeem::exception);
}

TEST_CASE("xml_xpath")
{
	using namespace zeem::literals;
	auto doc = R"(<test><a/><a/><a/></test>)"_xml;

	auto r = doc.find("//a");
	REQUIRE(r.size() == 3);
	CHECK(r.front()->get_qname() == "a");
}

TEST_CASE("xml_xpath_2")
{
	using namespace zeem::literals;
	auto doc = R"(
<test>
	<b/>
	<b>
		<c>
			<a>x</a>
		</c>
	</b>
	<b>
		<c>
			<a>
				<![CDATA[x]]>
			</a>
		</c>
	</b>
	<b>
		<c z='z'>
			<a>y</a>
		</c>
	</b>
</test>
)"_xml;

	auto r = doc.find("//b[c/a[contains(text(),'x')]]");
	REQUIRE(r.size() == 2);
	CHECK(r.front()->get_qname() == "b");

	auto r2 = doc.find("//b/c[@z='z']/a[text()='y']");
	REQUIRE(r2.size() == 1);
	CHECK(r2.front()->get_qname() == "a");
}

TEST_CASE("xml_namespaces")
{
	using namespace zeem::literals;

	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/zeem/m2">
<div>
<m:test0/>
<test1 m:if="${true}"/><test2 m:unless="${true}"/>
</div>
</data>
    )"_xml;

	auto &data = *doc.child();
	CHECK(data.parent() == &doc);
	CHECK(data.get_local_name() == "data");
	CHECK(data.get_ns().empty());

	CHECK(data.empty() == false);
	CHECK(data.begin() != data.end());

	auto &div = data.front();
	CHECK(div.get_local_name() == "div");
	CHECK(div.get_ns().empty());
	CHECK(div.parent() == &data);

	auto &test0 = div.front();
	CHECK(test0.parent() == &div);
	CHECK(test0.get_local_name() == "test0");
	CHECK(test0.get_qname() == "m:test0");
	CHECK(test0.get_ns() == "http://www.hekkelman.com/zeem/m2");

	auto &test1 = *(std::next(div.begin()));
	CHECK(test1.parent() == &div);
	CHECK(test1.get_local_name() == "test1");
	CHECK(test1.get_ns().empty());

	CHECK(test1.attributes().size() == 1);
	auto &test1_if = *test1.attributes().begin();
	CHECK(test1_if.get_local_name() == "if");
	CHECK(test1_if.get_qname() == "m:if");
	CHECK(test1_if.get_ns() == "http://www.hekkelman.com/zeem/m2");

	auto &test2 = *(std::next(std::next(div.begin())));
	CHECK(test2.parent() == &div);
	CHECK(test2.get_local_name() == "test2");
	CHECK(test2.get_ns().empty());

	CHECK(test2.attributes().size() == 1);
	auto &test2_unless = *test2.attributes().begin();
	CHECK(test2_unless.get_local_name() == "unless");
	CHECK(test2_unless.get_qname() == "m:unless");
	CHECK(test2_unless.get_ns() == "http://www.hekkelman.com/zeem/m2");
}

TEST_CASE("xml_namespaces_2")
{
	using namespace zeem::literals;

	auto doc = R"(<?xml version="1.0"?>
<data xmlns="http://www.hekkelman.com/zeem">
<x a="1">
<y a="2"/>
</x>
</data>
    )"_xml;

	auto &data = *doc.child();
	CHECK(data.parent() == &doc);
	CHECK(data.get_local_name() == "data");
	CHECK(data.get_ns() == "http://www.hekkelman.com/zeem");

	CHECK(data.empty() == false);
	CHECK(data.begin() != data.end());

	auto &x = data.front();
	CHECK(x.get_local_name() == "x");
	CHECK(x.get_qname() == "x");
	CHECK(x.get_ns() == "http://www.hekkelman.com/zeem");
	CHECK(x.parent() == &data);

	auto ax = x.attributes().find("a");
	CHECK(ax != x.attributes().end());
	CHECK(ax->value() == "1");
	CHECK(ax->get_ns() == "http://www.hekkelman.com/zeem");

	auto &y = x.front();
	CHECK(y.parent() == &x);
	CHECK(y.get_local_name() == "y");
	CHECK(y.get_qname() == "y");
	CHECK(y.get_ns() == "http://www.hekkelman.com/zeem");

	auto ay = y.attributes().find("a");
	CHECK(ay != y.attributes().end());
	CHECK(ay->value() == "2");
	CHECK(ay->get_ns() == "http://www.hekkelman.com/zeem");

	zeem::element data2("data", { { "xmlns", "http://www.hekkelman.com/zeem" } });
	auto x2 = data2.emplace_back("x", std::initializer_list<zeem::attribute>{ { "a", "1" } });
	x2->emplace_back("y", std::initializer_list<zeem::attribute>{ { "a", "2" } });

	CHECK(data == data2);
}

TEST_CASE("xml_namespaces_3")
{
	using namespace zeem::literals;

	auto doc = R"(<?xml version="1.0"?>
<data xmlns="http://www.hekkelman.com/zeem" xmlns:a="http://a.com/">
<x a="1">
<y a:a="2"/>
</x>
</data>
    )"_xml;

	auto &data = *doc.child();
	CHECK(data.parent() == &doc);
	CHECK(data.get_local_name() == "data");
	CHECK(data.get_ns() == "http://www.hekkelman.com/zeem");

	CHECK(data.empty() == false);
	CHECK(data.begin() != data.end());

	auto &x = data.front();
	CHECK(x.get_local_name() == "x");
	CHECK(x.get_qname() == "x");
	CHECK(x.get_ns() == "http://www.hekkelman.com/zeem");
	CHECK(x.parent() == &data);

	auto ax = x.attributes().find("a");
	CHECK(ax != x.attributes().end());
	CHECK(ax->value() == "1");
	CHECK(ax->get_ns() == "http://www.hekkelman.com/zeem");

	auto &y = x.front();
	CHECK(y.parent() == &x);
	CHECK(y.get_local_name() == "y");
	CHECK(y.get_qname() == "y");
	CHECK(y.get_ns() == "http://www.hekkelman.com/zeem");

	auto ay = y.attributes().find("a:a");
	CHECK(ay != y.attributes().end());
	CHECK(ay->value() == "2");
	CHECK(ay->get_ns() == "http://a.com/");
}

TEST_CASE("security_test_1")
{
	using namespace zeem::literals;

	zeem::element n("test");
	n.set_attribute("a", "a\xf6\"b");
	std::stringstream ss;
	CHECK_THROWS_AS((ss << n), zeem::exception);
}

// TEST_CASE("named_char_1")
// {
// 	const zeem::doctype::general_entity *c;

// 	c = zeem::get_named_character("AElig");
// 	CHECK(c != nullptr);
// 	CHECK(c->get_replacement() == "Æ");

// 	c = zeem::get_named_character("zwnj");
// 	CHECK(c != nullptr);
// 	CHECK(c->get_replacement() == "‌");

// 	c = zeem::get_named_character("supseteq");
// 	CHECK(c != nullptr);
// 	CHECK(c->get_replacement() == "⊇");
// }

TEST_CASE("named_char_2")
{
	using namespace zeem::literals;

	auto a = R"(<!DOCTYPE html SYSTEM "about:legacy-compat" ><test xmlns:m="http://www.hekkelman.com">&supseteq;</test>)"_xml;

	auto b = R"(<test xmlns:m="http://www.hekkelman.com">⊇</test>)"_xml;

	CHECK(a == b);
	if (not(a == b))
		std::cout << std::setw(2) << a << '\n'
				  << b << '\n';
}

TEST_CASE("doc-test-1")
{
	zeem::document doc;
	doc.nodes().emplace_back(zeem::comment("test"));
	CHECK(doc.empty());
}

TEST_CASE("trim")
{
	std::string s;

	s = "aap";
	zeem::trim(s);
	CHECK(s == "aap");

	s = " aap";
	zeem::trim(s);
	CHECK(s == "aap");

	s = "aap ";
	zeem::trim(s);
	CHECK(s == "aap");

	s = " aap ";
	zeem::trim(s);
	CHECK(s == "aap");

	s = "\t aap \n";
	zeem::trim(s);
	CHECK(s == "aap");
}

TEST_CASE("sort-1")
{
	using namespace zeem;

	element e("test", { { "aap", "1" },
						  { "noot", "2" },
						  { "mies", "3" },
						  { "boom", "4" },
						  { "roos", "5" },
						  { "vis", "6" },
						  { "vuur", "7" } });

	CHECK((std::ostringstream() << e).str() == R"(<test aap="1" noot="2" mies="3" boom="4" roos="5" vis="6" vuur="7"/>)");

	e.attributes().sort([](attribute &a, attribute &b)
		{ return a.get_local_name() < b.get_local_name(); });

	CHECK((std::ostringstream() << e).str() == R"(<test aap="1" boom="4" mies="3" noot="2" roos="5" vis="6" vuur="7"/>)");

	e.attributes().sort([](attribute &a, attribute &b)
		{ return a.value() < b.value(); });

	CHECK((std::ostringstream() << e).str() == R"(<test aap="1" noot="2" mies="3" boom="4" roos="5" vis="6" vuur="7"/>)");
}

TEST_CASE("emplace-1")
{
	zeem::element e1("e");

	zeem::cdata text("test");
	// e1.emplace_back(text);
	e1.nodes().insert(e1.end(), std::move(text));
}

TEST_CASE("utf8-1")
{
	using namespace zeem::literals;

	// Check overlong character
	CHECK_THROWS("<foo>\xC1\x81</foo>"_xml);

	// Check surrogate
	CHECK_THROWS("<foo>\xED\xA0\x80</foo>"_xml);
}

// Warning! AI generated code below

TEST_CASE("pi-1")
{
	zeem::processing_instruction pi("php", "echo 'hello';");

	CHECK(pi.type() == zeem::node_type::processing_instruction);
	CHECK(pi.get_target() == "php");
	CHECK(pi.get_qname() == "php");
	CHECK(pi.str() == "echo 'hello';");
	CHECK(pi.get_text() == "echo 'hello';");

	zeem::element wrapper("w");
	wrapper.nodes().emplace_back(zeem::processing_instruction(pi));
	std::ostringstream os;
	os << wrapper;
	CHECK(os.str() == "<w><?php echo 'hello';?></w>");
}

TEST_CASE("pi-2")
{
	zeem::processing_instruction pi;

	CHECK(pi.type() == zeem::node_type::processing_instruction);
	CHECK(pi.get_target().empty());
	CHECK(pi.str().empty());

	pi.set_target("xml-stylesheet");
	pi.set_text(R"(type="text/xsl" href="style.xsl")");

	CHECK(pi.get_target() == "xml-stylesheet");
	CHECK(pi.get_qname() == "xml-stylesheet");
	CHECK(pi.get_text() == "type=\"text/xsl\" href=\"style.xsl\"");

	zeem::element wrapper("w");
	wrapper.nodes().emplace_back(std::move(pi));
	std::ostringstream os;
	os << wrapper;
	CHECK(os.str() == R"(<w><?xml-stylesheet type="text/xsl" href="style.xsl"?></w>)");
}

TEST_CASE("pi-3")
{
	zeem::processing_instruction pi1("target", "data");
	zeem::processing_instruction pi2(pi1);

	CHECK(pi1.equals(&pi2));
	CHECK(pi2.get_target() == "target");
	CHECK(pi2.get_text() == "data");

	zeem::processing_instruction pi3(std::move(pi2));

	CHECK(pi3.get_target() == "target");
	CHECK(pi3.get_text() == "data");

	zeem::processing_instruction pi4;
	pi4 = pi1;

	CHECK(pi4.equals(&pi1));
	CHECK(pi4.get_target() == "target");
}

TEST_CASE("pi-4")
{
	zeem::processing_instruction pi1("a", "text1");
	zeem::processing_instruction pi2("b", "text2");

	using namespace zeem;
	swap(pi1, pi2);

	CHECK(pi1.get_target() == "b");
	CHECK(pi1.get_text() == "text2");
	CHECK(pi2.get_target() == "a");
	CHECK(pi2.get_text() == "text1");

	swap(pi1, pi2);

	CHECK(pi1.get_target() == "a");
	CHECK(pi1.get_text() == "text1");
	CHECK(pi2.get_target() == "b");
	CHECK(pi2.get_text() == "text2");
}

TEST_CASE("pi-5")
{
	zeem::element e("test");
	e.nodes().emplace_front(zeem::processing_instruction("xml-stylesheet", "type=\"text/xsl\" href=\"foo.xsl\""));

	auto nodes = e.nodes();
	CHECK(nodes.size() == 1);

	auto &pi = dynamic_cast<zeem::processing_instruction &>(*nodes.begin());
	CHECK(pi.get_target() == "xml-stylesheet");
	CHECK(pi.get_text() == "type=\"text/xsl\" href=\"foo.xsl\"");

	// element iterator skips non-element nodes
	CHECK(e.begin() == e.end());

	std::ostringstream os;
	os << e;
	CHECK(os.str() == R"(<test><?xml-stylesheet type="text/xsl" href="foo.xsl"?></test>)");
}

TEST_CASE("pi-6")
{
	zeem::element e("test");
	e.emplace_back("child");
	e.nodes().emplace_back(zeem::processing_instruction("target", "data"));
	e.emplace_back("child2");

	// nodes() sees all node types
	auto nodes = e.nodes();
	REQUIRE(nodes.size() == 3);

	std::ostringstream os;
	os << e;
	CHECK(os.str() == R"(<test><child/><?target data?><child2/></test>)");
}

TEST_CASE("pi-7")
{
	using namespace zeem::literals;

	auto doc = R"(<?xml version="1.0"?><?xml-stylesheet type="text/xsl" href="style.xsl"?><root/>)"_xml;

	auto nodes = doc.nodes();
	bool found = false;
	for (auto &n : nodes)
	{
		if (n.type() == zeem::node_type::processing_instruction)
		{
			auto &pi = dynamic_cast<zeem::processing_instruction &>(n);
			CHECK(pi.get_target() == "xml-stylesheet");
			CHECK(pi.get_text() == "type=\"text/xsl\" href=\"style.xsl\"");
			found = true;
		}
	}
	CHECK(found);

	// document child() returns the root element, skipping PI
	REQUIRE(doc.child() != nullptr);
	CHECK(doc.child()->get_qname() == "root");
}

TEST_CASE("pi-8")
{
	using namespace zeem::literals;

	auto doc = R"(<root><?app config="verbose"?><?app config="quiet"?></root>)"_xml;

	auto &root = *doc.child();
	auto nodes = root.nodes();

	std::size_t i = 0;
	for (auto &n : nodes)
	{
		if (n.type() == zeem::node_type::processing_instruction)
		{
			auto &pi = dynamic_cast<zeem::processing_instruction &>(n);
			CHECK(pi.get_target() == "app");
			CHECK(pi.type() == zeem::node_type::processing_instruction);

			if (i == 0)
				CHECK(pi.get_text() == "config=\"verbose\"");
			else
				CHECK(pi.get_text() == "config=\"quiet\"");
			++i;
		}
	}
	CHECK(i == 2);

	// element iterator skips both PIs
	CHECK(root.size() == 0);
	CHECK(root.begin() == root.end());
}

TEST_CASE("pi-9")
{
	zeem::processing_instruction pi1("target", "data");
	zeem::processing_instruction pi2("target", "data");
	zeem::processing_instruction pi3("other", "data");
	zeem::processing_instruction pi4("target", "other");

	CHECK(pi1.equals(&pi2));
	CHECK_FALSE(pi1.equals(&pi3));
	CHECK_FALSE(pi1.equals(&pi4));
	CHECK(pi1.equals(&pi1));

	zeem::comment c("data");
	CHECK_FALSE(pi1.equals(&c));

	zeem::element e("test");
	CHECK_FALSE(pi1.equals(&e));
}

TEST_CASE("pi-copy-in-element")
{
	zeem::element e("root");
	e.nodes().emplace_back(zeem::processing_instruction("php", "code"));

	auto copy = e;

	auto orig_nodes = e.nodes();
	auto copy_nodes = copy.nodes();

	REQUIRE(copy_nodes.size() == 1);

	auto &orig_pi = dynamic_cast<zeem::processing_instruction &>(*orig_nodes.begin());
	auto &copy_pi = dynamic_cast<zeem::processing_instruction &>(*copy_nodes.begin());

	CHECK(copy_pi.get_target() == "php");
	CHECK(copy_pi.get_text() == "code");
	CHECK_FALSE(&orig_pi == &copy_pi);
}

TEST_CASE("ns-move-1")
{
	zeem::element e("item");
	e.move_to_name_space("m", "http://example.com/ns", false, false);

	CHECK(e.get_qname() == "m:item");
	CHECK(e.get_local_name() == "item");
	CHECK(e.get_ns() == "http://example.com/ns");

	auto ns_attr = e.attributes().find("xmlns:m");
	REQUIRE(ns_attr != e.attributes().end());
	CHECK(ns_attr->value() == "http://example.com/ns");

	std::ostringstream os;
	os << e;
	CHECK(os.str() == R"(<m:item xmlns:m="http://example.com/ns"/>)");
}

TEST_CASE("ns-move-2")
{
	zeem::element e("item", { { "xmlns:m", "http://example.com/ns" } });
	e.set_qname("m", "item");

	// same prefix, same URI: should be a no-op
	e.move_to_name_space("m", "http://example.com/ns", false, false);

	CHECK(e.get_qname() == "m:item");
	CHECK(e.get_ns() == "http://example.com/ns");

	// URI already bound to prefix "m", trying to use "x" should throw
	CHECK_THROWS_AS(e.move_to_name_space("x", "http://example.com/ns", false, false), zeem::exception);
}

TEST_CASE("ns-move-3")
{
	zeem::element root("root");
	auto &child1 = *root.emplace_back("child1");
	auto &child2 = *child1.emplace_back("child2");

	root.move_to_name_space("m", "http://example.com/ns", true, false);

	CHECK(root.get_qname() == "m:root");
	CHECK(child1.get_qname() == "m:child1");
	CHECK(child2.get_qname() == "m:child2");

	CHECK(root.get_ns() == "http://example.com/ns");
	CHECK(child1.get_ns() == "http://example.com/ns");
	CHECK(child2.get_ns() == "http://example.com/ns");
}

TEST_CASE("ns-move-4")
{
	zeem::element e("item");
	e.set_attribute("a", "1");
	e.set_attribute("b", "2");

	e.move_to_name_space("m", "http://example.com/ns", false, true);

	CHECK(e.get_qname() == "m:item");

	// attributes should now be in the m: namespace
	auto a_attr = e.attributes().find("m:a");
	REQUIRE(a_attr != e.attributes().end());
	CHECK(a_attr->get_qname() == "m:a");
	CHECK(a_attr->get_ns() == "http://example.com/ns");

	auto b_attr = e.attributes().find("m:b");
	REQUIRE(b_attr != e.attributes().end());
	CHECK(b_attr->get_qname() == "m:b");
	CHECK(b_attr->get_ns() == "http://example.com/ns");

	std::ostringstream os;
	os << e;
	CHECK(os.str() == R"(<m:item m:a="1" m:b="2" xmlns:m="http://example.com/ns"/>)");
}

TEST_CASE("ns-move-5")
{
	zeem::element e("item");
	e.move_to_name_space("", "http://example.com/ns", false, false);

	CHECK(e.get_qname() == "item");
	CHECK(e.get_local_name() == "item");
	CHECK(e.get_ns() == "http://example.com/ns");

	auto ns_attr = e.attributes().find("xmlns");
	REQUIRE(ns_attr != e.attributes().end());
	CHECK(ns_attr->value() == "http://example.com/ns");

	std::ostringstream os;
	os << e;
	CHECK(os.str() == R"(<item xmlns="http://example.com/ns"/>)");
}

TEST_CASE("ns-move-7")
{
	zeem::element root("root");
	root.emplace_back("child1");
	root.emplace_back("child2");

	// non-recursive: children should not change
	root.move_to_name_space("m", "http://example.com/ns", false, false);

	CHECK(root.get_qname() == "m:root");
	CHECK(root.front().get_qname() == "child1");
	CHECK(root.back().get_qname() == "child2");
	CHECK(root.front().get_ns().empty());
	CHECK(root.back().get_ns().empty());
}

TEST_CASE("ns-move-8")
{
	using namespace zeem::literals;

	auto doc = R"(<?xml version="1.0"?><data><item a="1"><sub/></item></data>)"_xml;

	auto &data = *doc.child();
	auto &item = data.front();

	item.move_to_name_space("m", "http://example.com/ns", true, true);

	CHECK(data.get_qname() == "data");
	CHECK(item.get_qname() == "m:item");
	CHECK(item.front().get_qname() == "m:sub");

	auto a_attr = item.attributes().find("m:a");
	REQUIRE(a_attr != item.attributes().end());
	CHECK(a_attr->get_qname() == "m:a");

	std::ostringstream os;
	os << doc;
	CHECK(os.str() == R"(<data><m:item m:a="1" xmlns:m="http://example.com/ns"><m:sub/></m:item></data>)");
}

TEST_CASE("set_qname-1")
{
	zeem::element e("item");

	CHECK(e.get_qname() == "item");
	CHECK(e.get_local_name() == "item");
	CHECK(e.get_prefix().empty());

	e.set_qname("newname");

	CHECK(e.get_qname() == "newname");
	CHECK(e.get_local_name() == "newname");
	CHECK(e.get_prefix().empty());

	std::ostringstream os;
	os << e;
	CHECK(os.str() == "<newname/>");
}

TEST_CASE("set_qname-2")
{
	zeem::element e("item");

	// two-argument form: prefix + name
	e.set_qname("m", "item");

	CHECK(e.get_qname() == "m:item");
	CHECK(e.get_local_name() == "item");
	CHECK(e.get_prefix() == "m");

	std::ostringstream os;
	os << e;
	CHECK(os.str() == "<m:item/>");
}

TEST_CASE("set_qname-3")
{
	zeem::element e("item");

	// two-argument form with empty prefix: qname is just the name
	e.set_qname("", "plain");

	CHECK(e.get_qname() == "plain");
	CHECK(e.get_local_name() == "plain");
	CHECK(e.get_prefix().empty());

	std::ostringstream os;
	os << e;
	CHECK(os.str() == "<plain/>");
}

TEST_CASE("set_qname-4")
{
	zeem::element e("item");

	// change qname preserves attributes and children
	e.set_attribute("a", "1");
	e.emplace_back("child");

	e.set_qname("renamed");

	CHECK(e.get_qname() == "renamed");
	CHECK(e.attributes().size() == 1);
	CHECK(e.size() == 1);
	CHECK(e.front().get_qname() == "child");

	std::ostringstream os;
	os << e;
	CHECK(os.str() == R"(<renamed a="1"><child/></renamed>)");
}

TEST_CASE("set_qname-5")
{
	using namespace zeem::literals;

	// parse with a namespaced qname, then rename
	auto doc = R"(<?xml version="1.0"?><data xmlns:m="http://example.com"><m:item/></data>)"_xml;

	auto &data = *doc.child();
	auto &item = data.front();

	CHECK(item.get_qname() == "m:item");
	CHECK(item.get_ns() == "http://example.com");

	item.set_qname("x", "item");

	CHECK(item.get_qname() == "x:item");
	CHECK(item.get_local_name() == "item");
	CHECK(item.get_prefix() == "x");

	// namespace URI is resolved via xmlns:m, but the prefix in qname is now "x"
	// so get_ns() can no longer resolve it (no xmlns:x exists)
	CHECK(item.get_ns().empty());
}

TEST_CASE("set_text-1")
{
	zeem::element e("item");
	e.set_text("hello");

	CHECK(e.get_content() == "hello");

	auto nodes = e.nodes();
	REQUIRE(nodes.size() == 1);
	CHECK(nodes.begin()->type() == zeem::node_type::text);

	std::ostringstream os;
	os << e;
	CHECK(os.str() == "<item>hello</item>");
}

TEST_CASE("set_text-2")
{
	zeem::element e("item");
	e.set_text("first");
	e.set_text("second");

	// set_text replaces all text nodes
	CHECK(e.get_content() == "second");

	auto nodes = e.nodes();
	std::size_t text_count = 0;
	for (auto &n : nodes)
	{
		if (n.type() == zeem::node_type::text)
			++text_count;
	}
	CHECK(text_count == 1);

	std::ostringstream os;
	os << e;
	CHECK(os.str() == "<item>second</item>");
}

TEST_CASE("set_text-3")
{
	zeem::element e("item");
	e.emplace_back("child");
	e.set_text("text");

	// set_text removes text but preserves element children
	// text is appended after existing children
	CHECK(e.get_content() == "text");
	CHECK(e.size() == 1);
	CHECK(e.front().get_qname() == "child");

	std::ostringstream os;
	os << e;
	CHECK(os.str() == "<item><child/>text</item>");
}

TEST_CASE("set_text-4")
{
	zeem::element e("item");
	e.set_text("hello");
	e.set_attribute("a", "1");

	// set_text does not affect attributes
	CHECK(e.get_content() == "hello");
	CHECK(e.attributes().size() == 1);

	std::ostringstream os;
	os << e;
	CHECK(os.str() == R"(<item a="1">hello</item>)");
}

TEST_CASE("set_text-5")
{
	zeem::element e("item");
	e.set_text("");

	// empty string still creates a text node
	CHECK(e.get_content().empty());

	auto nodes = e.nodes();
	CHECK(nodes.size() == 1);
	CHECK(nodes.begin()->type() == zeem::node_type::text);
}

TEST_CASE("add_text-1")
{
	zeem::element e("item");
	e.add_text("hello");

	CHECK(e.get_content() == "hello");

	std::ostringstream os;
	os << e;
	CHECK(os.str() == "<item>hello</item>");
}

TEST_CASE("add_text-2")
{
	zeem::element e("item");
	e.add_text("hel");
	e.add_text("lo");

	// adjacent adds should append to the same text node
	CHECK(e.get_content() == "hello");

	auto nodes = e.nodes();
	std::size_t text_count = 0;
	for (auto &n : nodes)
	{
		if (n.type() == zeem::node_type::text)
			++text_count;
	}
	CHECK(text_count == 1);

	std::ostringstream os;
	os << e;
	CHECK(os.str() == "<item>hello</item>");
}

TEST_CASE("add_text-3")
{
	zeem::element e("item");
	e.add_text("text1");
	e.emplace_back("child");
	e.add_text("text2");

	// non-text child in between means a new text node is created
	CHECK(e.get_content() == "text1text2");

	auto nodes = e.nodes();
	std::size_t text_count = 0;
	for (auto &n : nodes)
	{
		if (n.type() == zeem::node_type::text)
			++text_count;
	}
	CHECK(text_count == 2);

	std::ostringstream os;
	os << e;
	CHECK(os.str() == "<item>text1<child/>text2</item>");
}

TEST_CASE("add_text-4")
{
	zeem::element e("item");
	e.add_text("a");
	e.add_text("b");
	e.add_text("c");

	CHECK(e.get_content() == "abc");

	// all three should be in one text node
	auto nodes = e.nodes();
	std::size_t text_count = 0;
	for (auto &n : nodes)
	{
		if (n.type() == zeem::node_type::text)
			++text_count;
	}
	CHECK(text_count == 1);
}

TEST_CASE("flatten_text-1")
{
	zeem::element e("item");

	// manually insert adjacent text nodes
	auto nodes = e.nodes();
	nodes.emplace_back(zeem::text("hello"));
	nodes.emplace_back(zeem::text(" world"));

	CHECK(e.get_content() == "hello world");

	// two text nodes before flatten
	std::size_t text_count = 0;
	for (auto &n : nodes)
	{
		if (n.type() == zeem::node_type::text)
			++text_count;
	}
	CHECK(text_count == 2);

	e.flatten_text();

	// after flatten: adjacent text nodes are merged into one
	CHECK(e.get_content() == "hello world");

	auto nodes2 = e.nodes();
	text_count = 0;
	for (auto &n : nodes2)
	{
		if (n.type() == zeem::node_type::text)
			++text_count;
	}
	CHECK(text_count == 1);

	std::ostringstream os;
	os << e;
	CHECK(os.str() == "<item>hello world</item>");
}

TEST_CASE("flatten_text-2")
{
	zeem::element e("item");

	// text + element + text: non-adjacent text nodes should NOT be merged
	auto nodes = e.nodes();
	nodes.emplace_back(zeem::text("hello"));
	e.emplace_back("child");
	nodes.emplace_back(zeem::text("world"));

	e.flatten_text();

	CHECK(e.get_content() == "helloworld");

	auto nodes2 = e.nodes();
	std::size_t text_count = 0;
	for (auto &n : nodes2)
	{
		if (n.type() == zeem::node_type::text)
			++text_count;
	}
	CHECK(text_count == 2);
}

TEST_CASE("flatten_text-3")
{
	zeem::element e("item");

	// three adjacent text nodes should all merge into one
	auto nodes = e.nodes();
	nodes.emplace_back(zeem::text("a"));
	nodes.emplace_back(zeem::text("b"));
	nodes.emplace_back(zeem::text("c"));

	e.flatten_text();

	CHECK(e.get_content() == "abc");

	auto nodes2 = e.nodes();
	std::size_t text_count = 0;
	for (auto &n : nodes2)
	{
		if (n.type() == zeem::node_type::text)
			++text_count;
	}
	CHECK(text_count == 1);

	std::ostringstream os;
	os << e;
	CHECK(os.str() == "<item>abc</item>");
}

TEST_CASE("flatten_text-4")
{
	zeem::element e("item");

	// text + comment + text: should NOT be merged (comment in between)
	auto nodes = e.nodes();
	nodes.emplace_back(zeem::text("hello"));
	nodes.emplace_back(zeem::comment("comment"));
	nodes.emplace_back(zeem::text("world"));

	e.flatten_text();

	CHECK(e.get_content() == "helloworld");

	auto nodes2 = e.nodes();
	std::size_t text_count = 0;
	for (auto &n : nodes2)
	{
		if (n.type() == zeem::node_type::text)
			++text_count;
	}
	CHECK(text_count == 2);
}

TEST_CASE("flatten_text-5")
{
	zeem::element e("item");

	// empty element: flatten_text should be a no-op
	e.flatten_text();

	CHECK(e.get_content().empty());
	CHECK(e.empty());
}

TEST_CASE("flatten_text-6")
{
	zeem::element e("item");

	// single text node: flatten_text should be a no-op
	auto nodes = e.nodes();
	nodes.emplace_back(zeem::text("hello"));

	e.flatten_text();

	CHECK(e.get_content() == "hello");

	auto nodes2 = e.nodes();
	std::size_t text_count = 0;
	for (auto &n : nodes2)
	{
		if (n.type() == zeem::node_type::text)
			++text_count;
	}
	CHECK(text_count == 1);
}

TEST_CASE("attr_erase-1")
{
	zeem::element e("item", { { "a", "1" }, { "b", "2" }, { "c", "3" } });

	auto erased = e.attributes().erase("b");
	CHECK(erased == 1);
	CHECK(e.attributes().size() == 2);
	CHECK_FALSE(e.attributes().contains("b"));
	CHECK(e.attributes().contains("a"));
	CHECK(e.attributes().contains("c"));

	std::ostringstream os;
	os << e;
	CHECK(os.str() == R"(<item a="1" c="3"/>)");
}

TEST_CASE("attr_erase-2")
{
	zeem::element e("item", { { "a", "1" } });

	// erasing a non-existent key returns 0 and leaves the set unchanged
	auto erased = e.attributes().erase("z");
	CHECK(erased == 0);
	CHECK(e.attributes().size() == 1);
	CHECK(e.attributes().contains("a"));

	std::ostringstream os;
	os << e;
	CHECK(os.str() == R"(<item a="1"/>)");
}

TEST_CASE("attr_erase-3")
{
	zeem::element e("item");

	// erasing from empty attribute set returns 0
	auto erased = e.attributes().erase("a");
	CHECK(erased == 0);
	CHECK(e.attributes().empty());
}

TEST_CASE("attr_erase-4")
{
	zeem::element e("item", { { "a", "1" }, { "b", "2" } });

	// erase all attributes one by one
	CHECK(e.attributes().erase("a") == 1);
	CHECK(e.attributes().size() == 1);
	CHECK(e.attributes().erase("b") == 1);
	CHECK(e.attributes().empty());

	// erasing again from empty set returns 0
	CHECK(e.attributes().erase("a") == 0);

	std::ostringstream os;
	os << e;
	CHECK(os.str() == "<item/>");
}

TEST_CASE("attr_erase-5")
{
	zeem::element e("item", { { "a", "1" }, { "b", "2" }, { "c", "3" } });

	// erase middle attribute
	e.attributes().erase("b");

	// remaining attributes should be intact
	auto ai = e.attributes().begin();
	REQUIRE(ai != e.attributes().end());
	CHECK(ai->get_local_name() == "a");
	CHECK(ai->value() == "1");
	++ai;
	REQUIRE(ai != e.attributes().end());
	CHECK(ai->get_local_name() == "c");
	CHECK(ai->value() == "3");
	++ai;
	CHECK(ai == e.attributes().end());
}

TEST_CASE("attr_erase-6")
{
	zeem::element e("item");
	e.set_attribute("xmlns:m", "http://example.com");
	e.set_attribute("a", "1");

	// erase a namespace attribute by its qname
	auto erased = e.attributes().erase("xmlns:m");
	CHECK(erased == 1);
	CHECK(e.attributes().size() == 1);
	CHECK(e.attributes().contains("a"));
	CHECK_FALSE(e.attributes().contains("xmlns:m"));

	std::ostringstream os;
	os << e;
	CHECK(os.str() == R"(<item a="1"/>)");
}

TEST_CASE("attr_erase-7")
{
	using namespace zeem::literals;

	// erase from a parsed document's element
	auto doc = R"(<?xml version="1.0"?><item a="1" b="2" c="3"/>)"_xml;

	auto &item = *doc.child();
	CHECK(item.attributes().size() == 3);

	item.attributes().erase("b");
	CHECK(item.attributes().size() == 2);

	std::ostringstream os;
	os << item;
	CHECK(os.str() == R"(<item a="1" c="3"/>)");
}

TEST_CASE("attr_erase-8")
{
	zeem::element e("item", { { "a", "1" }, { "b", "2" } });

	// erase does not affect the element itself or its children
	e.emplace_back("child");

	e.attributes().erase("a");

	CHECK(e.get_qname() == "item");
	CHECK(e.size() == 1);
	CHECK(e.front().get_qname() == "child");
	CHECK(e.attributes().size() == 1);
	CHECK(e.attributes().contains("b"));
}
