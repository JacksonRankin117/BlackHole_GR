#pragma once

#include <chrono>
#include <iostream>
#include <iomanip>
#include <format>
#include <algorithm>

class Stopwatch
{
public:

    using clock = std::chrono::steady_clock;

    explicit Stopwatch(int bar_width = 50)
        : bar_width(bar_width) {}

    // ---------------------------------------------------- Timing -----------------------------------------------------

    void Start()
    {
        start_time = clock::now();
        running = true;
    }

    void Stop()
    {
        stop_time = clock::now();
        running = false;
    }

    double ElapsedSeconds() const
    {
        auto end = running ? clock::now() : stop_time;
        return std::chrono::duration<double>(end - start_time).count();
    }

    // ----------------------------------------------- Progress Display ------------------------------------------------

    void DisplayProgress(std::uint64_t current,
                         std::uint64_t total)
    {
        if (total == 0) return;

        double progress =
            static_cast<double>(current) / total;

        progress = std::clamp(progress, 0.0, 1.0);

        const double elapsed = ElapsedSeconds();

        // ETA 
        double eta = 0.0;
        if (progress > 0.0 && progress < 1.0)
            eta = elapsed * (1.0 - progress) / progress;

        // Redraw line
        std::cout << "\r\033[K";

        // Progress bar 
        DrawBar(progress);

        // ----- Percent -----
        std::cout << " "
                  << std::fixed << std::setprecision(1)
                  << std::setw(5) << progress * 100.0
                  << "%";

        // ----- Time info -----
        std::cout << " | Elapsed "
                  << FormatTime(elapsed);

        if (progress < 1.0)
            std::cout << " | ETA "
                      << FormatTime(eta);

        std::cout << std::flush;
    }

    void Finish()
    {
        DisplayProgress(1, 1);
        std::cout << '\n';
    }

private:

    // -------------------- Bar Drawing --------------------

    void DrawBar(double progress) const
    {
        int filled =
            static_cast<int>(progress * bar_width);

        int empty = bar_width - filled;

        std::cout << "[";

        for (int i = 0; i < filled; ++i)
            std::cout << "█";

        for (int i = 0; i < empty; ++i)
            std::cout << "░";

        std::cout << "]";
    }

    // -------------------- Time Formatting --------------------

    static std::string FormatTime(double seconds)
    {
        int s = static_cast<int>(seconds);
        int h = s / 3600;
        int m = (s % 3600) / 60;
        s %= 60;

        return std::format("{:02}:{:02}:{:02}", h, m, s);
    }

private:

    clock::time_point start_time{};
    clock::time_point stop_time{};
    bool running = false;

    int bar_width;
};
