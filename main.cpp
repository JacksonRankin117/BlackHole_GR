#include <iostream>
#include <vector>
#include <cmath>

#include "camera.h"
#include "color.h"
#include "ray.h"
#include "sphere.h"
#include "math.h"

using namespace Math;

int main(int argc, char* argv[])
{
    // ---------------- Image Settings ----------------
    const int width  = 1920;
    const int height = 1080;

    std::vector<Color> image;
    image.reserve(width * height);

    // ---------------- Camera ----------------
    Camera cam(
        width,
        height,
        90.0,
        {-100.0, 50.0, 20.0},   // camera position
        {0.0, 0.0, 0.0},       // look at origin
        {0.0, 1.0, 0.0}        // up direction
    );

    // ---------------- Sphere ----------------
    Sphere sphere(
        Vec4{0.0, 0.0, 0.0, 0.0},  // spacetime center
        30.0,
        nullptr                    // ignore material
    );

    // ---------------- Render ----------------
    double last_progress = -1.0;

    for (int i = 0; i < height; i++)
    {
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

            if (sphere.Intersect(ray, 0.001, 1e30, rec))
            {
                Vec3 normal = rec.normal;

                pixel = {
                    0.5f * ((float)normal.X + 1.0f),
                    0.5f * ((float)normal.Y + 1.0f),
                    0.5f * ((float)normal.Z + 1.0f)
                };
            } else {
                // Sky gradient
                Vec3 unit_dir = (Vec3{ ray.r_direct.X, ray.r_direct.Y, ray.r_direct.Z}).Normalize();

                float t = 0.5f * (unit_dir.Y + 1.0f);

                pixel =
                    (1.0f - t) * Color{1.0f, 1.0f, 1.0f}
                  + t * Color{0.5f, 0.7f, 1.0f};
            }

            image.push_back(pixel);
        }
    }

    std::cout << "\rProgress: 100.0%" << std::endl;

    // ---------------- Save Image ----------------
    Color::SaveImage("image.pfm", width, height, image);

    return 0;
}
