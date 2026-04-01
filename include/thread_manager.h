#pragma once

#include <thread>
#include <atomic>
#include <vector>

#include "blackhole.h"
#include "camera.h"
#include "color.h"
#include "geodesic_integrator.h"
#include "hittable_list.h"
#include "material.h"
#include "starmap.h"
#include "stopwatch.h"

struct ThreadManager {
public:
    // Constructor
    ThreadManager(int width, int height)
        : tm_width(width), tm_height(height)
    {
        unsigned int N = CountThreads();
        SetThreads(N);
        std::cout << "Using " << N << " threads.\n";
    }

    // Returns the number of hardware threads available
    unsigned int CountThreads() {
        unsigned int n = std::thread::hardware_concurrency();
        return (n == 0) ? 4 : n;
    }

    void SetThreads(unsigned int N) {
        tm_thread_count = N;
    }

    // ---------------------------------------------------- Render -----------------------------------------------------
    std::vector<Color> RenderThreaded(Camera& cam,
                                      int width,
                                      int height,
                                      BlackHole::Spacetime& spacetime,
                                      HittableList& world,
                                      StarMap& star_map,
                                      Stopwatch& sw)
    {
        const int total = width * height;
        std::vector<Color> framebuffer(total);
        std::atomic<int> next_pixel{0};

        // Tracks how many pixels have been fully written across all threads.
        // Used for progress reporting. Incremented atomically so only one thread
        // prints at a time (no interleaved output).
        std::atomic<int> pixels_done{0};

        // Number of samples per pixel for anti-aliasing.
        // 4 = good default. 1 = AA disabled. 16 = high quality.
        constexpr int SAMPLES_PER_PIXEL = 4;

        // Exposure for tone mapping
        constexpr float EXPOSURE = 10.0f;

        // Chunk size: how many pixels each thread grabs at once.
        // Larger chunks = less atomic contention. Smaller = better load balancing.
        constexpr int CHUNK = 8;

        // Fast xorshift32 RNG — produces a float in [0, 1).
        // Passed by reference so state advances with each call.
        auto rng = [](uint32_t& state) -> float {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return (state & 0x00FFFFFFu) / float(0x01000000u);
        };

        // Start the stopwatch before launching threads
        sw.Start();

        auto worker = [&]()
        {
            while (true)
            {
                // Atomically grab the next chunk of pixels
                int start = next_pixel.fetch_add(CHUNK);
                if (start >= total)
                    break;

                for (int i = 0; i < CHUNK; ++i)
                {
                    int idx = start + i;
                    if (idx >= total)
                        break;

                    int x = idx % width;
                    int y = idx / width;

                    // Per-pixel RNG seed — unique per pixel, independent across threads
                    uint32_t seed = static_cast<uint32_t>(y * 1973 + x * 9277 + 1);

                    // Accumulate raw (pre-tonemapped) color over N samples
                    Color accumulated = {0.0f, 0.0f, 0.0f};

                    for (int s = 0; s < SAMPLES_PER_PIXEL; ++s)
                    {
                        // Jittered sub-pixel offset in [-0.5, +0.5]
                        float offset_x = rng(seed) - 0.5f;
                        float offset_y = rng(seed) - 0.5f;

                        // Generate photon with fractional pixel coordinates
                        GeodesicState photon = PhotonFromCamera(cam,
                                                                x + offset_x,
                                                                y + offset_y,
                                                                spacetime);

                        // Trace through curved spacetime
                        TraceResult result = TracePhotonAdaptive(photon, spacetime, world);

                        // Shade the result
                        Color sample;

                        if (result.captured)
                        {
                            // Photon fell into the black hole
                            sample = {0.0f, 0.0f, 0.0f};
                        }
                        else if (result.mat)
                        {
                            // Use the Cartesian ray stored in TraceResult (set at hit time)
                            Color sphere_color = result.mat->Shade(result.ray, result.hit);
                            Color bg_color     = SamplePhoton(result.state, star_map);

                            // Blend sphere and background by edge coverage weight.
                            // coverage = 1.0 for fully inside hits, tapers to 0 at the edge.
                            float w   = static_cast<float>(result.hit.coverage);
                            sample.r  = w * sphere_color.r + (1.0f - w) * bg_color.r;
                            sample.g  = w * sphere_color.g + (1.0f - w) * bg_color.g;
                            sample.b  = w * sphere_color.b + (1.0f - w) * bg_color.b;
                        }
                        else
                        {
                            // Photon escaped to the background starmap
                            sample = SamplePhoton(result.state, star_map);
                        }

                        accumulated.r += sample.r;
                        accumulated.g += sample.g;
                        accumulated.b += sample.b;
                    }

                    // Average the samples
                    float inv = 1.0f / static_cast<float>(SAMPLES_PER_PIXEL);
                    Color pixel;
                    pixel.r = accumulated.r * inv;
                    pixel.g = accumulated.g * inv;
                    pixel.b = accumulated.b * inv;

                    // Tone mapping — applied once after averaging
                    pixel.r = 1.0f - std::exp(-EXPOSURE * pixel.r);
                    pixel.g = 1.0f - std::exp(-EXPOSURE * pixel.g);
                    pixel.b = 1.0f - std::exp(-EXPOSURE * pixel.b);

                    // Gamma correction
                    pixel.r = std::pow(pixel.r, 1.0f / 2.2f);
                    pixel.g = std::pow(pixel.g, 1.0f / 2.2f);
                    pixel.b = std::pow(pixel.b, 1.0f / 2.2f);

                    framebuffer[idx] = pixel;

                    // ---- Progress reporting ----
                    // Only one thread prints at a time. We use fetch_add so the count
                    // is always accurate even with many threads writing simultaneously.
                    int done = pixels_done.fetch_add(1) + 1;
                    if (done % 100 == 0 || done == total)
                        sw.DisplayProgress(done, total);
                }
            }
        };

        // Launch all worker threads
        std::vector<std::thread> threads;
        threads.reserve(tm_thread_count);
        for (unsigned int i = 0; i < tm_thread_count; ++i)
            threads.emplace_back(worker);

        // Wait for all threads to finish
        for (auto& t : threads)
            t.join();

        // Stop the stopwatch and print the final completed bar
        sw.Stop();
        sw.Finish();

        return framebuffer;
    }

private:
    unsigned int tm_thread_count;
    int tm_width;
    int tm_height;
};