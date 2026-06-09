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

#include <iostream>
#include <string>

#if ZEEM_CXX_MODULE
import zeem;
#else
#include "zeem/zeem.hpp"
#endif

//[ xpath_sample
int main()
{
	using namespace zeem::literals;

	auto doc = R"(<bar xmlns:z="https://www.hekkelman.com/zeem">
        <z:foo>foei</z:foo>
    </bar>)"_xml;

	/* Create an xpath context and store our variable */
	zeem::context ctx;
	ctx.set("ns", "https://www.hekkelman.com/zeem");

	/* Create an xpath object with the specified XPath using the variable `ns` */
	auto xp = zeem::xpath("//*[namespace-uri() = $ns]");

	/* Iterate over the result of the evaluation of this XPath, the result will consist of zeem::element object pointers */
	for (auto n : xp.evaluate<zeem::element>(doc, ctx))
		std::cout << n->str() << '\n';

	return 0;
}
//]