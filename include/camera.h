#pragma once

#include "math_objects.h"
#include "ray.h"
//#include "blackhole.h"

// ------------------------------------------------------ Camera -------------------------------------------------------
struct CameraBasis {
    Vec3 right;
    Vec3 up;
};

class Camera {
public:
    Camera(int res_x, int res_y,                 // Resolution
           double fov_deg,                       // Field of view in degrees
           const Math::Vec3& pos,                // Camera position
           const Math::Vec3& target,             // Target position
           const Math::Vec3& up_hint = {0,0,1})  // Default up vector is +Z
        : width(res_x), height(res_y), position(pos)
    {
        FOV = fov_deg * M_PI / 180.0;  // Convert to radians
        BuildBasis(target, up_hint);   // Build camera basis
    }

    // Assume Minkowski spacetime
    Ray GenerateRay(double px, double py) const
    {
        double u = px / width;
        double v = py / height;

        double aspect = double(width) / double(height);
        double scale  = std::tan(FOV * 0.5);

        double x = (2.0 * u - 1.0) * aspect * scale;
        double y = (1.0 - 2.0 * v) * scale;

        Vec3 dir = (forward + x * right + y * up).Normalized();

        return Ray({1.0, position.X, position.Y, position.Z},
                   {1.0, dir.X,      dir.Y,      dir.Z     });
    }

    // Return the camera position
    Math::Vec3 Position() const { return position; }

private:
    int width, height;
    double FOV;
    Math::Vec3 position;

    Math::Vec3 forward;
    Math::Vec3 right;
    Math::Vec3 up;

    void BuildBasis(const Math::Vec3& target, const Math::Vec3& worldUp) {
        // Compute forward direction
        forward = (target - position).Normalized();

        // Compute right vector using worldUp
        right = Math::Vec3::Cross(worldUp, forward).Normalized();

        // Compute up vector using forward and right
        up    = Math::Vec3::Cross(forward, right).Normalized();
    }

    Ray GenerateJitteredRay(int px, int py, uint32_t& seed, float angle_scale) const
    {
        Ray base = GenerateRay(px, py);

        Vec3 dir = base.Direction();

        CameraBasis basis = GetBasis(dir);

        auto rng = [](uint32_t& s) -> float {
            s ^= s << 13;
            s ^= s >> 17;
            s ^= s << 5;
            return (s & 0x00FFFFFFu) / float(0x01000000u);
        };

        float ax = rng(seed) - 0.5f;
        float ay = rng(seed) - 0.5f;

        Vec3 jittered =
            dir
            + ax * angle_scale * basis.right
            + ay * angle_scale * basis.up;

        jittered = jittered.Normalized();

        return Ray(base.origin, {1.0, jittered.X, jittered.Y, jittered.Z});
    }

    CameraBasis GetBasis(const Vec3& dir) const
    {
        Vec3 worldUp = {0, 1, 0};

        Vec3 right = Vec3::Cross(worldUp, dir);
        if (right.Magnitude() < 1e-6)
            right = Vec3::Cross({1, 0, 0}, dir);

        right = right.Normalized();
        Vec3 up = Vec3::Cross(dir, right);

        return { right, up };
    }
};
