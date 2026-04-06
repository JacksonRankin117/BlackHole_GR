// src/main.cpp

#include <vector>

#include "accretion_disk.h"
#include "blackhole.h"
#include "camera.h"
#include "color.h"
#include "geodesic_integrator.h"
#include "hittable_list.h"
#include "math_objects.h"
#include "starmap.h"
#include "stopwatch.h"
#include "sphere.h"
#include "thread_manager.h"

int main()
{
    // ================================================ Image Settings =================================================
    constexpr int width  = 800;  // Image width
    constexpr int height = 600;  // Image height

    // Render timer
    Stopwatch sw{60};

    // ===================================================== Scene =====================================================

    // Background
    StarMap star_map("StarMaps/starmap_2020_16k.exr");

    // Black hole
    double M = 7.5e5 * BlackHole::M_Solar;
    BlackHole::Schwarzschild schwarzschild(M, {0, 0, 0});

    // Holds objects
    HittableList world;
    
    // ------------------------------------------------ Accretion Disk -------------------------------------------------
    ///*
    double L_edd    = 1.26e31 * (M / BlackHole::M_Solar);           // ~1.26e37 W
    double eta      = 0.057;                                        // Schwarzschild radiative efficiency (~6%)
    double mdot_edd = L_edd / (eta * BlackHole::c * BlackHole::c);  // ~2.8e22 kg/s

    // Pick a fraction:
    double mdot = 0.2 * mdot_edd;    // 20% Eddington limit

    auto disk = std::make_shared<AccretionDisk>(
        schwarzschild,                        // Spacetime     
        Math::Vec4{0,0,0,0},                  // Center
        mdot,                                 // Mass Accretion rate
        17.5 * schwarzschild.EventHorizon(),  // Outer radius 
        1.7,                                  // Spectral hardnening factor
        0.00 * schwarzschild.ISCO()           // Slab half thickness (0.0 for infinitely thin)
    );

    world.Add(disk);

    //*/
    // ==================================================== Camera =====================================================
    Math::Vec3 camPos  = { 0.5  * BlackHole::AU,    // 0.5 AU along +x
                           0.0  * BlackHole::AU,    // 0.0 AU along +y
                           0.02 * BlackHole::AU};   // 0.02 AU along +z (To get a good view of accretion disk)
    
    Math::Vec3 target  = {0.0, 0.0, 0.0};        // Stare at the origin like some sort of freaking creep
    Math::Vec3 upVec   = {0.0, 0.0, 1.0};        // (0, 0, 1) aligns with the north celestial pole

    // Construct the camera
    Camera cam(width,   // Image width in pixels
               height,  // Image height in pixels
               45.0,    // FOV in degrees
               camPos,  // Position of the camera
               target,  // Position of the target of the camera. 
               upVec);  // What the camera thinks is "up"
    // Debug
    //Color test = Color::FromBlackbody(315000.0);
    //std::cout << "315K test: r=" << test.r << " g=" << test.g << " b=" << test.b << "\n";
    
    // ==================================================== Render =====================================================
    ThreadManager tm(width, height);
    
    auto framebuffer = tm.RenderThreaded(cam, width, height, schwarzschild, world, star_map, sw);

    Color::SaveImage("output.pfm", width, height, framebuffer);
    //Color::SavePPM("output.ppm", width, height, framebuffer);  // debug

    return 0;
}

