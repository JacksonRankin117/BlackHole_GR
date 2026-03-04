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
        Math::Vec3 oc  = Spatial(ray.r_origin) - Spatial(s_center);
        Math::Vec3 dir = Spatial(ray.r_direct);

        double a      = Math::Vec3::Dot(dir, dir);
        double half_b = Math::Vec3::Dot(oc, dir);
        double c      = Math::Vec3::Dot(oc, oc) - s_radius * s_radius;

        double discriminant = half_b * half_b - a * c;
        if (discriminant < 0.0)
            return false;

        double sqrtD = std::sqrt(discriminant);

        double root = (-half_b - sqrtD) / a;
        if (root < lambda_min || root > lambda_max) {
            root = (-half_b + sqrtD) / a;
            if (root < lambda_min || root > lambda_max)
                return false;
        }

        rec.lambda = root;
        rec.point  = ray.at(root);

        Math::Vec3 outward_normal =
            (Spatial(rec.point) - Spatial(s_center)) / s_radius;

        rec.normal = outward_normal;
        rec.mat    = s_mat;

        return true;
    }

private:
    static Math::Vec3 Spatial(const Math::Vec4& v) noexcept
    {
        return { v.X, v.Y, v.Z };
    }

    Math::Vec4        s_center;
    double            s_radius;
    const Material*   s_mat;
};
