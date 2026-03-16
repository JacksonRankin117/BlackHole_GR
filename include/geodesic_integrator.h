#pragma once

#include "camera.h"
#include "math.h"
#include "blackhole.h"
#include "starmap.h"
#include "color.h"


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

// -------------------------------------------- Results of the Ray Tracing ---------------------------------------------
struct TraceResult {
    GeodesicState state;
    bool captured;
};

// ----------------------------------- Computes the derivatives of a geodesic state ------------------------------------
inline GeodesicState GetDerivatives(const GeodesicState& state, const BlackHole::Spacetime& spacetime) {
    GeodesicState deriv;
    deriv.x = state.k;

    Christoffel Gamma = spacetime.ChristoffelSymbols(state.x[1], state.x[2]);

    Math::Vec4 dk;
    for (int mu = 0; mu < 4; ++mu) {
        double sum = 0;
        for (int alpha = 0; alpha < 4; ++alpha) {
            for (int beta = 0; beta < 4; ++beta) {
                sum -= Gamma(mu, alpha, beta) * state.k[alpha] * state.k[beta];
            }
        }
        dk[mu] = sum;
    }
    deriv.k = dk;
    return deriv;
}

// --------------------------------------------- Performs a single RK4 step --------------------------------------------
inline GeodesicState RK4_Step(const GeodesicState& state, double dl, const BlackHole::Spacetime& spacetime)
{
    auto k1 = GetDerivatives(state, spacetime);
    auto k2 = GetDerivatives(state + 0.5*dl*k1, spacetime);
    auto k3 = GetDerivatives(state + 0.5*dl*k2, spacetime);
    auto k4 = GetDerivatives(state + dl*k3, spacetime);

    return state + (dl/6.0)*(k1 + 2*k2 + 2*k3 + k4);
}

// Integrate photon along geodesic until it either escapes to infinity or hits horizon
inline GeodesicState TracePhoton(const GeodesicState& init,             // Initial state
                                 const BlackHole::Spacetime& spacetime, // Spacetime metric
                                 double dl_max = 1.0e7,                 // Maximum step size
                                 int max_steps = 10000)                 // Maximum number of steps
{
    GeodesicState state = init;

    for(int step = 0; step < max_steps; ++step)
    {
        // Stop if photon is far away
        if(state.x[1] > 1000.0 * BlackHole::AU) break;

        // Stop if photon falls below Schwarzschild radius
        if(state.x[1] < spacetime.EventHorizon()) break;

        state = RK4_Step(state, dl_max, spacetime);
    }

    return state;
}

// ------------------------------------------ Adaptive step size integration -------------------------------------------
inline TraceResult TracePhotonAdaptive(const GeodesicState& init,
                                       const BlackHole::Spacetime& spacetime,
                                       double dl_max = 1.0e7,
                                       int max_steps = 20000)
{
    GeodesicState state = init;
    double r_s = spacetime.EventHorizon();

    for(int step = 0; step < max_steps; ++step)
    {
        double r = state.x[1];

        //  Capture test (with safety margin)
        if (r < 1.0001 * r_s)
            return {state, true};

        //  Escape test
        if (r > 1000.0 * BlackHole::AU)
            return {state, false};

        // ---------------- Adaptive step size ----------------

        double dl = dl_max;

        if (r < 20.0 * r_s)   // only adapt near BH
        {
            double safety = 0.2;

            double kr = state.k[1];         // radial 4-momentum
            double dist = r - r_s;

            dl = safety * dist / (std::abs(kr) + 1e-8);

            // Clamp to sane range
            dl = std::max(dl, 0.1);         // minimum step
            dl = std::min(dl, dl_max);      // maximum step
        }

        // ---------------- Integrate ONE step ----------------

        state = RK4_Step(state, dl, spacetime);
    }

    return {state, false};
}

// ----------------------------------------- Generate a Photon from the camera -----------------------------------------
inline GeodesicState PhotonFromCamera(const Camera& cam, int px, int py,
                                     const BlackHole::Spacetime& spacetime)
{
    // Get the Cartesian ray direction from camera
    Ray r_gen = cam.GenerateRay(px, py);
    Math::Vec3 d = r_gen.ReturnDirection(); // This is (dx, dy, dz) in Cartesian

    // Observer position in Spherical
    Math::Vec3 pos = cam.Position();
    double r = pos.Magnitude();
    double theta = std::acos(std::clamp(pos.Z / r, -1.0, 1.0));
    double phi = std::atan2(pos.Y, pos.X);

    // Transform Cartesian velocity (dx, dy, dz) to Spherical velocity (dr, dtheta, dphi)
    // Using the Jacobian transformation:
    double dr = (pos.X * d.X + pos.Y * d.Y + pos.Z * d.Z) / r;
    double dtheta = (pos.Z * dr - r * d.Z) / (r * r * std::sin(theta));
    double dphi = (pos.X * d.Y - pos.Y * d.X) / (pos.X * pos.X + pos.Y * pos.Y);

    // Time component (kt) via the Null Condition
    Matrix g = spacetime.Metric(r, theta);
    double gij_kk = g(1,1)*dr*dr + g(2,2)*dtheta*dtheta + g(3,3)*dphi*dphi;
    double kt = std::sqrt(-gij_kk / g(0,0));

    return GeodesicState{{0.0, r, theta, phi}, {kt, dr, dtheta, dphi}};
}

// ------------------------------------------- Return a Color for the pixel --------------------------------------------
inline Color SamplePhoton(const GeodesicState& final_state, const StarMap& star_map)
{
    // Direction vector in Cartesian at infinity
    double r     = final_state.x[1];
    double theta = final_state.x[2];
    double phi   = final_state.x[3];

    // Convert spherical coordinates to Cartesian direction vector
    Math::Vec3 dir;
    dir.X = std::sin(theta) * std::cos(phi);
    dir.Y = std::sin(theta) * std::sin(phi);
    dir.Z = std::cos(theta);

    return star_map.Sample(dir);
}
