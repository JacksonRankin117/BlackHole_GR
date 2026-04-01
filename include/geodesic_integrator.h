#pragma once

#include "blackhole.h"
#include "camera.h"
#include "color.h"
#include "geodesic_state.h"
#include "hittable_list.h"
#include "math_objects.h"
#include "starmap.h"

#include <algorithm>
#include <cmath>

// -------------------------------------------- Results of the Ray Tracing ---------------------------------------------
struct TraceResult {
    GeodesicState state;                                      // State of of the photon
    bool captured;                                            // Only true if the black hole captures the photon
    std::shared_ptr<Material> mat = nullptr;                  // Material of the hit obnject
    HitRecord hit;                                            // HitRecord object for a more detailed intersection
    Ray ray = Ray(Math::Vec4{0,0,0,0}, Math::Vec4{0,0,0,0});  // Records the ray
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
inline TraceResult TracePhotonAdaptive(
    const GeodesicState& init,
    const BlackHole::Spacetime& spacetime,
    const HittableList& hittables,
    double dl_max = 1.0e7,
    int max_steps = 20000)
{
    GeodesicState state = init;

    double r_s = spacetime.EventHorizon();

    double lambda = 0.0;
    double dl     = dl_max;

    constexpr double eps = 1e-6;

    // Helper: convert a geodesic state's spherical position to a Cartesian Vec4
    auto ToCartPos = [](const GeodesicState& s) -> Math::Vec4
    {
        double r     = s.x[1];
        double theta = s.x[2];
        double phi   = s.x[3];
        double sinT  = std::sin(theta);
        double cosT  = std::cos(theta);
        double sinP  = std::sin(phi);
        double cosP  = std::cos(phi);
        return Math::Vec4{
            0.0,
            r * sinT * cosP,
            r * sinT * sinP,
            r * cosT
        };
    };

    for (int step = 0; step < max_steps; ++step)
    {
        double r = state.x[1];

        // -------------------- Horizon crossing --------------------
        if (r <= 1.01 * r_s)
            return TraceResult{state, true, nullptr, HitRecord{}};

        // -------------------- Escape cutoff --------------------
        if (r > 1000.0 * r_s)
            return TraceResult{state, false, nullptr, HitRecord{}};

        // -------------------- RK4 integration --------------------
        GeodesicState k1 = GetDerivatives(state, spacetime);
        GeodesicState k2 = GetDerivatives(state + k1 * (0.5 * dl), spacetime);
        GeodesicState k3 = GetDerivatives(state + k2 * (0.5 * dl), spacetime);
        GeodesicState k4 = GetDerivatives(state + k3 * dl, spacetime);

        GeodesicState next = state + (dl / 6.0) * (k1 + 2.0*k2 + 2.0*k3 + k4);

        // -------------------- Coordinate conversion --------------------
        // Convert both endpoints of the geodesic segment to Cartesian, then
        // build the ray as the chord between them. This is more accurate than
        // shooting tangentially from state alone, since the chord actually spans
        // the segment that was integrated.

        Math::Vec4 cartPosA = ToCartPos(state);
        Math::Vec4 cartPosB = ToCartPos(next);

        // Ray origin: midpoint of the chord — reduces one-sided sampling bias.
        Math::Vec4 cartOrigin = (cartPosA + cartPosB) * 0.5;

        // Ray direction: chord vector from current to next position.
        // Length of the chord is the interval we hand to Intersect.
        Math::Vec4 chordVec  = cartPosB - cartPosA;
        double     chordLen  = chordVec.Magnitude(); // spatial length only

        // -------------------- Object intersection test --------------------
        // Test the chord as a ray with interval [0, chordLen].
        // Using chordLen (not dl) as the far limit ensures we only accept hits
        // that actually lie within this integration step, regardless of how dl
        // was last adapted.
        if (chordLen > eps)
        {
            Ray ray(cartOrigin, chordVec * (1.0 / chordLen));

            HitRecord rec;
            if (hittables.Intersect(ray, 0.0, chordLen, rec))
                return TraceResult{state, false, rec.mat, rec, ray};
        }

        // -------------------- Commit the RK4 step --------------------
        double r_next = next.x[1];
        double dl_used = dl;

        state   = next;
        lambda += dl_used;

        // -------------------- Adaptive step size --------------------
        // Update dl AFTER the intersection test so the interval used above
        // always matches the step that was actually taken.
        double r_mid = 0.5 * (r + r_next);

        double curvature_scale = std::max(r_mid - r_s, eps);
        double kappa           = std::abs(state.k[1]) + eps;

        double safety = 0.1; // tightened from 0.25 to close remaining chord gaps

        if (r_mid < 25.0 * r_s)
        {
            dl = safety * curvature_scale / kappa;
            dl = std::clamp(dl, 1e-4, dl_max);
        }
        else
        {
            dl = dl_max;
        }
    }

    return TraceResult{state, false, nullptr, HitRecord{}};
}
// ----------------------------------------- Generate a Photon from the camera -----------------------------------------
inline GeodesicState PhotonFromCamera(const Camera& cam, double px, double py,
                                      const BlackHole::Spacetime& spacetime)
{
    // Get the Cartesian ray direction from the camera
    Ray r_gen = cam.GenerateRay(px, py);
    Math::Vec3 d = r_gen.Direction(); // (dx, dy, dz) in Cartesian
 
    // Observer position in spherical coordinates
    Math::Vec3 pos = cam.Position();
    double r     = pos.Magnitude();
    double theta = std::acos(std::clamp(pos.Z / r, -1.0, 1.0));
    double phi   = std::atan2(pos.Y, pos.X);
 
    // Transform Cartesian velocity (dx, dy, dz) → spherical velocity (dr, dθ, dφ)
    // via the Jacobian of the spherical-to-Cartesian map
    double dr     = (pos.X * d.X + pos.Y * d.Y + pos.Z * d.Z) / r;
    double dtheta = (pos.Z * dr - r * d.Z) / (r * r * std::sin(theta));
    double dphi   = (pos.X * d.Y - pos.Y * d.X) / (pos.X * pos.X + pos.Y * pos.Y);
 
    // Time component (kᵗ) from the null condition g_{μν} kᵘ kᵛ = 0
    Matrix g      = spacetime.Metric(r, theta);
    double gij_kk = g(1,1)*dr*dr + g(2,2)*dtheta*dtheta + g(3,3)*dphi*dphi;
    double kt     = std::sqrt(-gij_kk / g(0,0));
 
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
