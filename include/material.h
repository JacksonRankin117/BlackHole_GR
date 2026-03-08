#pragma once

#include "color.h"

class Ray;
struct HitRecord;

class Material {
public:
    virtual ~Material() = default;

    [[nodiscard]]
    virtual Color Shade(const Ray& ray,
                        const HitRecord& hit) const noexcept = 0;
};

class OneColor : public Material {
public:
    explicit OneColor(Color color) noexcept
        : c(color) {}

    Color Shade(const Ray&,
                const HitRecord&) const noexcept override
    {
        return c;
    }

private:
    Color c;
};

class Blackbody : public Material {
public:
    explicit Blackbody(double temperature) noexcept
        : T(temperature) {}

    Color Shade(const Ray&,
                const HitRecord&) const noexcept override
    {
        return Color::FromBlackbody(T);
    }

private:
    double T;
};
