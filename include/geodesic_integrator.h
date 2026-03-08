#pragma once

#include "ray.h"
#include "blackhole.h"

class GeodesicIntegrator {
public:

    // Integrate a photon ray through spacetime using simple Euler method
    // lambda_step: step size in affine parameter
    // steps: number of steps to march
    static void EulerStep(Ray& ray, const BlackHole::Spacetime& spacetime,
                          double lambda_step, int steps)
    {
        // Determine horizon dynamically
        double horizon = 0.0;
        if (auto s = dynamic_cast<const BlackHole::Schwarzschild*>(&spacetime))
            horizon = s->EventHorizon();
        else if (auto k = dynamic_cast<const BlackHole::Kerr*>(&spacetime))
            horizon = k->HorizonOuter();

        for (int i = 0; i < steps; ++i) {
            auto& x = ray.r_pos;       // Vec4: (t, r, θ, φ)
            auto& p = ray.r_momentum;  // Vec4: (p^t, p^r, p^θ, p^φ)

            // Ensure r > 0
            if (x.Y <= 0.0) break;

            // Christoffel symbols
            auto Gamma = spacetime.ChristoffelSymbols(x.Y, x.Z); // r, θ

            Vec4 dp;
            for (int mu = 0; mu < 4; ++mu) {
                double sum = 0.0;
                for (int alpha = 0; alpha < 4; ++alpha)
                    for (int beta = 0; beta < 4; ++beta)
                        sum += -Gamma(mu, alpha, beta) * p[alpha] * p[beta];
                dp[mu] = sum;
            }

            // Euler step
            for (int mu = 0; mu < 4; ++mu) {
                x[mu] += lambda_step * p[mu];
                p[mu] += lambda_step * dp[mu];
            }

            // Stop if inside horizon or too far
            if (x.Y <= horizon) break;
            if (x.Y > 1e12) break;
        }
    }
};
