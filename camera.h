class Camera {
    public:
        //
        Camera(int res_x, int res_y, double FOV_deg, Vec3 position, Vec3 target, Vec3 up_hint = {0, 1, 0})
            : width(res_x), height(res_y),pos(position), FOV(FOV_deg * M_PI / 180.0)
        {
            BuildBasis(target, up_hint);
        }
        
        // Generate a ray per pixel
        Vec3 GenerateRay(int px, int py) const
        {
            // Normalized device coordinates (pixel center)
            double u = (px + 0.5) / width;   // 0 → 1
            double v = (py + 0.5) / height;

            // Map to image plane
            double x = 2.0 * u - 1.0;        // -1 -> +1
            double y = 1.0 - 2.0 * v;        // +1 -> -1

            // Aspect ratio correction
            double aspect = double(height) / width;
            y *= aspect;

            // FOV scaling
            double scale = std::tan(FOV * 0.5);
            x *= scale;
            y *= scale;

            // World-space ray
            Vec3 dir =
                forward +
                x * right +
                y * up;

            return dir.Normalize();
        }

    private:
        int width;
        int height;
        double FOV;
        Vec3 pos;

        // Camera basis
        Vec3 forward;
        Vec3 right;
        Vec3 up;

        void BuildBasis(const Vec3& target, Vec3 worldUp)
        {
            forward = (target - pos).Normalize();

            // Handle degeneracy when looking parallel to worldUp
            if (std::abs(Vec3::Dot(forward, worldUp)) > 0.999)
                worldUp = {1, 0, 0};

            right = (Vec3::Cross(forward, worldUp)).Normalize();
            up    = Vec3::Cross(right, forward);
        }
};