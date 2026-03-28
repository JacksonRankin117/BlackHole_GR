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
    constexpr int width  = 200;  // Image width
    constexpr int height = 100;  // Image height

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
    Camera cam(width,   // Image width
               height,  // Image height
               45.0,    // Field of view in degrees
               camPos,  // Camera position
               target,  // Target position
               upVec);  // Up vector

    // ==================================================== Render =====================================================
    render(cam, width, height, schwarzschild, world, star_map, sw);

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
            Stopwatch sw)
{

    // Pre-allocate a vector to hold the image
    const int total = width * height;
    std::vector<Color> image;
    image.resize(width * height);

    // Start the stopwatch
    sw.Start();

    for(int i = 0; i < height; ++i)
    {
        for(int j = 0; j < width; ++j)
        {
            // Ray generation and marching
            GeodesicState init = PhotonFromCamera(cam, j, i, spacetime);  // Generate Photon position and direction
            TraceResult result = TracePhotonAdaptive(init, spacetime, world);    // March the photon into the scene

            Color pixel;

            // Finds the color of the pixel
            if (result.captured) {
                // Color the pixel black if the photon is captured
                pixel = {0.0f, 0.0f, 0.0f}; // black hole
            } else if (result.mat) {
                // If it hits a Hittable object, color it in
                Ray ray(result.state.x, result.state.k); // reconstruct ray
                pixel = result.mat->Shade(ray, result.hit); // color the sphere
            } else {
                // If it misses completely, color the pixel with information from thew skymap
                pixel = SamplePhoton(result.state, star_map); // background
            }

            // Linear tone map
            float exposure = 10.0f;
            pixel.r = 1.0f - std::exp(-exposure * pixel.r);
            pixel.g = 1.0f - std::exp(-exposure * pixel.g);
            pixel.b = 1.0f - std::exp(-exposure * pixel.b);

            // Gamma-correction
            pixel.r = std::pow(pixel.r, 1.0f / 2.2f);
            pixel.g = std::pow(pixel.g, 1.0f / 2.2f);
            pixel.b = std::pow(pixel.b, 1.0f / 2.2f);

            image[i*width + j] = pixel;

            // Display pixel-by-pixel progress
            int current = i * width + j + 1;

            // Display output
            if ((current % 10) == 0)   // Display progress every N iterations
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
