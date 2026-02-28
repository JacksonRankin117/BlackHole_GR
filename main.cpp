#include <iostream>
#include <cmath>

//#include "math.h"
//#include "physics.h"

#include "camera.h"
#include "color.h"
#include "ray.h"

using namespace Math;

int main(int argc, char* argv[])
{
    //----------------------------------------------- Initialize Camera ------------------------------------------------
    // Dimensions of the image
    int width = 1920;
    int height = 1080;

    // Initialize camera
    Camera cam = Camera(width,                 // Image width
                        height,                // Image height
                        180.0,                 // Field of view in degrees
                        {-100.0, 50.0, 20.0},  // Camera position
                        {0.0, 0.0, 0.0},       // Target position
                        {0, 1, 0});            // Up direction (z-axis is up)

    // ------------------------------------------------ Image Rendering ------------------------------------------------
    // Stores the last progress of the render
    double last_progress = -1.0;  // If we were to initialize it with 0.0, the program wouldn't print anything

    // Store the image as a vector of pixels
    std::vector<Color> image;

    // Project rays through each pixel
    for (int i = 0; i < height; i++)
    {
        // --------------------------------------------- Progress Bar --------------------------------------------------
        // Calculates progress
        double progress = std::round(1000.0 * i / height) / 10.0;

        // If progress has changed, print the new one, otherwise continue
        if (progress != last_progress)
        {
            std::cout << "\rProgress: " << progress << "%" << std::flush;
            last_progress = progress;
        }

        // -------------------------------------------- Ray Generation -------------------------------------------------
        for (int j = 0; j < width; j++)
        {
            // Generate a ray through each pixel
            Ray ray = cam.GenerateRay(j, i);

            //
            Vec4 dir = ray.r_direct; // normalized

            Color c = {
                0.5f * ((float)dir.X + 1.0f),
                0.5f * ((float)dir.Y + 1.0f),
                0.5f * ((float)dir.Z + 1.0f)
            };

            // Add the Color to the image as a pixel
            image.push_back(c);
        }
    }

    // Display 100.0%
    std::cout << "\rProgress: 100.0%" << std::flush;

    // Save the image as a PFM file
    Color::SaveImage("image.pfm", width, height, image);



    return 0;
}
