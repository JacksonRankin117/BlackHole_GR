#pragma once

#include <vector>
#include <thread>
#include <atomic>
#include <chrono>

#include "color.h"
#include "stopwatch.h"

class ThreadManager
{
public:

    ThreadManager(int width, int height)
        : width(width), height(height)
    {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4; // fallback
        std::cout << "Using " << num_threads << " threads.\n";
    }

    // ------------------------------------------------------------------
    template<typename RenderFunc>
    void RenderThreaded(RenderFunc render_pixel,
                        std::vector<Color>& image,
                        Stopwatch& sw)
    {
        std::atomic<int> next_row{0};
        std::atomic<int> rows_done{0};
        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        auto worker = [&, render_pixel]()  // capture render_pixel by value
        {
            while (true)
            {
                int i = next_row.fetch_add(1);
                if (i >= height) break;

                for (int j = 0; j < width; ++j)
                    image[i * width + j] = render_pixel(i, j);

                rows_done.fetch_add(1, std::memory_order_relaxed);
            }
        };

        for (unsigned t = 0; t < num_threads; ++t)
            threads.emplace_back(worker);

        while (rows_done.load(std::memory_order_relaxed) < height)
        {
            sw.DisplayProgress(rows_done.load(), height);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        for (auto& th : threads)
            th.join();

        sw.DisplayProgress(height, height);
    }

private:

    int width;
    int height;
    unsigned num_threads;
};