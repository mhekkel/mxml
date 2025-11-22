#include <chrono>

int main()
{
	std::chrono::time_point<std::chrono::system_clock> t;

	// std::chrono::from_stream(std::cin, "%F", t);

	for (size_t ix = 0; const char *s : {
							"01-01-2025T00:00:00.00001Z",
							"01-01-2025T00:00:00.00001+01:00",
							"01-01-2025T00:00:00.00001" })
	{
		std::stringstream is{ s };

		switch (ix)
		{
			case 0:
				std::chrono::from_stream(is, "%FT%TZ", t);
				break;

			case 1:
				std::chrono::from_stream(is, "%FT%T%0z", t);
				break;

			case 2:
				std::chrono::from_stream(is, "%FT%T", t);
				break;
		}

		if (is.bad() or is.fail())
			exit(-1);
	}

	return 0;
}