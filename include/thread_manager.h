// include/thread_manager.h

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
        std::atomic<int> pixels_done{0};

        // Number of samples per pixel for anti-aliasing.
        // 4 = good default. 1 = AA disabled. 16 = high quality.
        constexpr int SAMPLES_PER_PIXEL = 1;

        // Exposure scale applied before tone-mapping.
        // With T^4 luminance scaling in FromBlackbody (T_ref = 1e6 K),
        // disk pixels arrive with raw values roughly in [0.01, 100].
        // Tune this so the brightest disk region sits around 1.0–5.0
        // after multiplication — Reinhard will compress the rest.
        constexpr float EXPOSURE = 5.0f;

        // Chunk size: how many pixels each thread grabs at once.
        constexpr int CHUNK = 8;

        // Fast xorshift32 RNG — produces a float in [0, 1).
        auto rand01 = [](uint32_t& s) -> float {
            s ^= s << 13;
            s ^= s >> 17;
            s ^= s << 5;
            return (s & 0x00FFFFFFu) / float(0x01000000u);
        };

        sw.Start();

        auto worker = [&]()
        {
            while (true)
            {
                int start = next_pixel.fetch_add(CHUNK);
                if (start >= total)
                    break;

                for (int i = 0; i < CHUNK; ++i)
                {
                    int idx = start + i;
                    if (idx >= total) break;

                    int x = idx % width;
                    int y = idx / width;

                    // Per-pixel RNG seed
                    uint32_t seed = static_cast<uint32_t>(y * 1973 + x * 9277 + 1);
                    rand01(seed);
                    rand01(seed);

                    // ------------------------------------------------
                    // 1. Accumulate raw LINEAR HDR samples
                    // ------------------------------------------------
                    Color accumulated = {0.0f, 0.0f, 0.0f};

                    for (int s = 0; s < SAMPLES_PER_PIXEL; ++s)
                    {
                        float offset_x = (s == 0) ? 0.0f : rand01(seed) - 0.5f;
                        float offset_y = (s == 0) ? 0.0f : rand01(seed) - 0.5f;

                        GeodesicState photon = PhotonFromCamera(cam,
                                                                x + offset_x,
                                                                y + offset_y,
                                                                spacetime);

                        TraceResult result = TracePhotonAdaptive(photon, spacetime, world);

                        Color sample;

                        if (result.captured)
                        {
                            sample = {0.0f, 0.0f, 0.0f};
                        }
                        else if (result.mat)
                        {
                            Color obj_color = result.mat->Shade(result.ray, result.hit);
                            Color bg_color  = SamplePhoton(result.state, star_map);

                            float w  = static_cast<float>(result.hit.coverage);
                            sample.r = w * obj_color.r + (1.0f - w) * bg_color.r;
                            sample.g = w * obj_color.g + (1.0f - w) * bg_color.g;
                            sample.b = w * obj_color.b + (1.0f - w) * bg_color.b;
                        }
                        else
                        {
                            sample = SamplePhoton(result.state, star_map);
                        }

                        // Guard NaN/Inf
                        if (!std::isfinite(sample.r)) sample.r = 0.0f;
                        if (!std::isfinite(sample.g)) sample.g = 0.0f;
                        if (!std::isfinite(sample.b)) sample.b = 0.0f;

                        accumulated.r += sample.r;
                        accumulated.g += sample.g;
                        accumulated.b += sample.b;
                    }

                    // ------------------------------------------------
                    // 2. Average samples
                    // ------------------------------------------------
                    float inv = 1.0f / static_cast<float>(SAMPLES_PER_PIXEL);
                    Color pixel = accumulated * inv;

                    // ------------------------------------------------
                    // 3. Exposure + ACES tone mapping
                    // ------------------------------------------------
                    auto aces = [](float x) -> float {
                        const float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
                        return std::clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0f, 1.0f);
                    };

                    //pixel.r = aces(pixel.r * EXPOSURE);
                    //pixel.g = aces(pixel.g * EXPOSURE);
                    //pixel.b = aces(pixel.b * EXPOSURE);

                    // ------------------------------------------------
                    // 4. Gamma correction (linear → sRGB display)
                    // ------------------------------------------------
                    //pixel.r = std::pow(std::max(pixel.r, 0.0f), 1.0f / 2.2f);
                    //pixel.g = std::pow(std::max(pixel.g, 0.0f), 1.0f / 2.2f);
                    //pixel.b = std::pow(std::max(pixel.b, 0.0f), 1.0f / 2.2f);

                    framebuffer[idx] = pixel;

                    // ---- Progress reporting ----
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

        for (auto& t : threads)
            t.join();
                
        // Fix seam at center column
    int seam_x = width / 2;
    for (int dx = -2; dx <= 2; ++dx) {
        int x = seam_x + dx;
        if (x < 5 || x >= width - 5) continue;
        for (int y = 0; y < height; ++y) {
            int idx = y * width + x;
            // Sample 5 pixels out instead of 3
            Color l = framebuffer[y * width + (x - 5)];
            Color r = framebuffer[y * width + (x + 5)];
            framebuffer[idx].r = (l.r + r.r) * 0.5f;
            framebuffer[idx].g = (l.g + r.g) * 0.5f;
            framebuffer[idx].b = (l.b + r.b) * 0.5f;
        }
    }

        sw.Stop();
        sw.Finish();

        return framebuffer;
    }

private:
    unsigned int tm_thread_count;
    int tm_width;
    int tm_height;
};