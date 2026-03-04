#pragma once

#include "ray.h"

class Material;

struct HitRecord {
    Math::Vec4 point;      // spacetime hit point
    Math::Vec3 normal;     // spatial surface normal
    double     lambda;
    const Material* mat;
};

class Hittable {
public:
    virtual ~Hittable() = default;

    [[nodiscard]]
    virtual bool Intersect(const Ray& ray,
                           double lambda_min,
                           double lambda_max,
                           HitRecord& rec) const noexcept = 0;
};
