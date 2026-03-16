#include <iostream>
#include <vector>

#include "camera.h"
#include "color.h"
#include "starmap.h"
#include "math.h"
#include "blackhole.h"
#include "geodesic_integrator.h"


int main()
{
    // ------------------------------------------------ Image Settings -------------------------------------------------
    constexpr int width  = 200;  // Image width
    constexpr int height = 100;  // Image height

    std::vector<Color> image;       // Image data initialization
    image.resize(width * height);   // Pre-allocate memory for the image

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

    // --------------------------------------------------- Star Map ----------------------------------------------------
    StarMap star_map("StarMaps/starmap_2020_16k.exr");
    BlackHole::Schwarzschild Schwarzschild(1.0e6 * BlackHole::M_Solar, {0, 0, 0}); // mass = 1 million Solar masses

    // ---------------------------------------------------- Render -----------------------------------------------------
    for(int i = 0; i < height; ++i)
    {
        // Progress bar
        double progress = std::round(1000.0 * i / height) / 10.0;      // Calculate progress with one decimal place
        std::cout << "\rProgress: " << progress << "%" << std::flush;  // Print progress

        for(int j = 0; j < width; ++j)
        {
            GeodesicState init = PhotonFromCamera(cam, j, i, Schwarzschild);
            TraceResult result = TracePhotonAdaptive(init, Schwarzschild);

            Color pixel;

            if (result.captured)
            {
                pixel = {0.0f, 0.0f, 0.0f};
            }
            else
            {
                pixel = SamplePhoton(result.state, star_map);
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
        }
    }
    // Finish the render
    std::cout << "\rProgress: 100.0%" << std::endl;

    // ----------------------------------------------------- Save ------------------------------------------------------
    Color::SaveImage("output.pfm", width, height, image);

    return 0;
}
