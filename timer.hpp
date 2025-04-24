#include <iostream>
#include <string_view>
#include <string>
#include <chrono>
#include <array>
#include <numeric>
#include <concepts>
#include <type_traits>
#include <source_location>


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
    
    private:
        clock::time_point start;
        clock::time_point end;
        Settings settings;


        std::string format_output(std::string_view result)
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
                settings.output_stream << format_output(automatic_duration_to_string(elapsed)) << std::endl;

            } else {
                auto elapsed = std::chrono::duration_cast<duration>(end - start).count();
                settings.output_stream << format_output(std::to_string(elapsed) + ' ' + std::string(duration_suffix<duration>())) << std::endl;
            }
        }


        [[nodiscard]] const auto get_elapsed(void) const noexcept
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
        using Timer_T = Timer<duration, clock>;
        using Settings = typename Timer_T::Settings;

    private:
        std::array<int64_t, N> timers;
        Settings settings;
        
        std::string format_output(std::string_view result)
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

    public:

        template <typename F, typename... Args>
        AverageTimer(Settings settings, F&& function, Args&&... args) : settings(settings)
        {
            Settings timer_settings = settings;
            timer_settings.show_output = false;

            for (auto& t : timers)
            {
                Timer_T timer(timer_settings, function, std::forward<Args>(args)...);
                t = timer.get_elapsed();
            }
        }


        ~AverageTimer()
        {
            if (!settings.show_output) return;

            if constexpr (std::is_same_v<duration, automatic_duration>) {
                auto average_time = std::accumulate(timers.begin(), timers.end(), 0LL) / N;
                settings.output_stream << format_output(automatic_duration_to_string(average_time)) << std::endl;
            } else {
                auto average_time = std::accumulate(timers.begin(), timers.end(), 0LL) / N;
                settings.output_stream << format_output(std::to_string(average_time) + ' ' + std::string(duration_suffix<duration>())) << std::endl;
            }
        }

    };
}
