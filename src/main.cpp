#include <iostream>
#include <vector>
#include "camera.h"
#include "color.h"
#include "starmap.h"
#include "math.h"
#include "blackhole.h"

int main()
{
    // ------------------------------------------------ Image Settings -------------------------------------------------
    constexpr int width  = 1920;  // Image width
    constexpr int height = 1080;  // Image height

    std::vector<Color> image;       // Image data initialization
    image.reserve(width * height);  // Pre-allocate memory for the image

    // ---------------------------------------------------- Camera -----------------------------------------------------
    Math::Vec3 camPos  = {0.0, BlackHole::AU, 0};  // Camera position at 1 AU along the Y-axis
    Math::Vec3 target  = {0.0, 0.0, 0.0};          // Stare at the origin like some sort of freaking creep
    Math::Vec3 upVec   = {0.0, 0.0, 1.0};          // Z-axis is the "up" direction

    // Construct the camera
    Camera cam(width,   // Image width
               height,  // Image height
               45.0,    // Field of view in degrees
               camPos,  // Camera position
               target,  // Target position
               upVec);  // Up vector

    // --------------------------------------------------- Star Map ----------------------------------------------------
    StarMap star_map("StarMaps/starmap_2020_16k.exr");
    BlackHole::Schwarzschild dummySpacetime(1.0 * BlackHole::M_Solar, {0, 0, 0}); // mass = 1.0 Solar masses

    // ---------------------------------------------------- Render -----------------------------------------------------
    for(int i = 0; i < height; ++i)
    {
        // Progress bar
        double progress = std::round(1000.0 * i / height) / 10.0;      // Calculate progress with one decimal place
        std::cout << "\rProgress: " << progress << "%" << std::flush;  // Print progress

        for(int j = 0; j < width; ++j)
        {
            // Generate a ray based on the pixel index
            Ray r = cam.GenerateRay(j, i);
            Math::Vec3 dir = r.direction;   // Find the spatial direction of the ray

            // Sample the star map with the ray direction
            Color pixel = star_map.Sample(dir);

            // Tone mapping
            float exposure = 20.0f;
            pixel.r = 1.0f - std::exp(-exposure * pixel.r);
            pixel.g = 1.0f - std::exp(-exposure * pixel.g);
            pixel.b = 1.0f - std::exp(-exposure * pixel.b);

            // Add the pixel to the image via indexing (remember the pre-allocated memory?)
            image[i * width + j] = pixel;
        }
    }
    // Finish the render
    std::cout << "\rProgress: 100.0%" << std::endl;

    // ----------------------------------------------------- Save ------------------------------------------------------
    Color::SaveImage("imge.pfm", width, height, image);

    return 0;
}
