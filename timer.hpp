#include <iostream>
#include <string_view>
#include <string>
#include <chrono>
#include <array>
#include <numeric>


using namespace std::chrono_literals;


namespace Timer
{
    struct automatic_duration {};

    template <typename duration>
    struct duration_string {
        static constexpr std::string_view value = "unknown";
        static constexpr std::string_view to_string() noexcept { return value; }
    };

    template <>
    struct duration_string<std::chrono::nanoseconds> {
        static constexpr std::string_view value = "ns";
    };

    template <>
    struct duration_string<std::chrono::microseconds> {
        static constexpr std::string_view value = "us";
    };

    template <>
    struct duration_string<std::chrono::milliseconds> {
        static constexpr std::string_view value = "ms";
    };

    template <>
    struct duration_string<std::chrono::seconds> {
        static constexpr std::string_view value = "s";
    };

    template <>
    struct duration_string<std::chrono::minutes> {
        static constexpr std::string_view value = "m";
    };

    template <>
    struct duration_string<std::chrono::hours> {
        static constexpr std::string_view value = "h";
    };

    template <>
    struct duration_string<automatic_duration> {
        static const std::string to_string(int64_t elapsed) noexcept {
            if (elapsed < (1us).count()) {
                return std::to_string(elapsed) + " ns";
            } else if (elapsed < (1ms).count()) {
                return std::to_string(elapsed / 1000.0) + " us";
            } else if (elapsed < (1s).count()) {
                return std::to_string(elapsed / 1'000'000.0) + " ms";
            } else if (elapsed < (1min).count()) {
                return std::to_string(elapsed / 1'000'000'000.0) + " s";
            } else if (elapsed < (1h).count()) {
                return std::to_string(elapsed / 60'000'000'000.0) + " m";
            } else {
                return std::to_string(elapsed / 3'600'000'000'000.0) + " h";
            }
        }

    };





    template <typename duration = automatic_duration, typename clock = std::chrono::steady_clock>
    class Timer 
    {
    public:
        struct Settings
        {
            std::string_view name;
            bool show_output = true;
        };
    
    private:
        clock::time_point start;
        clock::time_point end;
        const Settings settings;

    public:
        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;
        Timer(Timer&&) = delete;
        Timer& operator=(Timer&&) = delete;
    
        template <typename F, typename... Args>
        inline Timer(Settings settings, F&& function, Args&&... args) noexcept(std::) : start(clock::now()), settings(settings)
        {
            std::forward<F>(function)(std::forward<Args>(args)...);
            end = clock::now();
        }

        inline ~Timer() noexcept
        {
            if (!settings.show_output) return;

            if constexpr (std::is_same_v<duration, automatic_duration>) {
                auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                std::cout << '[' << settings.name << ']' <<  " took: " << duration_string<automatic_duration>::to_string(elapsed) << " to run" << std::endl;

            } else {
                auto elapsed = std::chrono::duration_cast<duration>(end - start).count();
                std::cout << '[' << settings.name << ']' <<  " took: " << elapsed  << ' ' << duration_string<duration>::value << " to run" << std::endl;
            }
        }


        [[nodiscard]] auto get_elapsed(void) const noexcept
        { 
            return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        }


        template <typename F, typename... Args>
        void _prepare_and_run(Settings settings, F&& function, Args&&... args) noexcept
        {
            this->settings = settings;
            start = clock::now();
            std::forward<F>(function)(std::forward<Args>(args)...);
            end = clock::now();
        }
    };


    template <size_t N, typename duration = automatic_duration, typename clock = std::chrono::steady_clock>
    class AverageTimer
    {
    public:
        using Timer = Timer<duration, clock>;
        using Settings = typename Timer::Settings;

    private:
        std::array<int64_t, N> timers;
        const Settings settings;
        
    public:

        template <typename F, typename... Args>
        AverageTimer(Settings settings, F&& function, Args&&... args) : settings(settings)
        {
            Settings timer_settings = settings;
            timer_settings.show_output = false;

            for (auto& t : timers)
            {
                Timer timer(timer_settings, function, std::forward<Args>(args)...);
                t = timer.get_elapsed();
            }
        }


        ~AverageTimer()
        {
            if (!settings.show_output) return;

            if constexpr (std::is_same_v<duration, automatic_duration>) {
                auto average_time = std::accumulate(timers.begin(), timers.end(), 0LL) / N;
                std::cout << '[' << settings.name << ']' << " took: " << duration_string<automatic_duration>::to_string(average_time) << " to run on average" << std::endl;
            } else {
                auto average_time = std::accumulate(timers.begin(), timers.end(), 0LL);
                std::cout << '[' << settings.name << ']' << " took: " << average_time / N << ' ' << duration_string<duration>::to_string() << " to run on average" << std::endl;
            }
        }

    };
}
