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
        width = file.dataWindow().max.x - file.dataWindow().min.x + 1;
        height = file.dataWindow().max.y - file.dataWindow().min.y + 1;

        pixels.resizeErase(height, width);
        file.setFrameBuffer(&pixels[0][0] - file.dataWindow().min.x - file.dataWindow().min.y * width, 1, width);
        file.readPixels(file.dataWindow().min.y, file.dataWindow().max.y);
    }

    Color Sample(const Math::Vec3& dir) const {
        // Convert 3D direction to spherical coordinates
        double theta = std::acos(dir.Y);              // 0..pi
        double phi   = std::atan2(dir.Z, dir.X);      // -pi..pi
        if (phi < 0) phi += 2*M_PI;                  // 0..2pi

        // Map to pixel coordinates
        double u = phi / (2*M_PI);
        double v = theta / M_PI;

        int x = std::min(int(u * width), width - 1);
        int y = std::min(int(v * height), height - 1);

        const Imf::Rgba& px = pixels[y][x];
        return Color(px.r, px.g, px.b); // Assuming px values are 0..1
    }
};
