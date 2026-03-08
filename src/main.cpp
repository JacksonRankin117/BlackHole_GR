#include <iostream>
#include <vector>
#include "camera.h"
#include "color.h"
#include "starmap.h"
#include "math.h"
#include "blackhole.h"

int main()
{
    // --------------------- Image Settings ---------------------
    const int width  = 1920;   // small for quick test
    const int height = 1080;

    std::vector<Color> image;
    image.reserve(width * height);

    // ---------------------- Camera ----------------------------
    Math::Vec3 camPos  = {0.0, BlackHole::AU, 0};
    Math::Vec3 target  = {0.0, 0.0, 0.0};
    Math::Vec3 upVec   = {0.0, 0.0, 1.0};
    Camera cam(width, height, 45.0, camPos, target, upVec);

    // ---------------------- Star Map --------------------------
    StarMap star_map("StarMaps/starmap_2020_16k.exr");
    BlackHole::Schwarzschild dummySpacetime(1.0, {0, 0, 0}); // mass = 1.0


    // ---------------------- Render ----------------------------
    for(int i = 0; i < height; ++i)
    {
        double progress = 100.0 * i / height;
        std::cout << "\rProgress: " << progress << "%" << std::flush;

        for(int j = 0; j < width; ++j)
        {
            Ray r = cam.GenerateRay(j, i);
            Math::Vec3 dir = r.direction;

            Color pixel = star_map.Sample(dir);

            // Tone mapping
            float exposure = 20.0f;
            pixel.r = 1.0f - std::exp(-exposure * pixel.r);
            pixel.g = 1.0f - std::exp(-exposure * pixel.g);
            pixel.b = 1.0f - std::exp(-exposure * pixel.b);

            image.push_back(pixel);
        }
    }
    std::cout << "\rProgress: 100.0%" << std::endl;
    std::cout << "\rProgress: 100.0%" << std::endl;

    // ---------------------- Save ------------------------------
    Color::SaveImage("imge.pfm", width, height, image);

    return 0;
}
