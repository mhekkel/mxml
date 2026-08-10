// Copyright (c) 2026 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

#define CATCH_CONFIG_RUNNER

#include <catch2/catch_session.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <filesystem>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#if ZEEM_CXX_MODULE
import zeem;
#else
# include "zeem/zeem.hpp"
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

namespace
{

using rng_type = std::mt19937;

template <typename T>
const T &pick(rng_type &rng, const std::vector<T> &values)
{
	return values[rng() % values.size()];
}

const std::vector<std::string> kElementNames = {
	"a",
	"b",
	"root",
	"item",
	"child",
	"ns:item",
	"xml:lang",
	"Xml",
	"A_1",
	"_private",
	"x:y:z",
	"data",
	"body",
	"html",
	"doc",
};

const std::vector<std::string> kBadNames = {
	"1a",
	"a b",
	"a<b",
	"",
	"a>b",
	"<x",
	"a\x01b",
	"a=b",
};

const std::vector<std::string> kAttributeNames = {
	"id",
	"class",
	"href",
	"xml:lang",
	"xmlns",
	"xmlns:x",
	"x:attr",
	"data-x",
	"a",
};

const std::vector<std::string> kAttributeValues = {
	"",
	"x",
	"a b",
	"&amp;",
	"<tag>",
	"&",
	"&#65;",
	"&#x41;",
	"\u00E9",
	"1&2",
	"&quot;",
	"\"",
};

const std::vector<std::string> kText = {
	"",
	" ",
	"hello",
	"a<b",
	"a>b",
	"&",
	"&&",
	"<",
	">",
	"&amp;",
	"&#x41;",
	"&#65;",
	"]]>",
	"\u00E9",
	"\t",
	"\n",
	"&unknown;",
	"&#0;",
	"&#x0;",
	"&#x110000;",
	"&#9999999;",
};

const std::vector<std::string> kCdata = { "", "text", "a<b", "]]>", "\u00E9", "&amp;" };

const std::vector<std::string> kComments = {
	"<!-- -->",
	"<!-- hi -->",
	"<!-- a<b -->",
	"<!---->",
	"<!--\u00E9-->",
};

const std::vector<std::string> kPIs = {
	"<?pi data?>",
	"<?pi?>",
	"<?xml-stylesheet href=\"a.xsl\"?>",
	"<?a b c?>",
};

const std::vector<std::string> kContentModels = {
	"EMPTY",
	"ANY",
	"(#PCDATA)",
	"(#PCDATA|a)*",
	"(a|b)*",
	"(a,b)+",
	"(a?,b*)",
	"#PCDATA",
	"(a)",
	"(#PCDATA|a|b)*",
};

const std::vector<std::string> kAttrTypes = {
	"CDATA",
	"ID",
	"IDREF",
	"NMTOKEN",
	"(a|b)",
};

const std::vector<std::string> kAttrDefaults = {
	"#IMPLIED",
	"#REQUIRED",
	"\"x\"",
	"'y'",
	"#FIXED \"z\"",
};

const std::vector<std::string> kEntities = {
	"<!ENTITY e \"text\">",
	"<!ENTITY e \"a&amp;b\">",
	"<!ENTITY % p \"value\">",
	"<!ENTITY e \"&#65;\">",
};

const std::vector<std::string> kSeeds = {
	"<root/>",
	"<root><item id=\"1\">text</item></root>",
	R"(<?xml version="1.0"?><root xmlns:x="http://example.com"><x:item>a &amp; b</x:item></root>)",
	"<!DOCTYPE root [<!ELEMENT root (#PCDATA)><!ENTITY e \"replacement\">]><root>&e;</root>",
	"<a><b><c><d>deep</d></c></b></a>",
};

std::string generate_element(rng_type &rng, const std::string &name, const std::vector<std::string> &names, int depth)
{
	std::string n = name;
	if (depth > 0 and (rng() % 15) == 0)
		n = pick(rng, kBadNames);

	std::string xml = "<" + n;

	int attrs = rng() % 4;
	for (int i = 0; i < attrs; ++i)
	{
		switch (rng() % 4)
		{
			case 0: xml += " " + pick(rng, kAttributeNames) + "=\"" + pick(rng, kAttributeValues) + "\""; break;
			case 1: xml += " " + pick(rng, kAttributeNames) + "='" + pick(rng, kAttributeValues) + "'"; break;
			case 2: xml += " " + pick(rng, kBadNames) + "=\"" + pick(rng, kAttributeValues) + "\""; break;
			case 3: xml += " " + pick(rng, kAttributeNames) + "=" + pick(rng, kAttributeValues); break;
		}
	}

	xml += ">";

	int children = rng() % 5;
	for (int i = 0; i < children and xml.size() < 4096; ++i)
	{
		switch (rng() % 7)
		{
			case 0: xml += pick(rng, kText); break;
			case 1: xml += "&" + pick(rng, names) + ";"; break;
			case 2:
				if (depth < 4)
					xml += generate_element(rng, pick(rng, names), names, depth + 1);
				else
					xml += pick(rng, kText);
				break;
			case 3: xml += "<![CDATA[" + pick(rng, kCdata) + "]]>"; break;
			case 4: xml += pick(rng, kComments); break;
			case 5: xml += pick(rng, kPIs); break;
			case 6: xml += "&#" + std::to_string(rng() % 200000) + ";"; break;
		}
	}

	if (depth > 0 and (rng() % 10) == 0)
		xml += "</" + pick(rng, kBadNames) + ">";
	else
		xml += "</" + n + ">";

	return xml;
}

std::string generate_doctype(rng_type &rng, const std::string &root, const std::vector<std::string> &names)
{
	std::string dtd = "<!DOCTYPE " + root + " [\n";

	int decls = 1 + rng() % 6;
	for (int i = 0; i < decls; ++i)
	{
		switch (rng() % 4)
		{
			case 0:
				dtd += "<!ELEMENT " + pick(rng, names) + " " + pick(rng, kContentModels) + ">\n";
				break;
			case 1:
				dtd += "<!ATTLIST " + pick(rng, names) + " " + pick(rng, kAttributeNames) + " " + pick(rng, kAttrTypes) + " " + pick(rng, kAttrDefaults) + ">\n";
				break;
			case 2:
				dtd += pick(rng, kEntities) + "\n";
				break;
			case 3:
				dtd += "<!ENTITY % pe" + std::to_string(i) + " \"x\">\n";
				dtd += "%pe" + std::to_string(i) + ";\n";
				break;
		}
	}

	dtd += "]>\n";
	return dtd;
}

std::string generate_document(rng_type &rng)
{
	std::vector<std::string> names;
	int n = 2 + rng() % 3;
	for (int i = 0; i < n; ++i)
		names.push_back(pick(rng, kElementNames));

	const std::string root = names[0];

	std::string xml;

	if ((rng() % 3) == 0)
		xml += "<?xml version=\"1.0\"?>\n";

	if ((rng() % 2) == 0)
		xml += generate_doctype(rng, root, names);

	xml += generate_element(rng, root, names, 0);

	return xml;
}

std::string generate_junk(rng_type &rng)
{
	size_t len = rng() % 256;
	std::string junk;
	junk.reserve(len);
	for (size_t i = 0; i < len; ++i)
	{
		switch (rng() % 5)
		{
			case 0: junk += char(rng() % 256); break;
			case 1: junk += char(0x80 + rng() % 128); break;
			case 2: junk += char(1 + rng() % 31); break;
			case 3: junk += "&<>\""; break;
			case 4: junk += char('a' + rng() % 26); break;
		}
	}

	if ((rng() % 2) == 0)
		junk = "<a>" + junk + "</a>";
	else if ((rng() % 3) == 0)
		junk = "<a " + junk + ">";

	return junk;
}

std::string mutate(rng_type &rng, std::string xml)
{
	int mutations = 1 + rng() % 4;
	for (int m = 0; m < mutations; ++m)
	{
		switch (rng() % 4)
		{
			case 0:
				if (not xml.empty())
					xml[rng() % xml.size()] = char(1 + rng() % 255);
				break;
			case 1:
				if (xml.size() > 2)
				{
					size_t pos = rng() % xml.size();
					size_t len = rng() % (xml.size() - pos);
					xml.erase(pos, len);
				}
				break;
			case 2:
				if (not xml.empty())
					xml.insert(rng() % xml.size(), pick(rng, kText));
				break;
			case 3:
				if (xml.size() > 2)
					xml.resize(rng() % xml.size());
				break;
		}
	}
	return xml;
}

void exercise(const std::string &xml)
{
	using namespace zeem;

	try
	{
		document doc(xml);

		std::ostringstream os;
		os << doc;
		document reparsed(os.str());

		for (element *e : doc.find("//*"))
			(void)e->attributes();
	}
	catch (const not_wf_exception &)
	{
	}
	catch (const invalid_exception &)
	{
	}
	catch (const exception &)
	{
	}

	if (xml.size() < 1024)
	{
		try
		{
			std::istringstream is(xml);
			document doc;
			doc.set_validating(true);
			is >> doc;
		}
		catch (const not_wf_exception &)
		{
		}
		catch (const invalid_exception &)
		{
		}
		catch (const exception &)
		{
		}

		try
		{
			std::istringstream is(xml);
			document doc;
			doc.set_validating_ns(true);
			is >> doc;
		}
		catch (const not_wf_exception &)
		{
		}
		catch (const invalid_exception &)
		{
		}
		catch (const exception &)
		{
		}

		try
		{
			std::istringstream is(xml);
			parser p(is);
			p.parse(true, true);
		}
		catch (const not_wf_exception &)
		{
		}
		catch (const invalid_exception &)
		{
		}
		catch (const exception &)
		{
		}
	}

	try
	{
		std::istringstream is(xml);
		document doc;
		doc.set_preserve_cdata(true);
		is >> doc;
	}
	catch (const not_wf_exception &)
	{
	}
	catch (const invalid_exception &)
	{
	}
	catch (const exception &)
	{
	}

	try
	{
		std::istringstream is(xml);
		parser p(is);
		p.parse(false, false);
	}
	catch (const not_wf_exception &)
	{
	}
	catch (const invalid_exception &)
	{
	}
	catch (const exception &)
	{
	}
}

} // namespace

TEST_CASE("fuzz-generated-documents")
{
	rng_type rng(0x5eed);

	for (size_t i = 0; i < 1000; ++i)
	{
		const std::string xml = generate_document(rng);
		CAPTURE(i, xml);
		exercise(xml);
	}
}

TEST_CASE("fuzz-mutated-documents")
{
	rng_type rng(0x5eed);

	for (size_t i = 0; i < 1000; ++i)
	{
		const std::string xml = mutate(rng, pick(rng, kSeeds));
		CAPTURE(i, xml);
		exercise(xml);
	}
}

TEST_CASE("fuzz-junk-inputs")
{
	rng_type rng(0x5eed);

	for (size_t i = 0; i < 1000; ++i)
	{
		const std::string xml = generate_junk(rng);
		CAPTURE(i, xml);
		exercise(xml);
	}
}
