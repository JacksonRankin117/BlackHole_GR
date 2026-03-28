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
    GeodesicState state;                      // State of of the photon
    bool captured;                            // Only true if the black hole captures the photon
    std::shared_ptr<Material> mat = nullptr;  // Material of the hit obnject
    HitRecord hit;                            // HitRecord object for a more detailed intersection
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

    // Affine parameter along the geodesic
    double lambda = 0.0;
    double dl     = dl_max;

    constexpr double eps = 1e-6;

    for (int step = 0; step < max_steps; ++step)
    {
        double r = state.x[1];

        // -------------------- Horizon crossing --------------------
        // If the photon has fallen to (or inside) the event horizon, it's captured.
        // The 1.01 factor gives a small numerical safety margin.
        if (r <= 1.01 * r_s)
            return TraceResult{state, true, nullptr, HitRecord{}};

        // -------------------- Escape cutoff --------------------
        // If the photon is far enough away, it has escaped. Stop tracing.
        if (r > 1000.0 * r_s)
            return TraceResult{state, false, nullptr, HitRecord{}};

        // -------------------- RK4 integration --------------------
        // Advance the geodesic by one affine step using a 4th-order Runge-Kutta integrator.
        GeodesicState k1 = GetDerivatives(state, spacetime);
        GeodesicState k2 = GetDerivatives(state + k1 * (0.5 * dl), spacetime);
        GeodesicState k3 = GetDerivatives(state + k2 * (0.5 * dl), spacetime);
        GeodesicState k4 = GetDerivatives(state + k3 * dl, spacetime);

        GeodesicState next = state + (dl / 6.0) * (k1 + 2.0*k2 + 2.0*k3 + k4);

        // -------------------- Coordinate conversion: spherical → Cartesian --------------------
        //
        // The photon state is tracked in Boyer-Lindquist / Schwarzschild spherical coordinates:
        //   state.x = { T, r, θ, φ }
        //   state.k = { kᵗ, kʳ, kᶿ, kᵠ }
        //
        // But scene objects (spheres, etc.) are placed in Cartesian coordinates (X, Y, Z).
        // To do a geometrically correct ray-object intersection, we must express both the
        // photon's position and its direction in the same Cartesian space.
        //
        // Step 1: Extract spherical coordinates and precompute trig values.
        double r_sph  = state.x[1];
        double theta  = state.x[2];
        double phi    = state.x[3];

        double sinT = std::sin(theta);
        double cosT = std::cos(theta);
        double sinP = std::sin(phi);
        double cosP = std::cos(phi);

        // Step 2: Convert photon position to Cartesian.
        //   X = r sin(θ) cos(φ)
        //   Y = r sin(θ) sin(φ)
        //   Z = r cos(θ)
        Math::Vec4 cartPos{
            0.0,
            r_sph * sinT * cosP,   // X
            r_sph * sinT * sinP,   // Y
            r_sph * cosT           // Z
        };

        // Step 3: Convert the spatial 4-velocity components to Cartesian.
        //
        // This applies the Jacobian of the spherical-to-Cartesian map to the
        // spherical velocity vector (kʳ, kᶿ, kᵠ):
        //
        //   kˣ = sin(θ)cos(φ)·kʳ  +  r·cos(θ)cos(φ)·kᶿ  -  r·sin(θ)sin(φ)·kᵠ
        //   kʸ = sin(θ)sin(φ)·kʳ  +  r·cos(θ)sin(φ)·kᶿ  +  r·sin(θ)cos(φ)·kᵠ
        //   kᶻ = cos(θ)·kʳ        -  r·sin(θ)·kᶿ
        //
        // (This is the same Jacobian used in PhotonFromCamera, but applied in reverse.)
        double kr     = state.k[1];
        double ktheta = state.k[2];
        double kphi   = state.k[3];

        Math::Vec4 cartDir{
            0.0,
            sinT * cosP * kr  +  r_sph * cosT * cosP * ktheta  -  r_sph * sinT * sinP * kphi,  // kˣ
            sinT * sinP * kr  +  r_sph * cosT * sinP * ktheta  +  r_sph * sinT * cosP * kphi,  // kʸ
            cosT        * kr  -  r_sph * sinT         * ktheta                                  // kᶻ
        };

        // -------------------- Object intersection test --------------------
        // Build a Cartesian ray and test it against all scene objects.
        // Both origin and direction are now in Cartesian space, matching the
        // coordinate system used by the sphere centers.
        Ray ray(cartPos, cartDir);

        HitRecord rec;
        if (hittables.Intersect(ray, 0.0, dl, rec))
        {
            // The photon has hit an object — return its material and hit record.
            return TraceResult{state, false, rec.mat, rec};
        }

        // -------------------- Commit the RK4 step --------------------
        double r_next = next.x[1];

        state   = next;
        lambda += dl;

        // -------------------- Adaptive step size --------------------
        // Near the black hole, spacetime curvature is large and we need smaller steps
        // to keep the integration accurate. Far away, we can take larger steps.
        //
        // The step size is scaled by how close the photon is to the horizon,
        // divided by the radial velocity component to avoid over-shooting.
        double r_mid = 0.5 * (r + r_next);

        double curvature_scale = std::max(r_mid - r_s, eps);
        double kappa           = std::abs(state.k[1]) + eps;

        double safety = 0.25;

        if (r_mid < 25.0 * r_s)
        {
            // Close to the black hole: shrink the step proportionally to proximity.
            dl = safety * curvature_scale / kappa;
            dl = std::clamp(dl, 1e-4, dl_max);
        }
        else
        {
            // Far from the black hole: use the maximum step size.
            dl = dl_max;
        }
    }

    // Exceeded max steps without hitting anything or escaping — return the final state.
    return TraceResult{state, false, nullptr, HitRecord{}};
}
// ----------------------------------------- Generate a Photon from the camera -----------------------------------------
inline GeodesicState PhotonFromCamera(const Camera& cam, int px, int py,
                                     const BlackHole::Spacetime& spacetime)
{
    // Get the Cartesian ray direction from camera
    Ray r_gen = cam.GenerateRay(px, py);
    Math::Vec3 d = r_gen.Direction(); // This is (dx, dy, dz) in Cartesian

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
