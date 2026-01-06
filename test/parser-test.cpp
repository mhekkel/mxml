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

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <mcfp/detail/charconv.hpp>
#include <mcfp/mcfp.hpp>
#include <ranges>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#if defined(_WIN32)
# include <conio.h>
# include <ctype.h>
#endif

namespace fs = std::filesystem;

int VERBOSE;
int TRACE;
int error_tests, should_have_failed, total_tests, wrong_exception, skipped_tests;

bool run_valid_test(std::istream &is, fs::path &outfile)
{
	bool result = true;

	zeem::document indoc;
	is >> indoc;

	std::stringstream s;
	indoc.set_collapse_empty_tags(false);
	indoc.set_suppress_comments(true);
	indoc.set_escape_white_space(true);
	indoc.set_wrap_prolog(false);
	s << indoc;

	std::string s1 = s.str();
	zeem::trim(s1);

	if (TRACE)
		std::cout << s1 << '\n';

	if (fs::is_directory(outfile))
		;
	else if (fs::exists(outfile))
	{
		std::ifstream out(outfile, std::ios::binary);
		std::string s2, line;
		while (not out.eof())
		{
			getline(out, line);
			s2 += line + "\n";
		}
		zeem::trim(s2);

		if (s1 != s2)
		{
			std::stringstream ss;
			ss << "output differs: \n"
			   << '\n'
			   << s1 << '\n'
			   << '\n'
			   << s2 << '\n'
			   << '\n';

			throw zeem::exception(ss.str());
		}
	}
	else
		std::cout << "skipped output compare for " << outfile << '\n';

	return result;
}

void dump(zeem::element &e, int level = 0)
{
	std::cout << level << "> " << e.get_qname() << '\n';
	for (auto &[name, ign] : e.attributes())
		std::cout << level << " (a)> " << name << '\n';
	for (auto &c : e)
		dump(c, level + 1);
}

bool run_test(const zeem::element &test, fs::path base_dir)
{
	bool result = true;

	fs::path input(base_dir / test.get_attribute("URI"));
	fs::path output(base_dir / test.get_attribute("OUTPUT"));

	++total_tests;

	if (not fs::exists(input))
	{
		std::cout << "test file " << input << " does not exist\n";
		return false;
	}

	// if (test.attr("SECTIONS") == "B.")
	// {
	// 	if (VERBOSE)
	// 		cout << "skipping unicode character validation tests\n";
	// 	++skipped_tests;
	// 	return true;
	// }

	fs::current_path(input.parent_path());

	std::ifstream is(input, std::ios::binary);
	if (not is.is_open())
		throw zeem::exception("test file not open");

	std::string error;

	try
	{
		fs::current_path(input.parent_path());

		if (test.get_attribute("TYPE") == "valid")
			result = run_valid_test(is, output);
		else if (test.get_attribute("TYPE") == "not-wf" or test.get_attribute("TYPE") == "invalid")
		{
			bool failed = false;
			try
			{
				zeem::document doc;
				doc.set_validating(test.get_attribute("TYPE") == "invalid");
				doc.set_validating_ns(test.get_attribute("RECOMMENDATION") == "NS1.0");
				is >> doc;
				++should_have_failed;
				result = false;
			}
			catch (zeem::not_wf_exception &e)
			{
				if (test.get_attribute("TYPE") != "not-wf")
				{
					++wrong_exception;
					throw zeem::exception(std::string("Wrong exception (should have been invalid):\n\t") + e.what());
				}

				failed = true;
				if (VERBOSE > 1)
					std::cout << e.what() << '\n';
			}
			catch (zeem::invalid_exception &e)
			{
				if (test.get_attribute("TYPE") != "invalid")
				{
					++wrong_exception;
					throw zeem::exception(std::string("Wrong exception (should have been not-wf):\n\t") + e.what());
				}

				failed = true;
				if (VERBOSE > 1)
					std::cout << e.what() << '\n';
			}
			catch (std::exception &e)
			{
				throw zeem::exception(std::string("Wrong exception:\n\t") + e.what());
			}

			if (VERBOSE and not failed)
				throw zeem::exception("invalid document, should have failed");
		}
		else
		{
			bool failed = false;
			try
			{
				zeem::document doc;
				is >> doc;
				++should_have_failed;
				result = false;
			}
			catch (std::exception &e)
			{
				if (VERBOSE > 1)
					std::cout << e.what() << '\n';

				failed = true;
			}

			if (VERBOSE and not failed)
			{
				if (test.get_attribute("TYPE") == "not-wf")
					throw zeem::exception("document should have been not well formed");
				else // or test.attr("TYPE") == "error"
					throw zeem::exception("document should have been invalid");
			}
		}
	}
	catch (std::exception &e)
	{
		if (test.get_attribute("TYPE") == "valid")
			++error_tests;
		result = false;
		error = e.what();
	}

	if ((result == false and VERBOSE == 1) or (VERBOSE > 1))
	{
		std::cout << "-----------------------------------------------\n"
				  << "ID:             " << test.get_attribute("ID") << '\n'
				  << "FILE:           " << /*fs::system_complete*/ (input) << '\n'
				  << "TYPE:           " << test.get_attribute("TYPE") << '\n'
				  << "SECTION:        " << test.get_attribute("SECTIONS") << '\n'
				  << "EDITION:        " << test.get_attribute("EDITION") << '\n'
				  << "RECOMMENDATION: " << test.get_attribute("RECOMMENDATION") << '\n';

		std::istringstream s(test.get_content());
		for (;;)
		{
			std::string line;
			getline(s, line);

			zeem::trim(line);

			if (line.empty())
			{
				if (s.eof())
					break;
				continue;
			}

			std::cout << "DESCR:          " << line << '\n';
		}

		std::cout << '\n';

		if (result == false)
		{
			std::istringstream iss(error);
			for (;;)
			{
				std::string line;
				getline(iss, line);

				zeem::trim(line);

				if (line.empty() and iss.eof())
					break;

				std::cout << "  " << line << '\n';
			}

			std::cout << '\n';

			//			cout << "exception: " << error << '\n';
		}
	}

	return result;
}

void run_test_case(const zeem::element &testcase, const std::string &id, const std::set<std::string> &skip,
	const std::string &type, int edition, fs::path base_dir, std::vector<std::string> &failed_ids)
{
	if (VERBOSE > 1 and id.empty())
		std::cout << "Running testcase " << testcase.get_attribute("PROFILE") << '\n';

	if (not testcase.get_attribute("xml:base").empty())
	{
		base_dir /= testcase.get_attribute("xml:base");

		if (fs::exists(base_dir))
			fs::current_path(base_dir);
	}

	std::string path;
	if (id.empty())
		path = ".//TEST";
	else
		path = std::string(".//TEST[@ID='") + id + "']";

	std::regex ws_re(" "); // whitespace

	for (const zeem::element *n : zeem::xpath(path).evaluate<zeem::element>(testcase))
	{
		auto testID = n->get_attribute("ID");
		if (skip.count(testID))
			continue;

		if (not id.empty() and testID != id)
			continue;

		if (not type.empty() and type != n->get_attribute("TYPE"))
			continue;

		if (edition != 0)
		{
			auto es = n->get_attribute("EDITION");
			if (not es.empty())
			{
				auto b = std::sregex_token_iterator(es.begin(), es.end(), ws_re, -1);
				auto e = std::sregex_token_iterator();
				auto ei = find_if(b, e, [edition](const std::string &e)
					{ return stoi(e) == edition; });

				if (ei == e)
					continue;
			}
		}

		if (fs::exists(base_dir / n->get_attribute("URI")) and
			not run_test(*n, base_dir))
		{
			failed_ids.push_back(n->get_attribute("ID"));
		}
	}
}

void test_testcases(const fs::path &testFile, const std::string &id, const std::set<std::string> &skip,
	const std::string &type, int edition, std::vector<std::string> &failed_ids)
{
	std::ifstream file(testFile, std::ios::binary);

	int saved_verbose = VERBOSE;
	VERBOSE = 0;

	int saved_trace = TRACE;
	TRACE = 0;

	fs::path base_dir = fs::weakly_canonical(testFile.parent_path());
	fs::current_path(base_dir);

	zeem::document doc(file);

	VERBOSE = saved_verbose;
	TRACE = saved_trace;

	for (auto test : doc.find("//TESTCASES"))
	{
		if (test->get_qname() != "TESTCASES")
			continue;
		run_test_case(*test, id, skip, type, edition, base_dir, failed_ids);
	}
}

int main(int argc, char *argv[])
{
	int result = 0;

	auto &config = mcfp::config::instance();

	config.init(
		"usage: parser-test [options]",
		mcfp::make_option("help,h", "produce help message"),
		mcfp::make_option("verbose,v", "verbose output"),
		mcfp::make_option<std::string>("id", "ID for the test to run from the test suite"),
		mcfp::make_option<std::vector<std::string>>("skip", "Skip this test, can be specified multiple times"),
		mcfp::make_option<std::vector<std::string>>("questionable", "Questionable tests, do not consider failure of these to be an error"),
		mcfp::make_option<int>("edition", "XML 1.0 specification edition to test, default is 5, 0 which means run all tests"),
		mcfp::make_option("trace", "Trace productions in parser"),
		mcfp::make_option<std::string>("type", "Type of test to run (valid|not-wf|invalid|error)"),
		mcfp::make_option<std::string>("single", "Test a single XML file"),
		mcfp::make_option<std::string>("dump", "Dump the structure of a single XML file"),
		mcfp::make_option("print-ids", "Print the ID's of failed tests"),
		mcfp::make_option<std::string>("conf", "Configuration file"));

	std::error_code ec;
	config.parse(argc, argv, ec);
	if (ec)
	{
		std::clog << "error parsing arguments: " << ec.message() << '\n';
		exit(1);
	}

	if (config.count("help"))
	{
		std::cout << config << '\n';
		return 1;
	}

	VERBOSE = config.count("verbose");
	TRACE = config.count("trace");

	fs::path savedwd = fs::current_path();

	try
	{
		if (config.count("single"))
		{
			fs::path path(config.get("single"));

			std::ifstream file(path, std::ios::binary);
			if (not file.is_open())
				throw zeem::exception("could not open file");

			fs::path dir(path.parent_path());
			fs::current_path(dir);

			run_valid_test(file, dir);
		}
		else if (config.count("dump"))
		{
			fs::path path(config.get("dump"));

			std::ifstream file(path, std::ios::binary);
			if (not file.is_open())
				throw zeem::exception("could not open file");

			fs::path dir(path.parent_path());
			fs::current_path(dir);

			zeem::document doc;
			file >> doc;
			dump(doc.front());
		}
		else
		{
			fs::path xmlconfFile("XML-Test-Suite/xmlconf/xmlconf.xml");
			if (config.operands().size() == 1)
				xmlconfFile = config.operands().front();

			if (not fs::exists(xmlconfFile))
				throw std::runtime_error("Config file not found: " + xmlconfFile.string());

			std::string id;
			if (config.has("id"))
				id = config.get("id");

			std::vector<std::string> skip;
			if (config.has("skip"))
				skip = config.get<std::vector<std::string>>("skip");

			std::string type;
			if (config.count("type"))
				type = config.get("type");

			int edition = 5;
			if (config.count("edition"))
				edition = config.get<int>("edition");

			std::vector<std::string> failed_ids;

			test_testcases(xmlconfFile, id, { skip.begin(), skip.end() }, type, edition, failed_ids);

			std::cout << '\n'
					  << "summary: \n"
					  << "  ran " << total_tests - skipped_tests << " out of " << total_tests << " tests\n"
					  << "  " << error_tests << " threw an exception\n"
					  << "  " << wrong_exception << " wrong exception\n"
					  << "  " << should_have_failed << " should have failed but didn't\n";

			std::vector<std::string> questionable;
			if (config.count("questionable"))
				questionable = config.get<std::vector<std::string>>("questionable");

			std::set<std::string> erronous;
			for (auto fid : failed_ids)
			{
				if (std::ranges::find(questionable, fid) == questionable.end())
					erronous.insert(fid);
			}

			if (not erronous.empty())
				result = 1;

			if (config.count("print-ids") and not failed_ids.empty())
			{
				std::cout << '\n';
				if (erronous.empty())
					std::cout << "All the failed tests were questionable\n";
				else
				{
					std::cout << '\n'
							  << "ID's for the failed, non-questionable tests: \n";

					std::ranges::copy(erronous, std::ostream_iterator<std::string>(std::cout, "\n"));

					std::cout << '\n';
				}
			}
		}
	}
	catch (std::exception &e)
	{
		std::cout << e.what() << '\n';
		return 1;
	}

	fs::current_path(savedwd);

	return result;
}
