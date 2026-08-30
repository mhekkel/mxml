// Copyright (c) 2026 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

#ifndef ZEEM_CXX_MODULE
# include "zeem/export.hpp"
# include "fast_float/fast_float.h"
# include "zeem/export.hpp"
# include "zeem/zeem.hpp"
#endif

namespace zeem
{

template <>
std::from_chars_result ff_charconv<float>::from_chars(const char *a, const char *b, float &v)
{
	auto r = fast_float::from_chars(a, b, v);
	return { r.ptr, r.ec };
}

template <>
std::from_chars_result ff_charconv<double>::from_chars(const char *a, const char *b, double &v)
{
	auto r = fast_float::from_chars(a, b, v);
	return { r.ptr, r.ec };
}

} // namespace zeem
