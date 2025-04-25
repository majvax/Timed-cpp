/*
 _____________________________________________________________
|                                                             |
|   Timed++ - Lightweight C++20 Timing Utilities              |
|_____________________________________________________________|


    ---------------------------------------------------------------------------
    Timed++ (or Timed-cpp) is a minimal and modern, C++2O header-only library 
    for timing and benchmarking code with style.
    Designed for clarity and simplicity, it leverages modern C++ features 
    to make profiling your functions and code blocks effortless and accurate.


    Features:
      - No external dependencies, header-only, portable
      - Automatic and average timers
      - Customizable output format with named placeholders:
            {filename}, {row}, {name}, {function}, {result}
      - Uses std::chrono and std::source_location for precise timing and context

    Example usage:

        #include "timer.hpp"

        int main()
        {
            {
                // Time the foo function with 10 iterations
                Timer::AverageTimer<10>({"foo"}, foo, 1, 2);
            }
            {
                // Time the fibonacci function once and output the result in seconds
                Timer::Timer<std::chrono::seconds>({"fibonacci"}, fibonacci, 41);
            }
        }


    Author: majvax
    Repository: https://github.com/majvax/Timed-cpp
    Version: 0.2.0 pre-alpha


    Quote from the author:
    "If I learn something new while creating this library,
          it's that std::format is absolute cinema."
    ---------------------------------------------------------------------------

    TODO: 
        - abstract the output formatting to a separate class
        - add more examples and documentation
        - add more tests and benchmarks
        - add more features and utilities
        - base timer class to inherit from
*/


#include <iostream>         // std::cout, std::ostream
#include <string_view>      // std::string_view
#include <string>           // std::string, std::to_string
#include <chrono>           // std::chrono::steady_clock, std::chrono::duration
#include <array>            // std::array
#include <numeric>          // std::accumulate
#include <concepts>         // std::same_as, std::is_base_of_v, std::is_same_v
#include <utility>          // std::forward
#include <source_location>  // std::source_location


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


    struct BaseTimerSettings
    {
        std::string_view name;
        std::string format = "[{filename}:{row} in `{function}` -- {name}] -> {result}";
        bool show_output = true;
        std::source_location location = std::source_location::current();
        std::ostream& output_stream = std::cout;


        std::string_view get_name() const noexcept { return name; }
        std::string_view get_format() const noexcept { return format; }
        std::string_view get_filename() const noexcept { return location.file_name(); }
        int get_line() const noexcept { return location.line(); }
        std::string_view get_function_name() const noexcept { return location.function_name(); }
    };
    
    class BaseTimerFormatter
    {
    protected:
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

        template <Duration duration>
        static constexpr std::string_view duration_suffix() {
            if constexpr (std::same_as<duration, std::chrono::nanoseconds>) return "ns";
            else if constexpr (std::same_as<duration, std::chrono::microseconds>) return "us";
            else if constexpr (std::is_same_v<duration, std::chrono::milliseconds>) return "ms";
            else if constexpr (std::is_same_v<duration, std::chrono::seconds>) return "s";
            else if constexpr (std::is_same_v<duration, std::chrono::minutes>) return "m";
            else if constexpr (std::is_same_v<duration, std::chrono::hours>) return "h";
            else return "unknown";
        }

        template <typename S>
        requires std::derived_from<S, BaseTimerSettings>
        std::string format_output(std::string_view result, S& settings) const noexcept
        {
            std::string out = settings.format;

            auto replace = [&out](const std::string& what, const std::string_view& with) {
                size_t pos = 0;
                while ((pos = out.find(what, pos)) != std::string::npos) {
                    out.replace(pos, what.length(), with);
                    pos += with.length();
                }
            };
            replace("{filename}", settings.get_filename());
            replace("{row}", std::to_string(settings.get_line()));
            replace("{name}", settings.get_name());
            replace("{function}", settings.get_function_name());
            replace("{result}", result);
            return out;
        }
    };


    template <Duration duration = automatic_duration, typename clock = std::chrono::steady_clock>
    class Timer : public BaseTimerFormatter
    {
    public:
        using Settings = BaseTimerSettings;

    private:
        Settings settings;
        clock::time_point start;
        clock::time_point end;

    public:
        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;
        Timer(Timer&&) = delete;
        Timer& operator=(Timer&&) = delete;
    
        template <typename F, typename... Args>
        Timer(Settings settings, F&& function, Args&&... args) noexcept : settings(settings), start(clock::now())
        {
            std::forward<F>(function)(std::forward<Args>(args)...);
            end = clock::now();
        }

        ~Timer() noexcept
        {
            if (!settings.show_output) return;

            if constexpr (std::is_same_v<duration, automatic_duration>) {
                auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                settings.output_stream << format_output(automatic_duration_to_string(elapsed), settings) << std::endl;

            } else {
                auto elapsed = std::chrono::duration_cast<duration>(end - start).count();
                settings.output_stream << format_output(std::to_string(elapsed) + ' ' + std::string(duration_suffix<duration>()), settings) << std::endl;
            }
        }


        [[nodiscard]] const auto get_elapsed() const noexcept
        { 
            return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        }
    };


    template <size_t N, Duration duration = automatic_duration, typename clock = std::chrono::steady_clock>
    class AverageTimer : public BaseTimerFormatter
    {


    public:
        using ChildTimer = Timer<duration, clock>;
        using ChildSettings = ChildTimer::Settings;

        struct Settings: public ChildSettings
        {
            bool child_output = false;

            // Constructor for the child timer
            ChildSettings get_settings_t() const
            {
                ChildSettings settings = *this;
                settings.show_output = child_output;
                return settings;
            }
        };

    private:
            std::array<int64_t, N> timers;
            Settings settings;
        
    public:
        
        
        AverageTimer(const AverageTimer&) = delete;
        AverageTimer& operator=(const AverageTimer&) = delete;
        AverageTimer(AverageTimer&&) = delete;
        AverageTimer& operator=(AverageTimer&&) = delete;

        template <typename F, typename... Args>
        AverageTimer(Settings settings, F&& function, Args&&... args) : settings(settings)
        {
            for (auto& t : timers)
            {
                ChildSettings timer_settings = this->settings.get_settings_t();
                ChildTimer timer(std::move(timer_settings), function, std::forward<Args>(args)...);
                t = timer.get_elapsed();
            }
        }


        ~AverageTimer()
        {
            if (!settings.show_output) return;

            if constexpr (std::is_same_v<duration, automatic_duration>) {
                auto average_time = std::accumulate(timers.begin(), timers.end(), 0LL) / N;
                settings.output_stream << format_output(automatic_duration_to_string(average_time), settings) << std::endl;
            } else {
                auto average_time = std::accumulate(timers.begin(), timers.end(), 0LL) / N;
                settings.output_stream << format_output(std::to_string(average_time) + ' ' + std::string(duration_suffix<duration>()), settings) << std::endl;
            }
        }

    };


    template <Duration duration = automatic_duration, typename clock = std::chrono::steady_clock>
    class BlockTimer : public BaseTimerFormatter
    {
    public:
        using Settings = BaseTimerSettings;

    private:
        Settings settings;
        clock::time_point start;
        clock::time_point m_end;

    public:
        BlockTimer(const BlockTimer&) = delete;
        BlockTimer& operator=(const BlockTimer&) = delete;
        BlockTimer(BlockTimer&&) = delete;
        BlockTimer& operator=(BlockTimer&&) = delete;
    
        BlockTimer(Settings settings) noexcept : settings(settings), start(clock::now())
        {
        }

        void end() noexcept
        {
            m_end = clock::now();
        }

        void show_result() const noexcept
        {
            if (!settings.show_output) return;

            if constexpr (std::is_same_v<duration, automatic_duration>) {
                auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(m_end - start).count();
                settings.output_stream << format_output(automatic_duration_to_string(elapsed), settings) << std::endl;

            } else {
                auto elapsed = std::chrono::duration_cast<duration>(m_end - start).count();
                settings.output_stream << format_output(std::to_string(elapsed) + ' ' + std::string(duration_suffix<duration>()), settings) << std::endl;
            }
        }

        void end_and_show_result() noexcept
        {
            end();
            show_result();
        }


        [[nodiscard]] const auto get_elapsed() const noexcept
        { 
            return std::chrono::duration_cast<std::chrono::nanoseconds>(m_end - start).count();
        }
    }; 
    


}
