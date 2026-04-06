#pragma once

#include <memory>

#include "math_objects.h"
#include "ray.h"

class Material;

// ----------------------------------------------------- HitRecord -----------------------------------------------------
#include "color.h"

struct HitRecord {
    Math::Vec4 point;
    Math::Vec3 normal;
    double     lambda;
    double     coverage      = 1.0;
    Color      emission      = {};
    double     emissionScale = 0.0;
    std::shared_ptr<Material> mat = nullptr;
};

// ----------------------------------------------------- Hittable ------------------------------------------------------
class Hittable {
public:
    virtual ~Hittable() = default;

    // Did we hit it or not?
    [[nodiscard]] virtual bool Intersect(const Ray& ray,
                                         double lambda_min, double lambda_max,
                                         HitRecord& rec) const noexcept = 0;
};
