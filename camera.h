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
            double u = (px + 0.5) / width;
            double v = (py + 0.5) / height;

            double x = 2.0 * u - 1.0;
            double y = 1.0 - 2.0 * v;

            double aspect = double(width) / height;  // <--- fix
            x *= aspect;

            double scale = std::tan(FOV * 0.5);
            x *= scale;
            y *= scale;

            Math::Vec3 dir3 = (forward + x*right + y*up).Normalize();

            return Ray(
                Math::Vec4(1.0, pos.X, pos.Y, pos.Z),
                Math::Vec4(1.0, dir3.X, dir3.Y, dir3.Z)
            );
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
