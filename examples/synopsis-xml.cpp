/*-
 * SPDX-License-Identifier: BSD-2-Clause
 * 
 * Copyright (c) 2024 Maarten L. Hekkelman
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

//[ synopsis_xml_main
#include "mxml.hpp"

#include <iostream>

int main()
{
    using namespace mxml::literals; 

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
    for (auto person: doc.find("//person")) 
    {
        std::string firstname, lastname;

        /* Iterate over the __element__ nodes inside the person __element__ */
        for (auto name: *person)
        {
            if (name.name() == "firstname")	firstname = name.str();
            if (name.name() == "lastname")	lastname = name.str();
        }

        std::cout << person->get_attribute("id") << ": " << lastname << ", " << firstname << '\n';
    }

    return 0;
}
//]