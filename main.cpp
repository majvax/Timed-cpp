#include "timer.hpp"

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


int main()
{
    {
        Timer::AverageTimer<10>({"foo"}, foo, 1, 2);
    }

    {
        Timer::Timer<std::chrono::seconds>({"fibonacci"}, fibonacci, 41);
    }


    return 0;
}
