// Copyright (c) 2024 Maarten L. Hekkelman
//
// SPDX-License-Identifier: BSD-2-Clause

//[ synopsis_xml_main

#include <exception>
#include <iostream>
#include <string>

#if ZEEM_CXX_MODULE
import zeem;
#else
#include "zeem/zeem.hpp"
#endif

int main()
{
	using namespace zeem::literals;

	try
	{

		/* Construct an XML document in memory using a string literal */
		auto doc =
			R"(<persons>
            <person id="1">
                <firstname>John</firstname>
                <lastname>Doe</lastname>
            </person>
            <person id="2">
                <firstname>Jane</firstname>
                <lastname>Jones</lastname>
            </person>
        </persons>)"_xml;

		/* Iterate over an XPath result set */
		for (auto person : doc.find("//person"))
		{
			std::string firstname, lastname;

			/* Iterate over the __element__ nodes inside the person __element__ */
			for (const auto& name : *person)
			{
				if (name.name() == "firstname")
					firstname = name.str();
				if (name.name() == "lastname")
					lastname = name.str();
			}

			std::cout << person->get_attribute("id") << ": " << lastname << ", " << firstname << '\n';
		}
	}
	catch (const std::exception &ex)
	{
		std::cerr << "Error in running test: " << ex.what() << "\n";
	}

	return 0;
}
//]