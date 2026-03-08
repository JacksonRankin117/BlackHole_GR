#pragma once

#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImfRgba.h>
#include <OpenEXR/ImfArray.h>

#include <cmath>
#include "color.h"

class StarMap {
public:
    int width, height;
    Imf::Array2D<Imf::Rgba> pixels;

    StarMap(const char* filename) {
        Imf::RgbaInputFile file(filename);

        int dw_min_x = file.dataWindow().min.x;
        int dw_min_y = file.dataWindow().min.y;
        int dw_max_x = file.dataWindow().max.x;
        int dw_max_y = file.dataWindow().max.y;

        width  = dw_max_x - dw_min_x + 1;
        height = dw_max_y - dw_min_y + 1;

        pixels.resizeErase(height, width);

        // frameBuffer expects pointer to the first pixel in memory
        file.setFrameBuffer(&pixels[0][0] - dw_min_x - dw_min_y * width, 1, width);

        file.readPixels(dw_min_y, dw_max_y);
    }

    Color Sample(const Math::Vec3& dir) const
    {
        double theta = std::acos(dir.Z);         // vertical (declination)
        double phi   = std::atan2(dir.Y, dir.X); // horizontal (RA)

        if (phi < 0) phi += 2*M_PI;

        double u = 1.0 - phi / (2*M_PI); // RA increases left
        double v = 1.0 - theta / M_PI;

        int x = std::clamp(int(u * width), 0, width-1);
        int y = std::clamp(int(v * height), 0, height-1);

        const Imf::Rgba& px = pixels[y][x];
        // Debug
        //std::cout << "u=" << u << " v=" << v << " x=" << x << " y=" << y << "\n";

        return Color(px.r, px.g, px.b);
    }
};
