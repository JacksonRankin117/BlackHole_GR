// include/sphere.h

#pragma once

#include <memory>

#include "hittable.h"
#include "material.h"

// Yep, this is a sphere alright
class Sphere : public Hittable {
public:
    // Constructor
    Sphere(const Math::Vec4& center, double radius, std::shared_ptr<Material> mat) noexcept
        : s_center(center), s_radius(radius), s_mat(std::move(mat)) {}

    // ----------------------------------------------- Ray intersection ------------------------------------------------
    bool Intersect(const Ray& ray, double lambda_min, double lambda_max, HitRecord& rec) const noexcept override
    {
        Math::Vec3 pos    = Spatial(ray.origin);
        Math::Vec3 center = Spatial(s_center);
        Math::Vec3 dir    = ray.Direction().Normalized();

        Math::Vec3 oc = pos - center;

        double a = Math::Vec3::Dot(dir, dir);
        double b = 2.0 * Math::Vec3::Dot(oc, dir);
        double c = Math::Vec3::Dot(oc, oc) - s_radius * s_radius;

        double discriminant = b * b - 4.0 * a * c;

        // Soft edge: blend over a narrow band around discriminant == 0.
        // edge_width controls the feather width in world-space units squared.
        double edge_width = s_radius * s_radius * 0.01;
        if (discriminant < -edge_width)
            return false;

        // Coverage goes from 0 (fully outside) to 1 (fully inside)
        rec.coverage = std::clamp((discriminant + edge_width) / (2.0 * edge_width), 0.0, 1.0);

        double sqrt_disc = std::sqrt(std::max(discriminant, 0.0));
        double t = (-b - sqrt_disc) / (2.0 * a);
        if (t < lambda_min || t > lambda_max) {
            t = (-b + sqrt_disc) / (2.0 * a);
            if (t < lambda_min || t > lambda_max)
                return false;
        }

        rec.lambda   = t;
        rec.point    = ray.origin;
        Math::Vec3 hit_point = pos + t * dir;
        rec.normal   = (hit_point - center) / s_radius;
        rec.mat      = s_mat;
        return true;
    }

private:
    // Returns the spatial coordinates of a Vec4 point
    private:
        static Math::Vec3 Spatial(const Math::Vec4& v) noexcept
        {
            return {v.X, v.Y, v.Z};
        }

    Math::Vec4                s_center;
    double                    s_radius;
    std::shared_ptr<Material> s_mat;
};
