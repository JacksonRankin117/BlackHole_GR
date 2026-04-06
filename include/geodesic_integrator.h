// include/geodesic_integrator.h

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
    GeodesicState state;                                          // Final state of the photon
    bool captured;                                                // Only true if the black hole captures the photon
    std::shared_ptr<Material> mat = nullptr;                      // Material of the hit object
    HitRecord hit;                                                // HitRecord object for a more detailed intersection
    Ray ray = Ray(Math::Vec4{0,0,0,0}, Math::Vec4{0,0,0,0});     // Records the ray at hit time
};

// ----------------------------------- Computes the derivatives of a geodesic state ------------------------------------
inline GeodesicState GetDerivatives(const GeodesicState& state,
                                    const BlackHole::Spacetime& spacetime)
{
    GeodesicState deriv;

    double sinT = std::sin(state.x[2]);
    double cosT = std::cos(state.x[2]);

    // Extract b = sinT * kphi from p[3]
    double b    = state.p[3];
    double sinT_safe = std::copysign(std::max(std::abs(sinT), 1e-4), sinT);
    double kphi = b / sinT_safe;

    // Position derivatives: dx^μ/dλ = k^μ
    // p[3] stores b, but dφ/dλ = kphi, so override x[3] derivative
    deriv.x    = state.p;
    deriv.x[3] = kphi;

    // Build momentum vector with real kphi for Christoffel contraction
    Math::Vec4 k = state.p;
    k[3] = kphi;

    Christoffel Gamma = spacetime.ChristoffelSymbols(state.x[1], state.x[2], sinT, cosT);

    Math::Vec4 dk;
    for (int mu = 0; mu < 4; ++mu) {
        double sum = 0.0;
        for (int alpha = 0; alpha < 4; ++alpha)
            for (int beta = 0; beta < 4; ++beta)
                sum -= Gamma(mu, alpha, beta) * k[alpha] * k[beta];
        dk[mu] = sum;
    }

    // Convert d(kphi)/dλ → d(b)/dλ via chain rule:
    // d(sinT * kphi)/dλ = cosT * (dθ/dλ) * kphi + sinT * d(kphi)/dλ
    // Note: dθ/dλ = k^θ = state.p[2] (this is the real ktheta, not b-transformed)
    dk[3] = cosT * state.p[2] * kphi + sinT * dk[3];

    deriv.p = dk;
    return deriv;
}

// --------------------------------------------- Performs a single RK4 step -------------------------------------------
inline GeodesicState RK4_Step(const GeodesicState& state, double dl, const BlackHole::Spacetime& spacetime)
{
    auto k1 = GetDerivatives(state, spacetime);
    auto k2 = GetDerivatives(state + 0.5*dl*k1, spacetime);
    auto k3 = GetDerivatives(state + 0.5*dl*k2, spacetime);
    auto k4 = GetDerivatives(state + dl*k3, spacetime);

    return state + (dl/6.0)*(k1 + 2.0*k2 + 2.0*k3 + k4);
}

// ------------------------------------------ Adaptive step size integration -------------------------------------------
inline TraceResult TracePhotonAdaptive(
    const GeodesicState& init,
    const BlackHole::Spacetime& spacetime,
    const HittableList& hittables,
    double dl_max = 1.5e8,
    int max_steps = 100'000)
{
    GeodesicState state = init;

    double r_s = spacetime.EventHorizon();

    double lambda = 0.0;
    double dl     = dl_max;

    constexpr double eps             = 1e-6;
    constexpr double DL_OBJECT_CLAMP = 1.5e8; // metres — must be <= 1/10 smallest object radius

    // ---- Converts spherical state position to Cartesian Vec4 ----
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

    // ---- Reflects θ back into [0, π] and wraps φ into [0, 2π] ----
    // When θ crosses a pole, both k[2] (dθ/dλ) and k[3] (dφ/dλ) must flip
    // to keep the momentum vector continuous across the singularity.
    // Missing the k[3] flip was the original cause of the vertical seam artifact.
    auto NormalizeAngles = [](GeodesicState& s)
    {
        if (s.x[2] < 0.0) {
            s.x[2] = -s.x[2];
            s.x[3] += M_PI;
            s.p[2]  = -s.p[2];
            // b = sinT * kphi: sinT is unchanged in magnitude at pole reflection,
            // kphi flips sign, so b flips sign too
            s.p[3]  = -s.p[3];
        }
        if (s.x[2] > M_PI) {
            s.x[2] = 2.0 * M_PI - s.x[2];
            s.x[3] += M_PI;
            s.p[2]  = -s.p[2];
            s.p[3]  = -s.p[3];
        }

        s.x[3] = std::fmod(s.x[3], 2.0 * M_PI);
        if (s.x[3] < 0.0) s.x[3] += 2.0 * M_PI;
    };

    for (int step = 0; step < max_steps; ++step)
    {
        double r = state.x[1];

        // -------------------- Horizon crossing --------------------
        // Require inward radial momentum to avoid false captures on grazing trajectories
        if (r <= 1.01 * r_s && state.p[1] < 0.0)
            return TraceResult{state, true, nullptr, HitRecord{}};

        // -------------------- Escape cutoff --------------------
        if (r > 1000.0 * r_s)
            return TraceResult{state, false, nullptr, HitRecord{}};

        // -------------------- RK4 integration --------------------
        GeodesicState next = RK4_Step(state, dl, spacetime);
        NormalizeAngles(next);

        // -------------------- Coordinate conversion --------------------
        Math::Vec4 cartPosA = ToCartPos(state);
        Math::Vec4 cartPosB = ToCartPos(next);

        Math::Vec4 cartOrigin = (cartPosA + cartPosB) * 0.5;
        Math::Vec4 chordVec   = cartPosB - cartPosA;
        double     chordLen   = chordVec.Magnitude();

        // -------------------- Object intersection test --------------------
        if (chordLen > eps)
        {
            Ray ray(cartOrigin, chordVec * (1.0 / chordLen));

            HitRecord rec;
            if (hittables.Intersect(ray, 0.0, chordLen, rec))
                return TraceResult{state, false, rec.mat, rec, ray};
        }

        // -------------------- Commit the RK4 step --------------------
        double r_next = next.x[1];
        state   = next;
        lambda += dl;

        // -------------------- Adaptive step size --------------------
        double r_mid           = 0.5 * (r + r_next);
        double curvature_scale = std::max(r_mid - r_s, eps);
        double kappa           = std::abs(state.p[1]) + eps;

        if (r_mid < 25.0 * r_s)
        {
            dl = 0.1 * curvature_scale / kappa;
            dl = std::clamp(dl, 1e-4, dl_max);
        }
        else
        {
            dl = dl_max;
        }

        // Never step further than 1/10 the smallest hittable object radius,
        // or chords will skip over it entirely.
        // dl = std::min(dl, DL_OBJECT_CLAMP);
    }

    return TraceResult{state, false, nullptr, HitRecord{}};
}

// ----------------------------------------- Generate a Photon from the camera ----------------------------------------
inline GeodesicState PhotonFromCamera(const Camera& cam, double px, double py,
                                      const BlackHole::Spacetime& spacetime)
{
    Ray r_gen = cam.GenerateRay(px, py);
    Math::Vec3 d = r_gen.Direction();
    Math::Vec3 pos = cam.Position();

    double r     = pos.Magnitude();
    double theta = std::acos(std::clamp(pos.Z / r, -1.0, 1.0));
    double phi   = std::atan2(pos.Y, pos.X);

    double sinT = std::sin(theta);
    double cosT = std::cos(theta);
    double sinP = std::sin(phi);
    double cosP = std::cos(phi);

    double dr     =  sinT*cosP * d.X  +  sinT*sinP * d.Y  +  cosT * d.Z;
    double dtheta = (cosT*cosP * d.X  +  cosT*sinP * d.Y  -  sinT * d.Z) / r;

    // Store b = sinT * dphi instead of dphi.
    // (-sinP * d.X + cosP * d.Y) / r   is already sinT*dphi via the Jacobian.
    double b = (-sinP * d.X + cosP * d.Y) / r;

    // Null condition: g_μν k^μ k^ν = 0
    // g_33 * kphi^2 = g_33 * (b/sinT)^2 = (r^2 sin^2T) * b^2/sin^2T = r^2 * b^2
    // So replace g(3,3)*dphi*dphi with r^2 * b^2
    Matrix g      = spacetime.Metric(r, theta);
    double gij_kk = g(1,1)*dr*dr + g(2,2)*dtheta*dtheta + r*r*b*b;
    double kt     = std::sqrt(std::max(-gij_kk / g(0,0), 0.0));

    // p[3] now stores b = sinT * kphi, not kphi
    return GeodesicState{{0.0, r, theta, phi}, {kt, dr, dtheta, b}};
}
// ------------------------------------------- Return a Color for the pixel --------------------------------------------
inline Color SamplePhoton(const GeodesicState& final_state, const StarMap& star_map)
{
    double kr     = final_state.p[1];
    double ktheta = final_state.p[2];
    double b      = final_state.p[3];  // b = sinT * kphi

    double r     = final_state.x[1];
    double theta = final_state.x[2];
    double phi   = final_state.x[3];

    double sinT = std::sin(theta);
    double cosT = std::cos(theta);
    double sinP = std::sin(phi);
    double cosP = std::cos(phi);

    double vr     = kr;
    double vtheta = r * ktheta;
    double vphi   = r * b;      // r * sinT * kphi = r * b  — no sinT division!

    double dx = sinT*cosP * vr  +  cosT*cosP * vtheta  -  sinP * vphi;
    double dy = sinT*sinP * vr  +  cosT*sinP * vtheta  +  cosP * vphi;
    double dz = cosT      * vr  -  sinT      * vtheta;

    double len = std::sqrt(dx*dx + dy*dy + dz*dz);
    if (len < 1e-30) return Color{0.0f, 0.0f, 0.0f};

    Math::Vec3 dir{ dx/len, dy/len, dz/len };
    return star_map.Sample(dir);
}
