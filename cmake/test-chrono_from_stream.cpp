#include <chrono>
#include <iostream>

int main()
{
    std::chrono::time_point<std::chrono::system_clock> t;

    std::chrono::from_stream(std::cin, "%F", t);

    return 0;
}