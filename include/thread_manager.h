#pragma once

#include <thread>
#include <atomic>

struct ThreadManager {
public:
    // Constructor
    ThreadManager(int width, int height) {

    }

    // Counts the number of threads
    unsigned int CountThreads() {
        // Grabs the nmumber of threads
        unsigned int num_threads = std::thread::hardware_concurrency();

        if (num_threads == 0) {
            // Default to using 4 threads
            return 4;
        }

        return num_threads;
    }


private:
    unsigned int tm_thread_count;
    int tm_width;
    int tm_height;
};
