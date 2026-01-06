//           Copyright Maarten L. Hekkelman, 2022-2023
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#include "zeem.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

int main()
{
	//[ xml_validation_sample
	/* Define an entity loader function */
	auto loader = [](std::string_view base, std::string_view pubid, std::string_view sysid) -> std::istream *
	{
		if (base == "." and pubid.empty() and fs::exists(sysid))
			return new std::ifstream(std::string{ sysid });

		throw std::invalid_argument("Invalid arguments passed in loader");
	};

	/* Create document and set the entity loader */
	zeem::document doc;
	doc.set_entity_loader(loader);

	/* Read a file */
	std::ifstream is("sample.xml");
	is >> doc;

	using namespace zeem::literals;

	/* Compare the doc with an in-memory constructed document, note that spaces are ignored */
	if (doc == R"(<foo><bar>Hello, world!</bar></foo>)"_xml)
		std::cout << "ok\n";
	//]

	return 0;
}
