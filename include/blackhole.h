#pragma once

#include <cmath>
#include <stdexcept>
#include <limits>

#include "math_objects.h"

using namespace Math;

// Christoffel helper class
struct Christoffel {
    double data[4][4][4]{0.0}; // zero initialize

    // 4x4x4 access
    double& operator()(int mu, int alpha, int beta) {
        return data[mu][alpha][beta];
    }

    const double& operator()(int mu, int alpha, int beta) const {
        return data[mu][alpha][beta];
    }
};

namespace BlackHole {
    // Fundamental constants in SI
    inline constexpr double G = 6.67430e-11;     // Newton's constant
    inline constexpr double c = 299792458.0;     // Speed of light in m/s
    inline constexpr double M_Solar = 1.989e30;  // Mass of the sun in kgs
    inline constexpr double AU = 1.496e11;       // Astronomical unit in meters

    // Shared parameters
    struct Parameters {

        double M;   // Mass (kg)
        double a;   // Kerr spin parameter (meters) : a = J / (M c)
        Vec3 pos;   // Spatial position

        // Primary constructor
        Parameters(double mass, double spin, Vec3 position) : M(mass), a(spin), pos(position) {}
    };

    // Spacetime
    class Spacetime {
        public:
            virtual ~Spacetime() = default;
            virtual Matrix Metric(double r, double theta) const = 0;

            virtual Christoffel ChristoffelSymbols(double r, double theta) const = 0;

            virtual double EventHorizon() const = 0;
    };
    // --------------------------------------------------- Minkowski ---------------------------------------------------
    class Minkowski : public Spacetime {  // Flat spacetime
    public:

        Parameters params;

        // Position irrelevant, mass = 0
        Minkowski(Vec3 pos = {0,0,0})
            : params{0.0, 0.0, pos} {}

        // No horizon
        double EventHorizon() const override { return 0.0; }

        // Metric tensor η_{μν}
        Matrix Metric(double, double) const override
        {
            Matrix g{4,4};

            g(0,0) = -c * c;
            g(1,1) = 1.0;
            g(2,2) = 1.0;
            g(3,3) = 1.0;

            return g;
        }

        // All Christoffels vanish
        Christoffel ChristoffelSymbols(double, double) const override
        {
            return Christoffel{}; // Initialize as a zero-tensor
        }
    };

    // ------------------------------------------------- Schwarzschild -------------------------------------------------

    class Schwarzschild : public Spacetime{
    public:

        Parameters params;

        // Constructor
        Schwarzschild(double M, Vec3 pos) : params{M, 0.0, pos}
        {
            r_S = 2.0 * G * M / (c * c);  // Schwarzschild radius
            r_ISCO = 3.0 * r_S;           // Innermost stable circular orbit
        }

        // Return the event horizon and the Innermost Stable Circular Orbit
        double EventHorizon() const override { return r_S; }
        double ISCO() const { return r_ISCO; }

        // Metrix Tensor g_{\mu \nu}
        Matrix Metric(double r, double theta) const override
        {
            Matrix g{4,4};

            double f = 1.0 - r_S / r;

            // Set the metric components
            g(0,0) = -f * c * c;
            g(1,1) = 1.0 / f;
            g(2,2) = r * r;
            g(3,3) = r * r * sin(theta) * sin(theta);

            return g;
        }

        // Christoffel Symbols
        Christoffel ChristoffelSymbols(double r, double theta) const override {
            Christoffel Gamma;

            double f = 1.0 - r_S / r;
            double r2 = r*r;
            double sinT = sin(theta);
            double cosT = cos(theta);

            // Gamma^r_tt
            Gamma(1,0,0) = r_S * c * c * f / (2*r2);

            // Gamma^r_rr
            Gamma(1,1,1) = - r_S / (2 * r2 * f);

            // Gamma^r_(theta theta)
            Gamma(1,2,2) = - r * f;

            // Gamma^r_(phi phi)
            Gamma(1,3,3) = - r * f * sinT*sinT;

            // Gamma^theta_(r theta) = Gamma^theta_(theta r)
            Gamma(2,1,2) = 1.0 / r;
            Gamma(2,2,1) = 1.0 / r;

            // Gamma^theta_(phi phi)
            Gamma(2,3,3) = - sinT * cosT;

            // Gamma^phi_(r phi) = Gamma^phi_(phi r)
            Gamma(3,1,3) = 1.0 / r;
            Gamma(3,3,1) = 1.0 / r;

            // Gamma^phi_(theta phi) = Gamma^phi_(phi theta)
            Gamma(3,2,3) = cosT / sinT;
            Gamma(3,3,2) = cosT / sinT;

            return Gamma;
        }

    private:
        double r_S;      // Schwarzschild radius
        double r_ISCO;   // Innermost stable circular orbit
    };

    // ----------------------------------------------------- Kerr ------------------------------------------------------

    class Kerr : public Spacetime {

    public:

        Parameters params;

        // Primary constructor
        Kerr(double M, double a, Vec3 pos) : params{M, a, pos}
        {
            // Gravitational radius
            double rg = G * M / (c * c);
            K_rg = rg;

            // Prevents a naked singularity
            if (std::abs(a) > K_rg) throw std::invalid_argument("Spin exceeds extremal Kerr limit");

            // Calculates and stores the inner and outer singularity
            KerrInner = K_rg - sqrt(rg * rg - a * a);
            KerrOuter = K_rg + sqrt(rg * rg - a * a);

            r_ISCO = std::numeric_limits<double>::quiet_NaN();
        }

        // Returns the inner and outer horizons
        double HorizonInner() const { return KerrInner; }
        double HorizonOuter() const { return KerrOuter; }

        // Calculates and stores the Innermost Stable Circular Orbit
        void setISCO(bool retrograde)
        {
            //
            double a_star = params.a / K_rg;

            // Z1 and Z2 according to Boyer-Lundquist
            double Z1 = 1 + cbrt(1 - a_star * a_star) * (cbrt(1 + a_star) + cbrt(1 - a_star));
            double Z2 = sqrt(3 * a_star * a_star + Z1 * Z1);

            // If the orbit of the event horizon is retrograde, return +1.0. For an anterograde orbit, return -1.0
            double sign = retrograde ? +1.0 : -1.0;

            // Stores the ISCO
            r_ISCO = K_rg * (3 + Z2 + sign * sqrt((3 - Z1) * (3 + Z1 + 2 * Z2)));
        }

        // Returns the ISCO for a Kerr black hole
        double ISCO() const { return r_ISCO; }

        double EventHorizon() const override { return KerrOuter; }

        // Metric Tensor g_{\mu \nu}
        Matrix Metric(double r, double theta) const override
        {
            Matrix g{4,4};  // Initializes a 4x4 matrix. All components are 0.0 to start out with

            // Store the spin parameter
            double a = params.a;

            // Store the sine and cosine of theta
            double sinT = sin(theta);
            double cosT = cos(theta);

            double sin2 = sinT * sinT;

            // Sigma and Delta are constants that show up everywhere
            double Sigma = r*r + a*a * cosT*cosT;
            double Delta = r*r - 2.0*K_rg*r + a*a;

            // Populates the components of the 4x4 metric tensor according to Kerr
            g(0,0) = -(1.0 - (2.0*K_rg*r)/Sigma) * c*c;

            g(1,1) = Sigma / Delta;

            g(2,2) = Sigma;

            g(0,3) = -(2.0 * K_rg * a * r * sin2 / Sigma) * c;
            g(3,0) = g(0,3);

            g(3,3) = (r*r + a*a + (2.0 * K_rg * a*a * r * sin2) / Sigma) * sin2;

            // Return the Kerr metric
            return g;
        }

    private:
        double KerrInner;  // Radius of the inner horizon
        double KerrOuter;  // Radius of the outer horizon
        double r_ISCO;     // Radius of the ISCO
        double K_rg;       // Gravitational radius of a Kerr black hole
    };
}
