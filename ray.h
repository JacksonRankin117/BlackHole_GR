#pragma once

#include "color.h"
#include "math.h"
#include "physics.h"

class Ray {
    public:
        // Default constructor
        constexpr Ray(const Math::Vec4& origin, const Math::Vec4& direction) noexcept
                       : r_origin(origin), r_direct(direction) {}

        //
        [[nodiscard]] constexpr Math::Vec4 at(double lambda) const noexcept {
            return r_origin + lambda * r_direct;
        }

        Math::Vec4 r_origin;
        Math::Vec4 r_direct;
        Color r_color;

};
