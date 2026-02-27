//#include <iostream>

#include "math.h"
//#include "physics.h"

#include "camera.h"

using namespace Math;

int main(int argc, char* argv[])
{
    // Dimensions of the image
    int width = 1920;
    int height = 1080;

    // Initialize camera
    Camera cam = Camera(width,             // Image width
                        height,            // Image height
                        20.0,              // Field of view in degrees
                        {-100, 0, 20},     // Camera position
                        {0.0, 0.0, 0.0},   // Target position
                        {0, 0, 1});        // Up direction (z-axis is up)

    // Project rays through each pixel
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            Math::Vec3 ray = cam.GenerateRay(i, j);
            ray.
        }
    }

    return 0;
}
