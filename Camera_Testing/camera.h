#include "ray.h"

class Camera {
    public:
        explicit Camera(double res_x = 1920, double res_y = 1080, double FOV = 20.0, Vec3 pos = {-100, 20, 0})
            : c_res_x(res_x), c_res_y(res_y), c_FOV(FOV), c_pos(pos)
        {
            // Look at the origin, where the black hole is
            LookAt(Vec3{0,0,0});
        }

        // Make the camera look at a particular area
        void LookAt(const Vec3& target) {
            // Return a normalized 3D direction vector
            c_look = (target - c_pos).Normalize();
        }
        
        // Cast a ray dependant on pixel
        Ray CastRay(double ix, double iy) {
            double aspect_ratio = c_res_y / c_res_x;
        }

    private:
        // Camera resoluton in pixels
        double c_res_x;
        double c_res_y;

        // Field-Of-View (degrees)
        double c_FOV;

        // Position of the camera
        Vec3 c_pos;

        // Direction vector
        Vec3 c_look;
};