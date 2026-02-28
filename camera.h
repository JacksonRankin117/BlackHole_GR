#include "math.h"
#include "ray.h"

class Camera {
    public:
        //
        Camera(int res_x, int res_y, double FOV_deg, Math::Vec3 position, Math::Vec3 target, Math::Vec3 up_hint = {0, 1, 0})
            : width(res_x), height(res_y),pos(position), FOV(FOV_deg * M_PI / 180.0)
        {
            BuildBasis(target, up_hint);
        }

        Ray GenerateRay(int px, int py) const
        {
            // --- Normalized device coordinates ---
            double u = (px + 0.5) / width;
            double v = (py + 0.5) / height;

            // --- Image plane ---
            double x = 2.0 * u - 1.0;
            double y = 1.0 - 2.0 * v;

            double aspect = double(height) / width;
            y *= aspect;

            double scale = std::tan(FOV * 0.5);
            x *= scale;
            y *= scale;

            // --- Direction in world space ---
            Math::Vec3 dir3 =
                forward +
                x * right +
                y * up;

            dir3 = dir3.Normalize();

            // --- Convert to Vec4 ---
            Math::Vec4 origin4(pos.X, pos.Y, pos.Z, 1.0);  // point
            Math::Vec4 dir4(dir3.X, dir3.Y, dir3.Z, 0.0);  // direction

            return Ray(origin4, dir4);
        }

    private:
        int width;
        int height;
        double FOV;
        Math::Vec3 pos;

        // Camera basis
        Math::Vec3 forward;
        Math::Vec3 right;
        Math::Vec3 up;

        void BuildBasis(const Math::Vec3& target, Math::Vec3 worldUp)
        {
            forward = (target - pos).Normalize();

            // Handle degeneracy when looking parallel to worldUp
            if (std::abs(Math::Vec3::Dot(forward, worldUp)) > 0.999)
                worldUp = {1, 0, 0};

            right = (Math::Vec3::Cross(forward, worldUp)).Normalize();
            up    = Math::Vec3::Cross(right, forward);
        }
};
