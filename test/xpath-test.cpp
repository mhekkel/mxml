// Copyright (c) 2026 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

#include <exception>
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

void run_tests(const fs::path &file)
{
	if (not fs::exists(file))
	{
		std::cerr << "test file does not exist\n";
		exit(1);
	}

	std::ifstream input(file, std::ios::binary);

	zeem::document doc;
	input >> doc;

	fs::path dir = fs::absolute(file).parent_path().parent_path();
	if (not dir.empty())
		fs::current_path(dir);

	std::string base = doc.front().get_attribute("xml:base");
	if (not base.empty())
		fs::current_path(base);

	int nr_of_tests = 0, failed_nr_of_tests = 0;

	for (const zeem::element *test : doc.find("//xpath-test"))
	{
		++nr_of_tests;
		if (run_test(*test) == false)
			++failed_nr_of_tests;
	}

	std::cout << '\n';
	if (failed_nr_of_tests == 0)
		std::cout << "*** No errors detected\n";
	else
	{
		std::cout << failed_nr_of_tests << " out of " << nr_of_tests << " failed\n";
		if (not VERBOSE)
			std::cout << "Run with --verbose to see the errors\n";
	}
}

int main(int argc, char *argv[])
{
	using namespace std::literals;
	fs::path xmlconfFile("XPath-Test-Suite/xpath-tests.xml");

	for (int i = 1; i < argc; ++i)
	{
		if (argv[i] == "-v"s)
		{
			++VERBOSE;
			continue;
		}

		xmlconfFile = argv[i];
	}

	try
	{
		run_tests(xmlconfFile);
	}
	catch (std::exception &e)
	{
		std::cout << "exception: " << e.what() << '\n';
		return 1;
	}

	return 0;
}
