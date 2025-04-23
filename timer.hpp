#include <iostream>
#include <string_view>
#include <string>
#include <chrono>
#include <array>
#include <numeric>


namespace Timer
{

    struct automatic_duration {};


    template <typename T>
    concept Duration = std::is_base_of_v<std::chrono::duration<typename T::rep, typename T::period>, T>
                    || std::same_as<T, automatic_duration>;

    
    constexpr int64_t ns_in_us = 1000;
    constexpr int64_t ns_in_ms = ns_in_us * 1000;
    constexpr int64_t ns_in_s  = ns_in_ms * 1000;
    constexpr int64_t ns_in_min = 60 * ns_in_s;
    constexpr int64_t ns_in_hr  = 60 * ns_in_min;



    template <Duration duration>
    constexpr std::string_view duration_suffix() {
        if constexpr (std::same_as<duration, std::chrono::nanoseconds>) return "ns";
        else if constexpr (std::same_as<duration, std::chrono::microseconds>) return "us";
        else if constexpr (std::is_same_v<duration, std::chrono::milliseconds>) return "ms";
        else if constexpr (std::is_same_v<duration, std::chrono::seconds>) return "s";
        else if constexpr (std::is_same_v<duration, std::chrono::minutes>) return "m";
        else if constexpr (std::is_same_v<duration, std::chrono::hours>) return "h";
        else return "unknown";
    }

    
    static const std::string automatic_duration_to_string(int64_t elapsed) noexcept {
        if (elapsed < ns_in_us) {
            return std::to_string(elapsed) + " ns";
        } else if (elapsed < ns_in_ms) {
            return std::to_string(elapsed / 1000.0) + " us";
        } else if (elapsed < ns_in_s) {
            return std::to_string(elapsed / 1'000'000.0) + " ms";
        } else if (elapsed < ns_in_min) {
            return std::to_string(elapsed / 1'000'000'000.0) + " s";
        } else if (elapsed < ns_in_hr) {
            return std::to_string(elapsed / 60'000'000'000.0) + " m";
        } else {
            return std::to_string(elapsed / 3'600'000'000'000.0) + " h";
        }
    }


    template <Duration duration = automatic_duration, typename clock = std::chrono::steady_clock>
    class Timer 
    {
    public:
        struct Settings
        {
            std::string_view name;
            bool show_output = true;
        };
    
    private:
        typename clock::time_point start;
        typename clock::time_point end;
        Settings settings;

    public:
        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;
        Timer(Timer&&) = delete;
        Timer& operator=(Timer&&) = delete;
    
        template <typename F, typename... Args>
        inline Timer(Settings settings, F&& function, Args&&... args) noexcept : start(clock::now()), settings(settings)
        {
            std::forward<F>(function)(std::forward<Args>(args)...);
            end = clock::now();
        }

        inline ~Timer() noexcept
        {
            if (!settings.show_output) return;

            if constexpr (std::is_same_v<duration, automatic_duration>) {
                auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                std::cout << '[' << settings.name << ']' <<  " took: " << automatic_duration_to_string(elapsed) << " to run" << std::endl;

            } else {
                auto elapsed = std::chrono::duration_cast<duration>(end - start).count();
                std::cout << '[' << settings.name << ']' <<  " took: " << elapsed  << ' ' << duration_suffix<duration>() << " to run" << std::endl;
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


    template <size_t N, Duration duration = automatic_duration, typename clock = std::chrono::steady_clock>
    class AverageTimer
    {
    public:
        using Timer = Timer<duration, clock>;
        using Settings = typename Timer::Settings;

    private:
        std::array<int64_t, N> timers;
        Settings settings;
        
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
                std::cout << '[' << settings.name << ']' << " took: " << automatic_duration_to_string(average_time) << " to run on average" << std::endl;
            } else {
                auto average_time = std::accumulate(timers.begin(), timers.end(), 0LL);
                std::cout << '[' << settings.name << ']' << " took: " << average_time / N << ' ' << duration_suffix<duration>() << " to run on average" << std::endl;
            }
        }

    };
}
