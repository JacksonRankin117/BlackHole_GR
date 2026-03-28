#pragma once

#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImfRgba.h>
#include <OpenEXR/ImfArray.h>
#include <cmath>

#include "color.h"
#include "math_objects.h"


class StarMap {
public:
    // Rendered dimensions
    int width, height;

    // Pixel values
    Imf::Array2D<Imf::Rgba> pixels;

    // ------------------------------------- StarMap Construction from an OpenEXR --------------------------------------
    StarMap(const char* filename) {
        // Open the file and read the pixel data into the array
        Imf::RgbaInputFile file(filename);

        // Read the data window dimensions
        int dw_min_x = file.dataWindow().min.x;
        int dw_min_y = file.dataWindow().min.y;
        int dw_max_x = file.dataWindow().max.x;
        int dw_max_y = file.dataWindow().max.y;

        // Resize the pixel array to match the data window
        width  = dw_max_x - dw_min_x + 1;
        height = dw_max_y - dw_min_y + 1;

        // Initialize the pixel array with default values
        pixels.resizeErase(height, width);

        // frameBuffer expects pointer to the first pixel in memory
        file.setFrameBuffer(&pixels[0][0] - dw_min_x - dw_min_y * width, 1, width);

        // Read the pixel data into the array
        file.readPixels(dw_min_y, dw_max_y);
    }

    // ----------------------------------------------- Background Color ------------------------------------------------
    Color Sample(const Math::Vec3& dir) const
    {
        // Declination of the star map
        double theta = std::acos(std::clamp(dir.Z, -1.0, 1.0));

        // Right Ascension
        double phi = std::atan2(-dir.X, dir.Y);

        // Wrap phi to [0, 2*M_PI)
        if (phi < 0) phi += 2*M_PI;

        // Map to texture coordinates
        double u = phi / (2*M_PI);
        double v = theta / M_PI;

        // Clamp to [0, 1] and convert to pixel indices
        int x = std::clamp(int(u * width), 0, width - 1);
        int y = std::clamp(int(v * height), 0, height - 1);

        // Convert pixel color to Color
        const Imf::Rgba& px = pixels[y][x];

        // Return the color
        return Color(px.r, px.g, px.b);
    }
};
