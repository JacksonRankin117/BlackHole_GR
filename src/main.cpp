#include <iostream>

#include "camera.h"
#include "color.h"
#include "ray.h"
#include "sphere.h"
#include "math.h"
#include "starmap.h"

using namespace Math;

int main(int argc, char* argv[])
{
    // ------------------------------------------------ Image Settings -------------------------------------------------
    const int width  = 1920;
    const int height = 1080;

    std::vector<Color> image;
    image.reserve(width * height);

    // ---------------------------------------------------- Camera -----------------------------------------------------
    Camera cam(
        width, height,          // resolution of the image
        75.0,                  // field of view in degrees
        {150.0, 150.0, 800.0},  // camera position
        {0.0, 0.0, 0.0},        // look at origin
        {0.0, 0.0, 1.0}         // up direction
    );

    // ---------------------------------------------------- Scene -----------------------------------------------------
    Sphere sphere(
        Vec4{0.0, 0.0, 0.0, 0.0},  // spacetime center
        30.0,                      // radius
        nullptr                    // ignore material
    );

    StarMap star_map("StarMaps/starmap_2020_8k.exr");

    // ---------------------------------------------------- Render -----------------------------------------------------
    double last_progress = -1.0;

    for (int i = 0; i < height; i++)
    {
        // Progress bar
        double progress = std::round(1000.0 * i / height) / 10.0;
        if (progress != last_progress)
        {
            std::cout << "\rProgress: " << progress << "%" << std::flush;
            last_progress = progress;
        }

        for (int j = 0; j < width; j++)
        {
            Ray ray = cam.GenerateRay(j, i);

            double lambda;
            Color pixel;

            HitRecord rec;

            //---------------------------------------------- Pixel logic -----------------------------------------------
            if (sphere.Intersect(ray, 0.001, 1e30, rec))
            {
                Vec3 normal = rec.normal;

                pixel = {
                    0.5f * ((float)normal.X + 1.0f),
                    0.5f * ((float)normal.Y + 1.0f),
                    0.5f * ((float)normal.Z + 1.0f)
                };

            }
            else
            {
                // Sample from star map
                Vec3 dir = Vec3{ ray.r_direct.X, ray.r_direct.Y, ray.r_direct.Z }.Normalized();
                pixel = star_map.Sample(dir);

                // simple HDR tone map
                float exposure = 5.0f;
                pixel.r = 1.0f - std::exp(-exposure * pixel.r);
                pixel.g = 1.0f - std::exp(-exposure * pixel.g);
                pixel.b = 1.0f - std::exp(-exposure * pixel.b);
            }

            image.push_back(pixel);
        }
    }

    std::cout << "\rProgress: 100.0%" << std::endl;

    //--------------------------------------------------- Save Image ---------------------------------------------------
    Color::SaveImage("image.pfm", width, height, image);

    //--------------------------------------------------- Debugging ----------------------------------------------------
    cam.DebugProject({0, 30, 60});

    return 0;
}
