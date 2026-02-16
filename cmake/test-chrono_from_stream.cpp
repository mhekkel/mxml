#include <chrono>

int main()
{
	using namespace std::chrono_literals;

	int result = 0;

	std::stringstream is{ "01-01-2025T00:00:01" };
	std::chrono::time_point<std::chrono::system_clock> t{};
	std::chrono::from_stream(is, "%d-%m-%YT%T", t);

	if (is.bad() or is.fail() or t != std::chrono::sys_days{ 2025y / 1 / 1 } + 1s)
		result = -1;

	return result;
}