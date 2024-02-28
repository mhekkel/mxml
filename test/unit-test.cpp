#define CATCH_CONFIG_RUNNER

#if CATCH22
# include <catch2/catch.hpp>
#else
# include <catch2/catch_all.hpp>
#endif

#include <filesystem>

import mxml;

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

TEST_CASE("test_1")
{
	mxml::element n("test");

	CHECK(n.name() == "test");

	SECTION("insert")
	{
		auto i1 = n.insert(n.end(), mxml::element("c1"));

		CHECK(i1->name() == "c1");
		CHECK(i1->empty());
		CHECK(i1->size() == 0);
		CHECK(n.size() == 1);
		CHECK(n.front().name() == "c1");
		for (auto &e : n)
			CHECK(e.parent() == &n);

		auto i2 = n.insert(n.end(), mxml::element("c2"));

		CHECK(i2->name() == "c2");
		CHECK(i2->empty());
		CHECK(i2->size() == 0);
		CHECK(n.size() == 2);
		CHECK(n.front().name() == "c1");
		CHECK(n.back().name() == "c2");
		for (auto &e : n)
			CHECK(e.parent() == &n);

		auto i3 = n.insert(n.begin(), mxml::element("c0"));
		CHECK(i3->name() == "c0");
		CHECK(i3->empty());
		CHECK(i3->size() == 0);
		CHECK(n.size() == 3);
		CHECK(n.front().name() == "c0");
		CHECK(n.back().name() == "c2");

		mxml::element c3("c3");
		auto i4 = n.insert(n.end(), c3);
		CHECK(i4->name() == "c3");
		CHECK(i4->empty());
		CHECK(i4->size() == 0);
		CHECK(n.size() == 4);
		CHECK(n.front().name() == "c0");
		CHECK(n.back().name() == "c3");

		for (auto &e : n)
		{
			CHECK(e.parent() == &n);
			CHECK(e.empty());
			CHECK(e.size() == 0);
		}

		for (int i = 0; auto &e : n)
		{
			CHECK(e.name() == "c" + std::to_string(i));
			++i;
		}

		// CHECK(find(n.begin(), n.end(), i1) != n.end());

		auto n2 = n;

		CHECK(n2.size() == 4);
		CHECK(n2.front().name() == "c0");
		CHECK(n2.back().name() == "c3");
		for (auto &e : n2)
			CHECK(e.parent() == &n2);

		for (int i = 0; auto &e : n2)
		{
			CHECK(e.name() == "c" + std::to_string(i));
			++i;
		}

		auto n3(std::move(n2));

		CHECK(n2.empty());
		CHECK(n3.size() == 4);
		CHECK(n3.front().name() == "c0");
		CHECK(n3.back().name() == "c3");
		for (auto &e : n3)
			CHECK(e.parent() == &n3);

		for (int i = 0; auto &e : n3)
		{
			CHECK(e.name() == "c" + std::to_string(i));
			++i;
		}

		mxml::element n4;
		n4 = std::move(n3);

		CHECK(n3.empty());
		CHECK(n4.size() == 4);
		CHECK(n4.front().name() == "c0");
		CHECK(n4.back().name() == "c3");
		for (auto &e : n4)
			CHECK(e.parent() == &n4);

		for (int i = 0; auto &e : n4)
		{
			CHECK(e.name() == "c" + std::to_string(i));
			++i;
		}

		// erase

		for (int i = 4; i > 0; --i)
		{
			n4.erase(n4.begin());
			CHECK(n4.size() == i - 1);
		}
		CHECK(n4.empty());

		for (int i = 4; i > 0; --i)
		{
			n.erase(std::prev(n.end()));
			CHECK(n.size() == i - 1);
		}
		CHECK(n.empty());
	}

	SECTION("emplace")
	{
		auto t = n.emplace(n.end(), "c1");

		CHECK(t->name() == "c1");
		CHECK(n.size() == 1);
		CHECK(n.front().name() == "c1");
		for (auto &e : n)
			CHECK(e.parent() == &n);

		auto t2 = n.emplace_back("c2");

		CHECK(t2->name() == "c2");
		CHECK(n.size() == 2);
		CHECK(n.front().name() == "c1");
		CHECK(n.back().name() == "c2");
		for (auto &e : n)
			CHECK(e.parent() == &n);

		auto t3 = n.emplace_front("c0");
		CHECK(t3->name() == "c0");
		CHECK(n.size() == 3);
		CHECK(n.front().name() == "c0");
		CHECK(n.back().name() == "c2");
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
	using namespace mxml;

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
	mxml::element n("data", { { "attr1", "value-1" }, { "attr2", "value-2" } });

	CHECK(n.name() == "data");
	CHECK(n.attributes().empty() == false);
	CHECK(n.attributes().size() == 2);
	CHECK(n.attributes().begin() != n.attributes().end());

	size_t i = 0;
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
	mxml::element e("test");
	e.nodes().emplace_back(mxml::comment("commentaar"));
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
	mxml::element e("test");

	mxml::element a("aap");

	e.nodes().emplace(e.end(), a);
	CHECK(a.name() == "aap");
	CHECK((std::ostringstream() << e).str() == R"(<test><aap/></test>)");

	e.nodes().emplace(e.end(), std::move(a));
	CHECK(a.name() == "");
	CHECK((std::ostringstream() << e).str() == R"(<test><aap/><aap/></test>)");

	mxml::element b("noot");
	// mxml::node &n = b;

	// e.nodes().emplace(e.end(), n);
	CHECK(e.nodes().emplace(e.end(), b)->name() == "noot");
	CHECK((std::ostringstream() << e).str() == R"(<test><aap/><aap/><noot/></test>)");

	const auto &n2 = b;
	CHECK(e.nodes().emplace(e.end(), n2)->name() == "noot");
	CHECK((std::ostringstream() << e).str() == R"(<test><aap/><aap/><noot/><noot/></test>)");

	auto &&n3 = std::move(b);
	CHECK(e.nodes().emplace(e.end(), std::move(n3))->name() == "noot");
	CHECK(b.name() == "");
	CHECK((std::ostringstream() << e).str() == R"(<test><aap/><aap/><noot/><noot/><noot/></test>)");

	e.attributes().emplace("attr1", "value1");

	std::ostringstream s;
	s << e;
	CHECK(s.str() == R"(<test attr1="value1"><aap/><aap/><noot/><noot/><noot/></test>)");
}

TEST_CASE("xml_attributes_1")
{
	using namespace mxml::literals;

	auto doc = R"(<test xmlns:m="http://www.hekkelman.com">
<t1 m:a="v"/>
</test>)"_xml;

	auto &t = doc.child()->front();

	for (auto &a : t.attributes())
	{
		CHECK(a.name() == "a");
		CHECK(a.get_qname() == "m:a");
		CHECK(a.get_ns() == "http://www.hekkelman.com");
	}

	for (auto a : t.attributes())
	{
		CHECK(a.name() == "a");
		CHECK(a.get_qname() == "m:a");

		// the attribute was copied and thus lost namespace information
		CHECK(a.get_ns() != "http://www.hekkelman.com");
	}
}

TEST_CASE("xml_emplace")
{
	mxml::element e("test");

	e.emplace_back("test2", std::initializer_list<mxml::attribute>{ { "a1", "v1" }, { "a2", "v2" } });

	std::ostringstream s;
	s << e;
	CHECK(s.str() == R"(<test><test2 a1="v1" a2="v2"/></test>)");

	e.emplace_front("test1", std::initializer_list<mxml::attribute>{ { "a1", "v1" }, { "a2", "v2" } });

	std::ostringstream s2;
	s2 << e;
	CHECK(s2.str() == R"(<test><test1 a1="v1" a2="v2"/><test2 a1="v1" a2="v2"/></test>)");
}

TEST_CASE("xml_4")
{
	mxml::element e("test");
	e.emplace_back(mxml::element("test2", { { "attr1", "een" }, { "attr2", "twee" } }));

	std::ostringstream s;
	s << e;
	CHECK(s.str() == R"(<test><test2 attr1="een" attr2="twee"/></test>)");
}

TEST_CASE("xml_5_compare")
{
	mxml::element a("test", { { "a", "v1" }, { "b", "v2" } });
	mxml::element b("test", { { "b", "v2" }, { "a", "v1" } });

	CHECK(a == b);
}

TEST_CASE("xml_container_and_iterators")
{
	mxml::element e("test");

	mxml::element n("a");
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
	CHECK(e.front().name() == "c");

	e.push_front({ "aa" });
	CHECK(e.size() == 2);
	CHECK(e.front().name() == "aa");

	e.pop_back();
	CHECK(e.size() == 1);
	CHECK(e.back().name() == "aa");
	CHECK(e.front().name() == "aa");

	e.pop_back();
	CHECK(e.empty());

	// e.validate();
}

TEST_CASE("xml_copy")
{
	mxml::element e("test", { { "a", "een" }, { "b", "twee" } });

	e.push_back(e);
	e.push_back(e);

	mxml::element c("c", { { "x", "0" } });
	c.push_back(e);
	c.push_front(e);

	mxml::element c2 = c;

	CHECK(c == c2);
}

TEST_CASE("xml_copy2")
{
	mxml::element e("test", { { "a", "een" }, { "b", "twee" } });
	e.emplace_back("x1");
	e.nodes().emplace_back(mxml::comment("bla"));
	e.emplace_back("x2");

	CHECK((std::ostringstream() << e).str() == R"(<test a="een" b="twee"><x1/><!--bla--><x2/></test>)");

	auto e1 = e;

	mxml::element c1("test");
	c1.emplace_back(std::move(e));

	auto c2 = c1;

	mxml::element c3("test");
	for (auto &n : c1)
		c3.emplace_back(std::move(n));

	CHECK(c2 == c3);

// 	mxml::element e2("test", { { "a", "een" }, { "b", "twee" } });
// 	for (auto &n : c2.front())
// 		e2.emplace_back(std::move(n));
// 	for (auto &n : c2.front().nodes())
// 		e2.nodes().emplace_back(std::move(n));

// 	CHECK(e2 == e1);

// 	// e1.validate();
// 	// e2.validate();
}

TEST_CASE("xml_iterators")
{
	mxml::element e("test");
	for (int i = 0; i < 10; ++i)
		e.emplace_back("n")->set_content(std::to_string(i));

	auto bi = e.begin();
	auto ei = e.end();

	// for (int i = 0; i < 10; ++i)
	// {
	// 	CHECK((bi + i)->get_content() == std::to_string(i));
	// 	CHECK((ei - i - 1)->get_content() == std::to_string(9 - i));
	// }
}

TEST_CASE("xml_iterators_2")
{
	mxml::element e("test");
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
		std::advance(eii, - i - 1);
		CHECK(eii->get_content() == std::to_string(9 - i));
	}

	// std::vector<mxml::node *> nodes;
	// for (auto &n : e.nodes())
	// 	nodes.push_back(&n);

	// CHECK(nodes.size() == 10);

	// for (int i = 0; i < 10; ++i)
	// {
	// 	mxml::element *el = dynamic_cast<mxml::element *>(nodes[i]);
	// 	CHECK(el != nullptr);
	// 	CHECK(el->get_content() == std::to_string(i));
	// }
}

TEST_CASE("xml_attributes")
{
	mxml::element e("test", { { "a", "1" }, { "b", "2" } });

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
	mxml::document doc;

	mxml::element e("test", { { "a", "1" }, { "b", "2" } });
	doc.emplace(std::move(e));

	mxml::document doc2(R"(<test a="1" b="2"/>)");

	CHECK(doc == doc2);

	using namespace mxml::literals;

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
	REQUIRE(i != nullptr);
	l3.erase(i);

	CHECK(l3.empty());

	i = l1.find_first(".//l3");
	CHECK(i != nullptr);

	CHECK_THROWS_AS(l1.erase(i), mxml::exception);

	l1.erase(l1.begin());

	CHECK(l1.empty());
}

TEST_CASE("xml_doc2")
{
	mxml::document doc;
	doc.emplace("first");
	CHECK_THROWS_AS(doc.emplace("second"), mxml::exception);
}

TEST_CASE("xml_xpath")
{
	using namespace mxml::literals;
	auto doc = R"(<test><a/><a/><a/></test>)"_xml;

	auto r = doc.find("//a");
	REQUIRE(r.size() == 3);
	CHECK(r.front()->get_qname() == "a");
}

TEST_CASE("xml_xpath_2")
{
	using namespace mxml::literals;
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
	using namespace mxml::literals;

	auto doc = R"(<?xml version="1.0"?>
<data xmlns:m="http://www.hekkelman.com/libzeep/m2">
<div>
<m:test0/>
<test1 m:if="${true}"/><test2 m:unless="${true}"/>
</div>
</data>
    )"_xml;

	auto &data = *doc.child();
	CHECK(data.parent() == &doc);
	CHECK(data.name() == "data");
	CHECK(data.get_ns().empty());

	CHECK(data.empty() == false);
	CHECK(data.begin() != data.end());

	auto &div = data.front();
	CHECK(div.name() == "div");
	CHECK(div.get_ns().empty());
	CHECK(div.parent() == &data);

	auto &test0 = div.front();
	CHECK(test0.parent() == &div);
	CHECK(test0.name() == "test0");
	CHECK(test0.get_qname() == "m:test0");
	CHECK(test0.get_ns() == "http://www.hekkelman.com/libzeep/m2");

	auto &test1 = *(std::next(div.begin()));
	CHECK(test1.parent() == &div);
	CHECK(test1.name() == "test1");
	CHECK(test1.get_ns().empty());

	CHECK(test1.attributes().size() == 1);
	auto &test1_if = *test1.attributes().begin();
	CHECK(test1_if.name() == "if");
	CHECK(test1_if.get_qname() == "m:if");
	CHECK(test1_if.get_ns() == "http://www.hekkelman.com/libzeep/m2");

	auto &test2 = *(std::next(std::next(div.begin())));
	CHECK(test2.parent() == &div);
	CHECK(test2.name() == "test2");
	CHECK(test2.get_ns().empty());

	CHECK(test2.attributes().size() == 1);
	auto &test2_unless = *test2.attributes().begin();
	CHECK(test2_unless.name() == "unless");
	CHECK(test2_unless.get_qname() == "m:unless");
	CHECK(test2_unless.get_ns() == "http://www.hekkelman.com/libzeep/m2");
}

TEST_CASE("xml_namespaces_2")
{
	using namespace mxml::literals;

	auto doc = R"(<?xml version="1.0"?>
<data xmlns="http://www.hekkelman.com/libzeep">
<x a="1">
<y a="2"/>
</x>
</data>
    )"_xml;

	auto &data = *doc.child();
	CHECK(data.parent() == &doc);
	CHECK(data.name() == "data");
	CHECK(data.get_ns() == "http://www.hekkelman.com/libzeep");

	CHECK(data.empty() == false);
	CHECK(data.begin() != data.end());

	auto &x = data.front();
	CHECK(x.name() == "x");
	CHECK(x.get_qname() == "x");
	CHECK(x.get_ns() == "http://www.hekkelman.com/libzeep");
	CHECK(x.parent() == &data);

	auto ax = x.attributes().find("a");
	CHECK(ax != x.attributes().end());
	CHECK(ax->value() == "1");
	CHECK(ax->get_ns() == "http://www.hekkelman.com/libzeep");

	auto &y = x.front();
	CHECK(y.parent() == &x);
	CHECK(y.name() == "y");
	CHECK(y.get_qname() == "y");
	CHECK(y.get_ns() == "http://www.hekkelman.com/libzeep");

	auto ay = y.attributes().find("a");
	CHECK(ay != y.attributes().end());
	CHECK(ay->value() == "2");
	CHECK(ay->get_ns() == "http://www.hekkelman.com/libzeep");

	mxml::element data2("data", { { "xmlns", "http://www.hekkelman.com/libzeep" } });
	auto x2 = data2.emplace_back("x", std::initializer_list<mxml::attribute>{ { "a", "1" } });
	x2->emplace_back("y", std::initializer_list<mxml::attribute>{ { "a", "2" } });

	CHECK(data == data2);
}

TEST_CASE("xml_namespaces_3")
{
	using namespace mxml::literals;

	auto doc = R"(<?xml version="1.0"?>
<data xmlns="http://www.hekkelman.com/libzeep" xmlns:a="http://a.com/">
<x a="1">
<y a:a="2"/>
</x>
</data>
    )"_xml;

	auto &data = *doc.child();
	CHECK(data.parent() == &doc);
	CHECK(data.name() == "data");
	CHECK(data.get_ns() == "http://www.hekkelman.com/libzeep");

	CHECK(data.empty() == false);
	CHECK(data.begin() != data.end());

	auto &x = data.front();
	CHECK(x.name() == "x");
	CHECK(x.get_qname() == "x");
	CHECK(x.get_ns() == "http://www.hekkelman.com/libzeep");
	CHECK(x.parent() == &data);

	auto ax = x.attributes().find("a");
	CHECK(ax != x.attributes().end());
	CHECK(ax->value() == "1");
	CHECK(ax->get_ns() == "http://www.hekkelman.com/libzeep");

	auto &y = x.front();
	CHECK(y.parent() == &x);
	CHECK(y.name() == "y");
	CHECK(y.get_qname() == "y");
	CHECK(y.get_ns() == "http://www.hekkelman.com/libzeep");

	auto ay = y.attributes().find("a:a");
	CHECK(ay != y.attributes().end());
	CHECK(ay->value() == "2");
	CHECK(ay->get_ns() == "http://a.com/");
}

TEST_CASE("security_test_1")
{
	using namespace mxml::literals;

	mxml::element n("test");
	n.set_attribute("a", "a\xf6\"b");
	std::stringstream ss;
	CHECK_THROWS_AS((ss << n), mxml::exception);
}

// TEST_CASE("named_char_1")
// {
// 	const mxml::doctype::general_entity *c;

// 	c = mxml::get_named_character("AElig");
// 	CHECK(c != nullptr);
// 	CHECK(c->get_replacement() == "Æ");

// 	c = mxml::get_named_character("zwnj");
// 	CHECK(c != nullptr);
// 	CHECK(c->get_replacement() == "‌");

// 	c = mxml::get_named_character("supseteq");
// 	CHECK(c != nullptr);
// 	CHECK(c->get_replacement() == "⊇");
// }

TEST_CASE("named_char_2")
{
	using namespace mxml::literals;

	auto a = R"(<!DOCTYPE html SYSTEM "about:legacy-compat" ><test xmlns:m="http://www.hekkelman.com">&supseteq;</test>)"_xml;

	auto b = R"(<test xmlns:m="http://www.hekkelman.com">⊇</test>)"_xml;

	CHECK((a == b) == true);
	if (not(a == b))
		std::cout << std::setw(2) << a << '\n'
				  << b << '\n';
}

TEST_CASE("doc-test-1")
{
	mxml::document doc;
	doc.nodes().emplace_back(mxml::comment("test"));
	CHECK(doc.empty());
}