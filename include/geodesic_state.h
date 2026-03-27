#pragma once

#include "math_objects.h"

// ------------------------------------- Stores the state of a geodesic trajectory -------------------------------------
struct GeodesicState {
    Math::Vec4 x;
    Math::Vec4 k;

    GeodesicState operator+(const GeodesicState& b) const {
        GeodesicState out;
        for(int i=0;i<4;i++){
            out.x[i] = x[i] + b.x[i];
            out.k[i] = k[i] + b.k[i];
        }
        return out;
    }

    GeodesicState operator*(double s) const {
        GeodesicState out;
        for(int i=0;i<4;i++){
            out.x[i] = s * x[i];
            out.k[i] = s * k[i];
        }
        return out;
    }

    friend GeodesicState operator*(double s, const GeodesicState& a) {
        GeodesicState out;
        for(int i=0;i<4;i++){
            out.x[i] = s * a.x[i];
            out.k[i] = s * a.k[i];
        }
        return out;
    }
};

