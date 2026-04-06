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
        double theta = std::acos(std::clamp(dir.Z, -1.0, 1.0));
        double phi   = std::atan2(dir.Y, dir.X);
        if (phi < 0) phi += 2 * M_PI;

        double u = phi / (2 * M_PI);
        double v = theta / M_PI;

        // Bilinear sample with φ-wrapping
        double fx = u * width  - 0.5;
        double fy = v * height - 0.5;

        int x0 = static_cast<int>(std::floor(fx));
        int y0 = static_cast<int>(std::floor(fy));
        int x1 = x0 + 1;
        int y1 = y0 + 1;

        double tx = fx - x0;
        double ty = fy - y0;

        // Wrap x (φ is periodic), clamp y (θ has poles)
        x0 = ((x0 % width) + width) % width;
        x1 = ((x1 % width) + width) % width;
        y0 = std::clamp(y0, 0, height - 1);
        y1 = std::clamp(y1, 0, height - 1);

        auto get = [&](int x, int y) -> Color {
            const Imf::Rgba& px = pixels[y][x];
            return Color(px.r, px.g, px.b);
        };

        Color c00 = get(x0, y0);
        Color c10 = get(x1, y0);
        Color c01 = get(x0, y1);
        Color c11 = get(x1, y1);

        // Bilinear interpolation
        float ftx = static_cast<float>(tx);
        float fty = static_cast<float>(ty);

        Color top    = c00 * (1.0f - ftx) + c10 * ftx;
        Color bottom = c01 * (1.0f - ftx) + c11 * ftx;
        return top * (1.0f - fty) + bottom * fty;
    }
};
