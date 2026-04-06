#pragma once

class Ray {
public:
    Math::Vec4 origin;
    Math::Vec4 momentum;

    // Initializes a zero-ray
    Ray() : origin(Vec4()), momentum(Vec4()) {}

    Ray(const Math::Vec4& o, const Math::Vec4& k)
        : origin(o), momentum(k) {}

    Math::Vec3 Direction() const
    {
        Math::Vec3 d(momentum.X, momentum.Y, momentum.Z);
        double len = d.Magnitude();

        if (len < 1e-12)
            return Math::Vec3(0, 0, 0);

        return d / len;
    }
};