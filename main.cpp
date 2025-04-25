#include "timer.hpp"
#include <thread>


int foo(int a, int b)
{
    int result = 0;
    for (int i = 0; i < 100000000; ++i) {
        result += a + b;
    }
    return result;
}

int fibonacci(int n)
{
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}


int some_function_that_takes_a_while(int a, int b)
{
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return a + b;
}


int main()
{
    {
        Timer::AverageTimer<10>({{"foo"}}, foo, 1, 2);
    }

    {
        Timer::Timer<std::chrono::seconds>({"fibonacci"}, fibonacci, 41);
    }

    auto timer = Timer::BlockTimer({"some_function_that_takes_a_while"});

    some_function_that_takes_a_while(1, 2);

    timer.end_and_show_result();



    return 0;
}
