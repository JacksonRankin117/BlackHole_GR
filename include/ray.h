#pragma once

#include "math.h"
// ---------------------------------- Ray ----------------------------------
class Ray {
public:
    Math::Vec3 origin;
    Math::Vec3 direction; // normalized

    Ray(const Math::Vec3& o, const Math::Vec3& d)
        : origin(o), direction(d.Normalized()) {}
};
