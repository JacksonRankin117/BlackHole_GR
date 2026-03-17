#include <iostream>
#include <vector>

#include "blackhole.h"
#include "camera.h"
#include "color.h"
#include "geodesic_integrator.h"
#include "math.h"
#include "starmap.h"
#include "stopwatch.h"


int main()
{
    // ------------------------------------------------ Image Settings -------------------------------------------------
    constexpr int width  = 200;  // Image width
    constexpr int height = 100;  // Image height

    const int total = width * height;  // Image resolution

    std::vector<Color> image;       // Image data initialization
    image.resize(width * height);   // Pre-allocate memory for the image

    // Render timer
    Stopwatch sw{60};

    // ---------------------------------------------------- Camera -----------------------------------------------------
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

    // ----------------------------------------------------- Scene -----------------------------------------------------

    // Background
    StarMap star_map("StarMaps/starmap_2020_16k.exr");

    // Black hole
    BlackHole::Schwarzschild Schwarzschild(1.0e6 * BlackHole::M_Solar, {0, 0, 0}); // mass = 1 million Solar masses

    // ---------------------------------------------------- Render -----------------------------------------------------
    // Start stopwatch
    sw.Start();

    // Image loop
    for(int i = 0; i < height; ++i)
    {
        for(int j = 0; j < width; ++j)
        {
            // Ray generation and marching
            GeodesicState init = PhotonFromCamera(cam, j, i, Schwarzschild);  // Generate Photon position and direction
            TraceResult result = TracePhotonAdaptive(init, Schwarzschild);    // March the photon into the scene

            Color pixel;

            // Finds the color of the pixrl
            if (result.captured)
            {
                // pixel = {1.0f, 0.0f, 0.0f};  // Color it red if it falls in (debugging)
                pixel = {0.0f, 0.0f, 0.0f};  // Pixel should be black if it hits event horizon
            }
            else
            {
                pixel = SamplePhoton(result.state, star_map);  // Use the star map to color the pixel
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

    // ----------------------------------------------------- Save ------------------------------------------------------
    Color::SaveImage("output.pfm", width, height, image);

    return 0;
}
