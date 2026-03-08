#pragma once

#include "ray.h"
#include "blackhole.h"

namespace Physics {

// Single Euler step along the geodesic
inline void EulerStep(Ray& ray,
                      const BlackHole::Spacetime& spacetime,
                      double dlambda)
{
    // Convert current position to spherical coordinates (r, theta, phi)
    double r     = ray.r_pos.Magnitude();           // radial distance
    double theta = std::acos(ray.r_pos.Z / r);     // polar angle
    double phi   = std::atan2(ray.r_pos.Y, ray.r_pos.X); // azimuthal angle

    // Get Christoffel symbols at current location
    auto Gamma = spacetime.ChristoffelSymbols(r, theta);

    // Compute derivative of momentum using geodesic equation:
    // dp^mu/dlambda = - Γ^mu_{αβ} p^α p^β
    Math::Vec4 dp{0.0, 0.0, 0.0, 0.0};
    for (int mu = 0; mu < 4; mu++) {
        double sum = 0.0;
        for (int alpha = 0; alpha < 4; alpha++) {
            for (int beta = 0; beta < 4; beta++) {
                sum += -Gamma(mu, alpha, beta) * ray.r_momentum[alpha] * ray.r_momentum[beta];
            }
        }
        dp[mu] = sum * dlambda;
    }

    // Update momentum
    ray.r_momentum += dp;

    // Update position
    ray.r_pos += ray.r_momentum * dlambda;
}

} // namespace Physics
