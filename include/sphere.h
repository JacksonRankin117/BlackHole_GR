#pragma once

#include "hittable.h"
#include "material.h"

class Sphere : public Hittable {
public:

    Sphere(const Math::Vec4& center,
           double radius,
           const Material* mat) noexcept
        : s_center(center),
          s_radius(radius),
          s_mat(mat) {}

    bool Intersect(const Ray& ray,
                   double lambda_min,
                   double lambda_max,
                   HitRecord& rec) const noexcept override
    {
        Math::Vec3 pos = Spatial(ray.r_pos);
        Math::Vec3 center = Spatial(s_center);

        Math::Vec3 diff = pos - center;

        double dist2 = Math::Vec3::Dot(diff, diff);

        if (dist2 > s_radius * s_radius)
            return false;

        rec.lambda = 0.0;
        rec.point  = ray.r_pos;

        rec.normal = diff / s_radius;
        rec.mat    = s_mat;

        return true;
    }

private:

    static Math::Vec3 Spatial(const Math::Vec4& v) noexcept
    {
        return {v.X, v.Y, v.Z};
    }

    Math::Vec4        s_center;
    double            s_radius;
    const Material*   s_mat;
};
