#pragma once

#include "math_objects.h"

// ------------------------------------- Stores the state of a geodesic trajectory -------------------------------------
struct GeodesicState {
    Math::Vec4 x{};  // coordinate position  (t, r, θ, φ)
    Math::Vec4 p{};  // frame-basis momentum

    GeodesicState operator+(const GeodesicState& o) const {
        GeodesicState r;
        r.x = x + o.x;
        r.p = p + o.p;
        return r;
    }
    GeodesicState operator*(double s) const {
        GeodesicState r;
        r.x = x * s;
        r.p = p * s;
        return r;
    }
};
inline GeodesicState operator*(double s, const GeodesicState& g) { return g * s; }