#pragma once

#include "hittable.h"
#include "material.h"

// Yep, this is a sphere alright
class Sphere : public Hittable {
public:
    // Constructor
    Sphere(const Math::Vec4& center, double radius, const Material* mat) noexcept
        : s_center(center), s_radius(radius), s_mat(mat) {}

    // ----------------------------------------------- Ray intersection ------------------------------------------------
    bool Intersect(const Ray& ray, double lambda_min, double lambda_max, HitRecord& rec) const noexcept override
    {
        //
        Math::Vec3 pos = Spatial(ray.origin);
        Math::Vec3 center = Spatial(s_center);

        Math::Vec3 diff = pos - center;

        double dist2 = Math::Vec3::Dot(diff, diff);

        if (dist2 > s_radius * s_radius)
            return false;

        rec.lambda = 0.0;
        rec.point  = ray.origin;

        rec.normal = diff / s_radius;
        rec.mat    = s_mat;

        return true;
    }

private:
    // Returns the spatial coordinates of a Vec4 point
    static Math::Vec3 Spatial(const Math::Vec4& v) noexcept
    {
        return {v.X, v.Y, v.Z};
    }

    Math::Vec4        s_center;  // Center of the sphere in Minkowski space
    double            s_radius;  // Radius of the sphere in Minkowski space
    const Material*   s_mat;     // Material of the sphere
};
