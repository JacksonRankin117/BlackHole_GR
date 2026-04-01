#include <vector>

#include "blackhole.h"
#include "camera.h"
#include "color.h"
#include "geodesic_integrator.h"
#include "hittable_list.h"
#include "math_objects.h"
#include "starmap.h"
#include "stopwatch.h"
#include "sphere.h"
#include "thread_manager.h"

// =============================================== Function Declarations ===============================================
void render(Camera& cam,
            int width,
            int height,
            BlackHole::Spacetime& spacetime,
            HittableList& world,
            StarMap& star_map,
            Stopwatch sw);

int main()
{
    // ================================================ Image Settings =================================================
    constexpr int width  = 2160;  // Image width
    constexpr int height = 1440;  // Image height

    // Render timer
    Stopwatch sw{60};

    // ===================================================== Scene =====================================================

    // Background
    StarMap star_map("StarMaps/starmap_2020_16k.exr");

    // Black hole
    BlackHole::Schwarzschild schwarzschild(1.0e6 * BlackHole::M_Solar, {0, 0, 0}); // mass = 1 million Solar masses

    // ---------------------------------------------------- Objects ----------------------------------------------------
    HittableList world;

    // --- Red diffuse sphere ---
    auto red_material = std::make_shared<OneColor>(Color{1.0, 0.0, 0.0});

    auto sphere1 = std::make_shared<Sphere>(
        Math::Vec4{ 0.0,                   // T
                    0.0 * BlackHole::AU,   // X
                    0.05 * BlackHole::AU,  // Y
                    0.0 },                 // Z
        0.02 * BlackHole::AU,              // Radius
        red_material                       // Material
    );
    world.Add(sphere1);

    // --- Blue diffuse sphere ---
    auto blue_material = std::make_shared<OneColor>(Color{0.0, 0.0, 1.0});

    auto sphere2 = std::make_shared<Sphere>(
        Math::Vec4{ 0.0,                 // T
                    0.2 * BlackHole::AU, // X
                    0.1 * BlackHole::AU, // Y
                    0.0 },               // Z
        0.01 * BlackHole::AU,            // Radius
        blue_material                    // Material
    );
    world.Add(sphere2);

    // --- Hot blackbody sphere ---
    auto hot_material = std::make_shared<Blackbody>(5000.0); // Kelvin

    auto sphere3 = std::make_shared<Sphere>(
        Math::Vec4{ 0.00,                    // T
                   -0.10 * BlackHole::AU,    // X
                   -0.10 * BlackHole::AU,    // Y
                    0.05 * BlackHole::AU },  // Z
        0.01 * BlackHole::AU,                // Radius
        hot_material                         // Material
    );
    world.Add(sphere3);

    // ==================================================== Camera =====================================================
    Math::Vec3 camPos  = {0.5 * BlackHole::AU, 0.0, 0.0};  // Camera position at 0.5 AU along the X-axis
    Math::Vec3 target  = {0.0, 0.0, 0.0};                  // Stare at the origin like some sort of freaking creep
    Math::Vec3 upVec   = {0.0, 0.0, 1.0};                  // (0, 0, 1) aligns with the north celestial pole

    // Construct the camera
    Camera cam(width,   // Image width in pixels
               height,  // Image height in pixels
               45.0,    // FOV in degrees
               camPos,  // Position of the camera
               target,  // Position of the target of the camera. 
               upVec);  // What the camera thinks is "up"

    // ==================================================== Render =====================================================
    ThreadManager tm(width, height);
    
    auto framebuffer = tm.RenderThreaded(cam, width, height, schwarzschild, world, star_map, sw);

    Color::SaveImage("output.pfm", width, height, framebuffer);

    return 0;
}

// =====================================================================================================================
// =============================================== Function definitions ================================================
// =====================================================================================================================
void render(Camera& cam,
            int width,
            int height,
            BlackHole::Spacetime& spacetime,
            HittableList& world,
            StarMap& star_map,
            Stopwatch sw) {
    // ---- Anti-aliasing settings ----
    // Number of samples per pixel. Higher = smoother edges, longer render time.
    // 4 is a good default. Try 1 to disable AA, 16 for high quality.
    constexpr int SAMPLES_PER_PIXEL = 1;

    // A simple, fast pseudo-random number generator (xorshift32).
    // Produces a float in [0, 1). Used for jittered sub-pixel offsets.
    // Seeded differently per pixel to avoid structured noise patterns.
    auto rng = [](uint32_t& state) -> float {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return (state & 0x00FFFFFFu) / float(0x01000000u);
    };

    // Pre-allocate a vector to hold the image
    const int total = width * height;
    std::vector<Color> image;
    image.resize(total);

    // Start the stopwatch
    sw.Start();

    for (int i = 0; i < height; ++i)
    {
        for (int j = 0; j < width; ++j)
        {
            // ---- Per-pixel RNG seed ----
            // Seed is unique per pixel so each pixel gets independent jitter.
            // The multipliers are arbitrary primes to spread seeds apart.
            uint32_t seed = static_cast<uint32_t>(i * 1973 + j * 9277 + 1);

            // ---- Accumulate color over N samples ----
            Color accumulated = {0.0f, 0.0f, 0.0f};

            for (int s = 0; s < SAMPLES_PER_PIXEL; ++s)
            {
                // Jitter the ray within the current pixel by a random sub-pixel
                // offset in [-0.5, +0.5]. This is "jittered" (stratified) MSAA —
                // better than pure random sampling at avoiding clumping.
                float offset_x = rng(seed) - 0.5f;
                float offset_y = rng(seed) - 0.5f;

                // Generate the photon for this sample.
                // PhotonFromCamera accepts fractional pixel coordinates.
                GeodesicState init = PhotonFromCamera(cam,
                                                      j + offset_x,
                                                      i + offset_y,
                                                      spacetime);

                // March the photon through curved spacetime
                TraceResult result = TracePhotonAdaptive(init, spacetime, world);

                // Determine the raw color for this sample
                Color sample;

                if (result.captured)
                {
                    // Photon fell into the black hole
                    sample = {0.0f, 0.0f, 0.0f};
                }
                else if (result.mat)
                {
                    // Photon hit a scene object
                    Ray ray(result.state.x, result.state.k);
                    sample = result.mat->Shade(ray, result.hit);
                }
                else
                {
                    // Photon escaped to the background starmap
                    sample = SamplePhoton(result.state, star_map);
                }

                // Accumulate the raw (pre-tonemapped) sample.
                // Tone mapping is applied AFTER averaging so it operates on the
                // mean radiance, not on individually clamped samples. Doing it
                // the other way would bias bright pixels darker.
                accumulated.r += sample.r;
                accumulated.g += sample.g;
                accumulated.b += sample.b;
            }

            // ---- Average the samples ----
            float inv_samples = 1.0f / static_cast<float>(SAMPLES_PER_PIXEL);
            Color pixel;
            pixel.r = accumulated.r * inv_samples;
            pixel.g = accumulated.g * inv_samples;
            pixel.b = accumulated.b * inv_samples;

            /*
            // ---- Tone mapping (applied once, after averaging) ----
            float exposure = 10.0f;
            pixel.r = 1.0f - std::exp(-exposure * pixel.r);
            pixel.g = 1.0f - std::exp(-exposure * pixel.g);
            pixel.b = 1.0f - std::exp(-exposure * pixel.b);

            // ---- Gamma correction ----
            pixel.r = std::pow(pixel.r, 1.0f / 2.2f);
            pixel.g = std::pow(pixel.g, 1.0f / 2.2f);
            pixel.b = std::pow(pixel.b, 1.0f / 2.2f);
            */
            image[i * width + j] = pixel;

            // ---- Progress display ----
            int current = i * width + j + 1;
            if ((current % 10) == 0)
                sw.DisplayProgress(current, total);
        }
    }

    // Stop the stopwatch
    sw.Stop();

    // Finish the render
    sw.Finish();

    // Save to a PFM file
    Color::SaveImage("output.pfm", width, height, image);
}

