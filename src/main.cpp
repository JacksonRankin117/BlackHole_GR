//#include <atomic>
#include <iostream>
#include <vector>
#include <thread>
#include <functional>

#include "blackhole.h"
#include "camera.h"
#include "color.h"
#include "geodesic_integrator.h"
#include "math.h"
#include "starmap.h"
#include "stopwatch.h"
#include "thread_manager.h"


// Function declaration 
Color Render(const Camera& cam, int i, int j, const BlackHole::Spacetime& spacetime, const StarMap& starmap);
//Color RenderFAST(const Camera& cam, int i, int j, BlackHole::Spacetime& spacetime, const StarMap& starmap);

int main()
{
    // ------------------------------------------------ Image Settings -------------------------------------------------
    constexpr int width  = 3840;  // Image width
    constexpr int height = 2160;  // Image height

    const int size = width * height;  // Image resolution

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
    //auto star_map_ptr = std::make_shared<StarMap>("StarMaps/starmap_2020_16k.exr");

    // Black hole
    BlackHole::Schwarzschild Schwarzschild(1.0e6 * BlackHole::M_Solar, {0, 0, 0}); // mass = 1 million Solar masses

    // ------------------------------------------------ Multithreading -------------------------------------------------

    // Create a threadmanager object with the image's dimensions
    ThreadManager tm(width, height);  // Implicitly allocates the number of threads the program can use
    
    // Start the stopwatch
    sw.Start();

    // Debug code
    // std::cout << "Image size: " << image.size() << "\n";
    // std::cout << "Expected:   " << width * height << "\n";

    // Lambda function for rendering
    auto render_lambda = [&cam, &Schwarzschild, &star_map](int i, int j) {
        return Render(cam, i, j, static_cast<const BlackHole::Spacetime&>(Schwarzschild), star_map);
    };

    // Force the render to happen in a separate scope to prevent segfaults
    {
        // Render the image with the thread manager
        tm.RenderThreaded(
            render_lambda,  // pass the lambda
            image,
            sw
        );
    }

    // Stop the timer
    sw.Stop();
    sw.Finish();

    // ----------------------------------------------------- Save ------------------------------------------------------
    Color::SaveImage("output_1440p.pfm", width, height, image);

    return 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------- Function definitions ------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

// Standard render
Color Render(const Camera& cam, int i, int j, const BlackHole::Spacetime& spacetime, const StarMap& starmap) {
    // Ray generation and marching
    GeodesicState init = PhotonFromCamera(cam, j, i, spacetime);  // Generate Photon position and direction
    TraceResult result = TracePhotonAdaptive(init, spacetime);    // March the photon into the scene

    Color pixel;

    // Finds the color of the pixrl
    if (result.captured)
    {
        // pixel = {1.0f, 0.0f, 0.0f};  // Color it red if it falls in (debugging)
        pixel = {0.0f, 0.0f, 0.0f};  // Pixel should be black if it hits event horizon
    }
    else
    {
        pixel = SamplePhoton(result.state, starmap);  // Use the star map to color the pixel
    }

    // Linear tone map
    float exposure = 1.0f;
    pixel.r = 1.0f - std::exp(-exposure * pixel.r);
    pixel.g = 1.0f - std::exp(-exposure * pixel.g);
    pixel.b = 1.0f - std::exp(-exposure * pixel.b);

    // Gamma-correction
    float gamma = 2.2f;
    pixel.r = std::pow(pixel.r, 1.0f / gamma);
    pixel.g = std::pow(pixel.g, 1.0f / gamma);
    pixel.b = std::pow(pixel.b, 1.0f / gamma);

    return pixel;
}

// Render using 
Color RenderSchwarzschildFAST(const Camera& cam, int i, int j, BlackHole::Spacetime& spacetime, const StarMap& starmap) {


    return Color();
}
