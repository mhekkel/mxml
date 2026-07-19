// SPDX-FileCopyrightText: 2026 Maarten L. Hekkelman
// SPDX-License-Identifier: BSD-2-Clause

#include <cassert>
#include <chrono>

int main()
{
	std::istringstream is{ "2026-04-27T07:57" };

	std::chrono::time_point<std::chrono::system_clock> t1;
	std::chrono::from_stream(is, "%FT%H:%M", t1);

	using namespace std::chrono_literals;

	std::chrono::time_point<std::chrono::system_clock> t2;
	t2 = std::chrono::sys_days{2026y / 04 / 27} + 7h + 57min;
	assert(t1 == t2);

	auto info = std::chrono::current_zone()->get_info(t1);
	t1 -= info.offset;
	
	return 0;
}