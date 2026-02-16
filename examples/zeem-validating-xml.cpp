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

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

int main()
{
	//[ xml_validation_sample
	/* Define an entity loader function */
	auto loader = [](std::string_view base, std::string_view pubid, std::string_view sysid)
	{
		if (base == "." and pubid.empty() and fs::exists(sysid))
			return std::make_unique<std::ifstream>(std::string{ sysid });

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
