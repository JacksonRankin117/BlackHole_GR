#pragma once

class Ray {
public:
    Math::Vec4 origin;
    Math::Vec4 momentum;

    Ray(const Math::Vec4& o, const Math::Vec4& k)
        : origin(o), momentum(k) {}

    Math::Vec3 Direction() const
    {
        return Math::Vec3(momentum.X, momentum.Y, momentum.Z).Normalized();
    }
};