#pragma once

#include "math.h"
#include "ray.h"
//#include "blackhole.h"

// ---------------------------------- Camera ----------------------------------
class Camera {
public:
    Camera(int res_x, int res_y, double fov_deg,
           const Math::Vec3& pos, const Math::Vec3& target, const Math::Vec3& up_hint = {0,0,1})
        : width(res_x), height(res_y), position(pos)
    {
        FOV = fov_deg * M_PI / 180.0;
        BuildBasis(target, up_hint);
    }

    // Generate a ray for a pixel (world space)
    Ray GenerateRay(int px, int py) const {
        double u = (px + 0.5) / width;
        double v = (py + 0.5) / height;

        double aspect = double(width) / height;
        double scale = std::tan(FOV * 0.5);

        // Convert to NDC space [-1,1]
        double px_ndc = (2.0*u - 1.0) * aspect * scale;
        double py_ndc = (1.0 - 2.0*v) * scale;

        Math::Vec3 dir = (forward + px_ndc*right + py_ndc*up).Normalized();
        return Ray(position, dir);
    }

private:
    int width, height;
    double FOV;
    Math::Vec3 position;

    Math::Vec3 forward;
    Math::Vec3 right;
    Math::Vec3 up;

    void BuildBasis(const Math::Vec3& target, Math::Vec3 worldUp) {
        forward = (target - position).Normalized();

        // Avoid degenerate case
        if (std::abs(Math::Vec3::Dot(forward, worldUp)) > 0.999)
            worldUp = {1,0,0};

        right = Math::Vec3::Cross(forward, worldUp).Normalized();
        up    = Math::Vec3::Cross(right, forward);
    }
};
