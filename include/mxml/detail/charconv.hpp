//          Copyright Maarten L. Hekkelman 2025
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "fast_float/fast_float.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <string>
#include <vector>

#if __has_include(<experimental/type_traits>)
# include <experimental/type_traits>
#else
# include <type_traits>
#endif

namespace mxml::detail
{

#if (not defined(__cpp_lib_experimental_detect) or (__cpp_lib_experimental_detect < 201505)) and (not defined(_LIBCPP_VERSION) or _LIBCPP_VERSION < 5000)
// This code is copied from:
// https://ld2015.scusa.lsu.edu/cppreference/en/cpp/experimental/is_detected.html

template <class...>
using void_t = void;

namespace detail
{
	template <class Default, class AlwaysVoid,
		template <class...> class Op, class... Args>
	struct detector
	{
		using value_t = std::false_type;
		using type = Default;
	};

	template <class Default, template <class...> class Op, class... Args>
	struct detector<Default, void_t<Op<Args...>>, Op, Args...>
	{
		// Note that std::void_t is a c++17 feature
		using value_t = std::true_type;
		using type = Op<Args...>;
	};
} // namespace detail

struct nonesuch
{
	nonesuch() = delete;
	~nonesuch() = delete;
	nonesuch(nonesuch const &) = delete;
	void operator=(nonesuch const &) = delete;
};

template <template <class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

template <template <class...> class Op, class... Args>
constexpr inline bool is_detected_v = is_detected<Op, Args...>::value;

template <template <class...> class Op, class... Args>
using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

template <class Default, template <class...> class Op, class... Args>
using detected_or = detail::detector<Default, void, Op, Args...>;

template <class Expected, template <class...> class Op, class... Args>
using is_detected_exact = std::is_same<Expected, detected_t<Op, Args...>>;

template <class Expected, template <class...> class Op, class... Args>
constexpr inline bool is_detected_exact_v = is_detected_exact<Expected, Op, Args...>::value;

#else

template <template <class...> class Op, class... Args>
constexpr inline bool is_detected_v = std::experimental::is_detected<Op, Args...>::value;

#endif

template <typename T>
struct my_charconv
{
	using value_type = T;

	static auto from_chars(const char *first, const char *last, value_type &value)
	{
		return fast_float::from_chars(first, last, value);
	}
};

template <typename T>
struct std_charconv
{
	static std::from_chars_result from_chars(const char *a, const char *b, T &d)
	{
		return std::from_chars(a, b, d);
	}
};

template <typename T>
using from_chars_function = decltype(std::from_chars(std::declval<const char *>(), std::declval<const char *>(), std::declval<T &>()));

template <typename T>
using charconv = typename std::conditional_t<is_detected_v<from_chars_function, T>, std_charconv<T>, my_charconv<T>>;

template <typename T>
constexpr auto from_chars(const char *s, const char *e, T &v)
{
	return charconv<T>::from_chars(s, e, v);
}

} // namespace mxml::detail