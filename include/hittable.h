#pragma once

#include <memory>

#include "ray.h"

class Material;

// ----------------------------------------------------- HitRecord -----------------------------------------------------
struct HitRecord {
    Math::Vec4 point;     // Where the ray hit the hittable
    Math::Vec3 normal;    // spatial surface normal
    double lambda;        // distance along the ray to the hit point
   std::shared_ptr<Material> mat = nullptr;  // material at the hit point
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
