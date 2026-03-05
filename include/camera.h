#include "math.h"
#include "ray.h"

class Camera {
    public:
        //--------------------------------------------- Default constructor --------------------------------------------
        Camera(int res_x, int res_y,            // Resolution of the image, i.e. 1920x1080
               double FOV_deg,                  // field of view in degrees
               Math::Vec3 position,             // camera position
               Math::Vec3 target,               // target position
               Math::Vec3 up_hint = {0, 0, 1})  // up direction hint.
             : width(res_x), height(res_y),
               pos(position),
               FOV(FOV_deg * M_PI / 180.0)
        {
            // Build camera basis from target and up hint
            BuildBasis(target, up_hint);
        }
        //-------------------------------- Generate a ray for a given pixel coordinate ---------------------------------
        Ray GenerateRay(int px, int py) const
        {
            // u and v are the normalized pixel coordinates in [0, 1]
            double u = (px + 0.5) / width;
            double v = (py + 0.5) / height;

            // Convert u and v to camera coordinates (x, y)
            double x = 1.0 - 2.0 * u;
            double y = 1.0 - 2.0 * v;

            // Apply aspect ratio correction
            double aspect = double(width) / height;
            x *= aspect;

            // Apply field of view scaling
            double scale = std::tan(FOV * 0.5);
            x *= scale;
            y *= scale;

            // Convert to 3D direction vector
            Math::Vec3 dir3 = (forward + x*left + y*up).Normalize();

            // Return the generated ray
            return Ray(
                Math::Vec4(1.0, pos.X, pos.Y, pos.Z),    // This is the camera position, and the origin of the ray
                Math::Vec4(1.0, dir3.X, dir3.Y, dir3.Z)  // This is the direction of the ray, which has been normalized
            );
        }

    private:
        //--------------------------------------------- Camera parameters ----------------------------------------------
        int width;       // Image width, i.e. 1920
        int height;      // Image height, i.e. 1080
        double FOV;      // Field of view in degrees
        Math::Vec3 pos;  // Camera position, and the origin of each ray

        // Camera basis vectors
        Math::Vec3 forward;  // Camera forward direction
        Math::Vec3 left;     // Camera right direction
        Math::Vec3 up;       // Camera up direction

        //-------------------------------------- Camera Basis Vector Calculation ---------------------------------------
        void BuildBasis(const Math::Vec3& target, Math::Vec3 worldUp)
        {
            // Compute forward direction
            forward = (target - pos).Normalize();

            // Handle degeneracy when looking parallel to worldUp
            if (std::abs(Math::Vec3::Dot(forward, worldUp)) > 0.999)
                worldUp = {1, 0, 0};

            //
            left = Math::Vec3::Cross(worldUp, forward).Normalize();
            up   = Math::Vec3::Cross(forward, left);
        }
        //------------------------------------------ Camera Debug Projection -------------------------------------------
        public:
            void DebugProject(const Math::Vec3& worldPoint) const
            {
                Math::Vec3 rel = worldPoint - pos;

                std::cout << "Dot forward: "
                          << Math::Vec3::Dot(rel, forward) << "\n";

                std::cout << "Dot left: "
                          << Math::Vec3::Dot(rel, left) << "\n";

                std::cout << "Dot up: "
                          << Math::Vec3::Dot(rel, up) << "\n";
            }
};
