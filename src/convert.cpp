/*

#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>

struct Color { float r, g, b; };


int main() {
    std::ifstream in("output.pfm", std::ios::binary);
    if (!in) { std::cerr << "Open failed\n"; return 1; }

    std::string header;
    std::getline(in, header);            // PF

    std::string dims;
    std::getline(in, dims);              // width height
    int width, height;
    sscanf(dims.c_str(), "%d %d", &width, &height);

    std::string scale_line;
    std::getline(in, scale_line);        // scale
    float scale = std::stof(scale_line);

    std::vector<Color> pixels(width * height);
    in.read(reinterpret_cast<char*>(pixels.data()),
            pixels.size() * sizeof(Color));
    in.close();

    // Replace red with black (tolerant)
    const float eps = 1e-4f;
    int replaced = 0;

    for (auto& c : pixels) {
        if (fabs(c.r - 1.0f) < eps &&
            fabs(c.g) < eps &&
            fabs(c.b) < eps)
        {
            c.r = c.g = c.b = 0.0f;
            replaced++;
        }
    }

    std::cout << "Replaced " << replaced << " pixels\n";

    std::ofstream out("bh_render_fixed.pfm", std::ios::binary);
    out << header << "\n";
    out << width << " " << height << "\n";
    out << scale << "\n";
    out.write(reinterpret_cast<char*>(pixels.data()),
              pixels.size() * sizeof(Color));
}
*/
