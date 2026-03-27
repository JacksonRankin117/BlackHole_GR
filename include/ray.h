#pragma once

#include "math.h"

#include <cmath>


class Ray {
public:
    Math::Vec4 origin;     // (t,x,y,z)
    Math::Vec4 momentum;   // null 4-momentum

    Ray(const Math::Vec4& o, const Math::Vec4& k)
    : origin(o), momentum(k), direction(Math::Vec3(k.X, k.Y, k.Z).Normalized()) {}

    Math::Vec3 ReturnDirection() const { return direction; }

private:
    const Math::Vec3 direction;  // unit spatial direction
};
