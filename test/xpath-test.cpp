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

#include "zeem.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

namespace fs = std::filesystem;

int VERBOSE;

ostream &operator<<(ostream &os, const zeem::node &n)
{
	n.write(os, {});
	return os;
}

bool run_test(const zeem::element &test)
{
	if (VERBOSE)
	{
		cout << "----------------------------------------------------------" << endl
			 << "ID: " << test.get_attribute("ID")
			 << endl
			 << "xpath: " << test.get_attribute("xpath") << endl
			 //		 << "data: " << test.content() << endl
		     //		 << "expected-size: " << test.attr("expected-size") << endl
			 << endl;
	}

	fs::path data_file = fs::current_path() / test.get_attribute("data");
	if (not fs::exists(data_file))
		throw zeem::exception("file does not exist");

	std::ifstream file(data_file, ios::binary);

	zeem::document doc;
	file >> doc;

	if (VERBOSE)
		cout << "test doc:" << endl
			 << doc << endl;

	zeem::xpath xp(test.get_attribute("xpath"));

	zeem::context context;
	for (const zeem::element *e : test.find("var"))
		context.set(e->get_attribute("name"), e->get_attribute("value"));

	auto ns = xp.evaluate<zeem::node>(*doc.root(), context);

	if (VERBOSE)
	{
		int nr = 1;
		for (const zeem::node *n : ns)
			cout << nr++ << ">> " << *n << endl;
	}

	bool result = true;

	if (ns.size() != std::stoul(test.get_attribute("expected-size")))
	{
		cout << "incorrect number of nodes in returned node-set" << endl
			 << "expected: " << test.get_attribute("expected-size") << endl;

		result = false;
	}

	string test_attr_name = test.get_attribute("test-name");
	string attr_test = test.get_attribute("test-attr");

	if (not attr_test.empty())
	{
		if (VERBOSE)
			cout << "testing attribute " << test_attr_name << " for " << attr_test << endl;

		for (const zeem::node *n : ns)
		{
			const auto *e = dynamic_cast<const zeem::element *>(n);
			if (e == nullptr)
				continue;

			if (e->get_attribute(test_attr_name) != attr_test)
			{
				cout << "expected attribute content is not found for node " << e->get_qname() << endl;
				result = false;
			}
		}
	}

	if (VERBOSE)
	{
		if (result)
			cout << "Test passed" << endl;
		else
		{
			cout << "Test failed" << endl;

			int nr = 1;
			for (const zeem::node *n : ns)
				cout << nr++ << ") " << *n << endl;
		}
	}

	return result;
}

void run_tests(const fs::path &file)
{
	if (not fs::exists(file))
		throw zeem::exception("test file does not exist");

	std::ifstream input(file, ios::binary);

	zeem::document doc;
	input >> doc;

	fs::path dir = fs::absolute(file).parent_path().parent_path();
	if (not dir.empty())
		fs::current_path(dir);

	string base = doc.front().get_attribute("xml:base");
	if (not base.empty())
		fs::current_path(base);

	int nr_of_tests = 0, failed_nr_of_tests = 0;

	for (const zeem::element *test : doc.find("//xpath-test"))
	{
		++nr_of_tests;
		if (run_test(*test) == false)
			++failed_nr_of_tests;
	}

	cout << endl;
	if (failed_nr_of_tests == 0)
		cout << "*** No errors detected" << endl;
	else
	{
		cout << failed_nr_of_tests << " out of " << nr_of_tests << " failed" << endl;
		if (not VERBOSE)
			cout << "Run with --verbose to see the errors" << endl;
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
		cout << "exception: " << e.what() << endl;
		return 1;
	}

	return 0;
}
