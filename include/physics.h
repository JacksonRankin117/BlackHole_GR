#pragma once

#include "math.h"

namespace gr
{
    inline double Contract(const Math::Vec4& a,
                           const Math::Vec4& b,
                           const Math::Matrix& g)
    {
        Math::Vec4 gb = g * b;  // metric lowers index
        return a.T * gb.T +
               a.X * gb.X +
               a.Y * gb.Y +
               a.Z * gb.Z;
    }
}