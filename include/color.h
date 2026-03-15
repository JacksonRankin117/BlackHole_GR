#pragma once

#include <iostream>
#include <fstream>
#include <vector>

struct Color {
    float r, g, b;

    // ------------------------------------------------- Constructors --------------------------------------------------
    constexpr Color() noexcept : r(0), g(0), b(0) {}
    constexpr Color(float r_, float g_, float b_) noexcept
        : r(r_), g(g_), b(b_) {}

    void static SaveImage(const char* filename, int width, int height, const std::vector<Color>& data)
    {
        std::ofstream ofs(filename, std::ios::binary);

        // Write Header: PF (Color), Width Height, Aspect/Endianness
        ofs << "PF\n" << width << " " << height << "\n-1.0\n";

        // Write Binary Data
        // We cast the pointer to char* to write raw bytes
        for (int y = height - 1; y >= 0; --y)
        {
            const Color* row = &data[y * width];
            ofs.write(reinterpret_cast<const char*>(row),
                        width * sizeof(Color));
        }

        ofs.close();
        std::cout << "\nPFM file saved to " << filename << std::endl;
    }

    // ------------------------------------------------ Color Addition -------------------------------------------------
    constexpr Color operator+(const Color& other) const noexcept {
        return { r + other.r,
                 g + other.g,
                 b + other.b };
    }

    constexpr Color& operator+=(const Color& other) noexcept {
        r += other.r;
        g += other.g;
        b += other.b;
        return *this;
    }

    // -------------------------------------------- Component-wise Multiply --------------------------------------------
    constexpr Color operator*(const Color& other) const noexcept {
        return { r * other.r,
                 g * other.g,
                 b * other.b };
    }

    // ------------------------------------------------ Scalar Multiply ------------------------------------------------
    constexpr Color operator*(float s) const noexcept {
        return { r * s,
                 g * s,
                 b * s };
    }

    constexpr Color operator/(float s) const noexcept {
        return { r / s,
                 g / s,
                 b / s };
    }

    friend constexpr Color operator*(float s, const Color& c) noexcept {
        return c * s;
    }

    // ----------------------------------------------------- Clamp -----------------------------------------------------
    constexpr float Clamp(float x, float min_val, float max_val) noexcept
    {
        return (x < min_val) ? min_val : ((x > max_val) ? max_val : x);
    }

    void Clamp(float min_val = 0.0f, float max_val = 1.0f) noexcept {
        r = Clamp(r, min_val, max_val);
        g = Clamp(g, min_val, max_val);
        b = Clamp(b, min_val, max_val);
    }

    static Color FromBlackbody(double T);  // Edit later to grab a color based on temperature
};
