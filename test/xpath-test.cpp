// Copyright (c) 2026 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

#define CATCH_CONFIG_RUNNER

#if ZEEM_USE_DATE_H
# include <date/tz.h>
#endif

#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#if ZEEM_CXX_MODULE
import zeem;
#else
# include "zeem/zeem.hpp"
#endif

namespace fs = std::filesystem;

int VERBOSE;

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

std::ostream &operator<<(std::ostream &os, const zeem::node &n)
{
	n.write(os, {});
	return os;
}

bool run_test(const zeem::element &test)
{
	if (VERBOSE)
	{
		std::cout << "----------------------------------------------------------\n"
				  << "ID: " << test.get_attribute("ID")
				  << '\n'
				  << "xpath: " << test.get_attribute("xpath") << '\n'
				  //		 << "data: " << test.content() << '\n'
		          //		 << "expected-size: " << test.attr("expected-size") << '\n'
				  << '\n';
	}

	fs::path data_file = fs::current_path() / test.get_attribute("data");
	if (not fs::exists(data_file))
		throw zeem::exception("file does not exist");

	std::ifstream file(data_file, std::ios::binary);

	zeem::document doc;
	file >> doc;

	if (VERBOSE)
		std::cout << "test doc:\n"
				  << doc << '\n';

	zeem::xpath xp(test.get_attribute("xpath"));

	zeem::context context;
	for (const zeem::element *e : test.find("var"))
		context.set(e->get_attribute("name"), e->get_attribute("value"));

	auto ns = xp.evaluate<zeem::node>(*doc.root(), context);

	if (VERBOSE)
	{
		int nr = 1;
		for (const zeem::node *n : ns)
			std::cout << nr++ << ">> " << *n << '\n';
	}

	bool result = true;

	if (ns.size() != std::stoul(test.get_attribute("expected-size")))
	{
		std::cout << "incorrect number of nodes in returned node-set\n"
				  << "expected: " << test.get_attribute("expected-size") << " found: " << ns.size() << '\n';

		result = false;
	}

	std::string test_attr_name = test.get_attribute("test-name");
	std::string attr_test = test.get_attribute("test-attr");

	if (not attr_test.empty())
	{
		if (VERBOSE)
			std::cout << "testing attribute " << test_attr_name << " for " << attr_test << '\n';

		for (const zeem::node *n : ns)
		{
			const auto *e = dynamic_cast<const zeem::element *>(n);
			if (e == nullptr)
				continue;

			if (e->get_attribute(test_attr_name) != attr_test)
			{
				std::cout << "expected attribute content is not found for node " << e->get_qname() << '\n';
				result = false;
			}
		}
	}

	if (VERBOSE)
	{
		if (result)
			std::cout << "Test passed\n";
		else
		{
			std::cout << "Test failed\n";

			int nr = 1;
			for (const zeem::node *n : ns)
				std::cout << nr++ << ") " << *n << '\n';
		}
	}

	return result;
}

TEST_CASE("xpath")
{
	using namespace std::literals;
	auto xmlconfFile = gTestDir / "XPath-Test-Suite" / "xpath-tests.xml";

	REQUIRE(fs::exists(xmlconfFile));

	std::ifstream input(xmlconfFile, std::ios::binary);
	zeem::document confDoc;
	input >> confDoc;

	fs::current_path(gTestDir);
	std::string base = confDoc.front().get_attribute("xml:base");
	if (not base.empty())
		fs::current_path(base);

	for (const zeem::element *test : confDoc.find("//xpath-test"))
	{
		fs::path data_file = fs::current_path() / test->get_attribute("data");
		REQUIRE(fs::exists(data_file));

		std::ifstream file(data_file, std::ios::binary);

		zeem::document doc;
		file >> doc;

		zeem::xpath xp(test->get_attribute("xpath"));

		zeem::context context;
		for (const zeem::element *e : test->find("var"))
			context.set(e->get_attribute("name"), e->get_attribute("value"));

		auto ns = xp.evaluate<zeem::node>(*doc.root(), context);

		uint32_t expectedSize = std::stoul(test->get_attribute("expected-size"));

		CHECK(ns.size() == expectedSize);
		if (ns.size() != expectedSize)
			std::cout << "Failed test :\n" << *test << '\n';

		std::string test_attr_name = test->get_attribute("test-name");
		std::string attr_test = test->get_attribute("test-attr");

		if (not attr_test.empty())
		{
			for (const zeem::node *n : ns)
			{
				const auto *e = dynamic_cast<const zeem::element *>(n);
				if (e == nullptr)
					continue;

				CHECK(e->get_attribute(test_attr_name) == attr_test);
			}
		}
	}
}