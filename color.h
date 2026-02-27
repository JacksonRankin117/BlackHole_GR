#pragma once

#include <iostream>
#include <fstream>
#include <vector>

struct Pixel {
    float r, g, b;
};

void savePFM(const char* filename, int width, int height, const std::vector<Pixel>& data) {
    std::ofstream ofs(filename, std::ios::binary);

    // Write Header: PF (Color), Width Height, Aspect/Endianness
    ofs << "PF\n" << width << " " << height << "\n-1.0\n";

    // Write Binary Data
    // We cast the pointer to char* to write raw bytes
    ofs.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(Pixel));

    ofs.close();
    std::cout << "PFM file saved to " << filename << std::endl;
}
/*
 * EXAMPLE USAGE: (Makes a gradient)
 * ---------------------------------------------------------------------------------------------------------------------
 * 
 *  int main() {
 *      int width = 512;
 *      int height = 512;
 *      std::vector<Pixel> pixels(width * height);
 *
 *      // Generate a simple gradient
 *      for (int y = 0; y < height; ++y) {
 *          for (int x = 0; x < width; ++x) {
 *              int index = y * width + x;
 *              // Normalized coordinates 0.0 to 1.0
 *              pixels[index].r = (float)x / width;
 *              pixels[index].g = (float)y / height;
 *              pixels[index].b = 0.5f; 
 *          }
 *      }
 *
 *      savePFM("output.pfm", width, height, pixels);
 *      return 0;
 *  }
 */