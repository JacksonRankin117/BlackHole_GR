#pragma once
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>

struct Color {
    float r, g, b;

    constexpr Color() noexcept : r(0), g(0), b(0) {}
    constexpr Color(float r_, float g_, float b_) noexcept : r(r_), g(g_), b(b_) {}

    // ------------------------------------------------- Arithmetic ----------------------------------------------------
    constexpr Color operator+(const Color& o) const noexcept { return {r+o.r, g+o.g, b+o.b}; }
    constexpr Color& operator+=(const Color& o) noexcept { r+=o.r; g+=o.g; b+=o.b; return *this; }
    constexpr Color operator*(const Color& o) const noexcept { return {r*o.r, g*o.g, b*o.b}; }
    constexpr Color operator*(float s)        const noexcept { return {r*s,   g*s,   b*s};   }
    constexpr Color operator/(float s)        const noexcept { return {r/s,   g/s,   b/s};   }
    friend constexpr Color operator*(float s, const Color& c) noexcept { return c*s; }

    // ---------------------------------------------------- Clamp ------------------------------------------------------
    static constexpr float Clamp(float x, float lo, float hi) noexcept {
        return x < lo ? lo : x > hi ? x : x;
    }
    void Clamp(float lo = 0.f, float hi = 1.f) noexcept {
        r = Clamp(r, lo, hi);
        g = Clamp(g, lo, hi);
        b = Clamp(b, lo, hi);
    }

    // ------------------------------------------------ Blackbody ------------------------------------------------------
    // Tanner Helland (2012) — perceptually correct sRGB for display.
    // Valid 1000–40000 K. Returns values in [0,1] — no HDR scaling,
    // let the emissive material strength and exposure handle brightness.
    static Color FromBlackbody(double T_kelvin) noexcept {
        double T = std::clamp(T_kelvin, 1000.0, 40000.0) / 100.0;

        double r, g, b;

        if (T <= 66.0)
            r = 1.0;
        else
            r = std::clamp(329.698727446  * std::pow(T - 60.0, -0.1332047592) / 255.0, 0.0, 1.0);

        if (T <= 66.0)
            g = std::clamp((99.4708025861 * std::log(T) - 161.1195681661) / 255.0, 0.0, 1.0);
        else
            g = std::clamp(288.1221695283 * std::pow(T - 60.0, -0.0755148492) / 255.0, 0.0, 1.0);

        if (T >= 66.0)
            b = 1.0;
        else if (T <= 19.0)
            b = 0.0;
        else
            b = std::clamp((138.5177312231 * std::log(T - 10.0) - 305.0447927307) / 255.0, 0.0, 1.0);

        return Color{static_cast<float>(r), static_cast<float>(g), static_cast<float>(b)};
    }
    // ------------------------------------------------- Save PFM ------------------------------------------------------
    static void SaveImage(const char* filename, int width, int height,
                          const std::vector<Color>& data)
    {
        std::ofstream ofs(filename, std::ios::binary);
        ofs << "PF\n" << width << " " << height << "\n-1.0\n";
        for (int y = height - 1; y >= 0; --y) {
            const Color* row = &data[y * width];
            ofs.write(reinterpret_cast<const char*>(row), width * sizeof(Color));
        }
        ofs.close();
        std::cout << "\nPFM file saved to " << filename << std::endl;
    }
    // ------------------------------------------------- Save PPM ------------------------------------------------------
    // Writes a P3 (plain-text) PPM file. Expects pixel values already in
    // [0, 1] linear sRGB (i.e. after tone mapping and gamma correction).
    // Values are clamped and quantized to 8-bit (0–255).
    static void SavePPM(const char* filename, int width, int height,
                        const std::vector<Color>& data)
    {
        std::ofstream ofs(filename);
        ofs << "P3\n" << width << " " << height << "\n255\n";
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const Color& c = data[y * width + x];
                int ri = static_cast<int>(std::clamp(c.r, 0.f, 1.f) * 255.999f);
                int gi = static_cast<int>(std::clamp(c.g, 0.f, 1.f) * 255.999f);
                int bi = static_cast<int>(std::clamp(c.b, 0.f, 1.f) * 255.999f);
                ofs << ri << ' ' << gi << ' ' << bi << '\n';
            }
        }
        ofs.close();
        std::cout << "\nPPM file saved to " << filename << std::endl;
    }
};