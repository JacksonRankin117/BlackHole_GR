#pragma once

#include "math.h"
#include "ray.h"
//#include "blackhole.h"

// ------------------------------------------------------ Camera -------------------------------------------------------
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
    Ray GenerateRay(int px, int py) const
    {
        // Convert pixel coordinates to NDC space [-1,1]
        double u = (px + 0.5) / width;
        double v = (py + 0.5) / height;

        // Compute ray direction in NDC space
        double aspect = double(width) / height;
        double scale = std::tan(FOV * 0.5);

        // Convert NDC coordinates to ray direction
        double px_ndc = (2.0*u - 1.0) * aspect * scale;
        double py_ndc = (1.0 - 2.0*v) * scale;

        // Calculate the ray direction in world space
        Math::Vec3 dir = (forward + px_ndc*right + py_ndc*up).Normalized();

        // Return the ray
        return Ray({1.0, position.X, position.Y, position.Z}, {1.0,dir.X, dir.Y, dir.Z});
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
};
