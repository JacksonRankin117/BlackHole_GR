// AccretionDisk.h

#pragma once

#include <cmath>
#include <memory>

#include "blackhole.h"
#include "color.h"
#include "hittable.h"
#include "material.h"

// Disk lies in the XY plane (Z = 0 is the equatorial plane).
// Z is the symmetry axis. All radius calculations use sqrt(X² + Y²).

class AccretionDisk : public Hittable {
public:
    // ------------------------------------------------------------------ //
    //  Constructor                                                        //
    //                                                                     //
    //  spacetime    — Schwarzschild or Kerr instance (borrowed ref)       //
    //  center       — disk center in world space                          //
    //  mdot         — mass accretion rate [kg/s]                          //
    //  r_outer      — outer disk edge [m]                                 //
    //  f_col        — spectral hardening factor (default 1.7)             //
    //  half_height  — slab half-thickness [m], 0 = infinitely thin        //
    // ------------------------------------------------------------------ //
    AccretionDisk(const BlackHole::Spacetime& spacetime,
                  const Math::Vec4&           center,
                  double mdot,
                  double r_outer,
                  double f_col       = 1.7,
                  double half_height = 0.0) noexcept
        : d_spacetime(spacetime),
          d_center(center),
          d_mdot(mdot),
          d_r_out(r_outer),
          d_fcol(f_col),
          d_half_h(half_height)
    {
        if (auto* s = dynamic_cast<const BlackHole::Schwarzschild*>(&spacetime))
            d_r_in = s->ISCO();
        else if (auto* k = dynamic_cast<const BlackHole::Kerr*>(&spacetime))
            d_r_in = k->ISCO();
        else
            d_r_in = spacetime.EventHorizon() * 3.0;

        d_rg = BlackHole::G * spacetime_mass() / (BlackHole::c * BlackHole::c);

        d_r_peak    = d_r_in * std::pow(49.0 / 36.0, 2.0 / 3.0);
        d_flux_peak = PageThorneFlux(d_r_peak);
    }

    // ------------------------------------------------------------------ //
    //  Intersection                                                       //
    // ------------------------------------------------------------------ //
    [[nodiscard]] bool Intersect(const Ray& ray,
                                 double lambda_min, double lambda_max,
                                 HitRecord& rec) const noexcept override
    {
        const Math::Vec3 orig   = Spatial(ray.origin);
        const Math::Vec3 center = Spatial(d_center);
        const Math::Vec3 dir    = ray.Direction().Normalized();
        const Math::Vec3 ro     = orig - center;

        double     best_t = -1.0;
        Math::Vec3 best_n{0, 0, 1};

        if (d_half_h <= 0.0)
        {
            if (std::abs(dir.Z) < 1e-12) return false;
            double t = -ro.Z / dir.Z;
            if (t < lambda_min || t > lambda_max) return false;
            Math::Vec3 hit = ro + t * dir;
            double r = std::sqrt(hit.X*hit.X + hit.Y*hit.Y);
            if (r < d_r_in || r > d_r_out) return false;
            best_t = t;
            best_n = (dir.Z < 0.0) ? Math::Vec3{0, 0, 1} : Math::Vec3{0, 0, -1};
        }
        else
        {
            if (std::abs(dir.Z) > 1e-8)
            {
                TryCap(ro, dir, lambda_min, lambda_max,
                       -d_half_h, {0, 0, -1}, best_t, best_n);
                TryCap(ro, dir, lambda_min, lambda_max,
                        d_half_h, {0, 0,  1}, best_t, best_n);
            }
            TryCylinder(ro, dir, lambda_min, lambda_max,
                        d_r_in,  true,  best_t, best_n);
            TryCylinder(ro, dir, lambda_min, lambda_max,
                        d_r_out, false, best_t, best_n);
        }

        if (best_t < 0.0) return false;

        Math::Vec3 hit = ro + best_t * dir;
        double r   = std::sqrt(hit.X*hit.X + hit.Y*hit.Y);
        double phi = std::atan2(hit.Y, hit.X);

        FillRecord(rec, ray, best_t, r, phi, best_n);
        return true;
    }

private:
    // ------------------------------------------------------------------ //
    //  Page-Thorne radiative flux (Page & Thorne 1974, ApJ 191 499)      //
    // ------------------------------------------------------------------ //
    [[nodiscard]] double PageThorneFlux(double r) const noexcept
    {
        if (r <= d_r_in) return 0.0;

        double f = 0.0;

        if (auto* k = dynamic_cast<const BlackHole::Kerr*>(&d_spacetime))
        {
            double a_star = k->params.a / d_rg;
            double x      = std::sqrt(r / d_rg);
            double x_in   = std::sqrt(d_r_in / d_rg);

            double x1, x2, x3;
            SolveKerrRoots(a_star, x1, x2, x3);

            auto Q = [](double x, double xi) {
                return std::log(std::abs(x - xi));
            };

            f = x - x_in
              - 1.5 * a_star * std::log(x / x_in)
              - (3.0 * (x1 - a_star) * (x1 - a_star)) / (x1 * (x1 - x2) * (x1 - x3))
                  * (Q(x, x1) - Q(x_in, x1))
              - (3.0 * (x2 - a_star) * (x2 - a_star)) / (x2 * (x2 - x1) * (x2 - x3))
                  * (Q(x, x2) - Q(x_in, x2))
              - (3.0 * (x3 - a_star) * (x3 - a_star)) / (x3 * (x3 - x1) * (x3 - x2))
                  * (Q(x, x3) - Q(x_in, x3));

            f = std::max(f, 0.0);
        }
        else
        {
            // Schwarzschild
            f = 1.0 - std::sqrt(d_r_in / r);
        }

        double pre = (3.0 * BlackHole::G * spacetime_mass() * d_mdot)
                   / (8.0 * M_PI * r * r * r);
        return pre * f;
    }

    // ------------------------------------------------------------------ //
    //  Kerr cubic roots                                                   //
    // ------------------------------------------------------------------ //
    static void SolveKerrRoots(double a_star,
                                double& x1, double& x2, double& x3) noexcept
    {
        constexpr double two_pi = 2.0 * M_PI;
        double phi_k = std::acos(-a_star) / 3.0;
        double A     = 2.0 * std::sqrt(3.0);
        x1 = A * std::cos(phi_k);
        x2 = A * std::cos(phi_k - two_pi / 3.0);
        x3 = A * std::cos(phi_k - 2.0 * two_pi / 3.0);
    }

    // ------------------------------------------------------------------ //
    //  Value noise — deterministic hash-based 2D smooth noise            //
    // ------------------------------------------------------------------ //
    static double Hash(int ix, int iy) noexcept
    {
        unsigned h = static_cast<unsigned>(ix * 1619 + iy * 31337 + ix * iy * 7919);
        h ^= h >> 16;
        h *= 0x45d9f3bu;
        h ^= h >> 16;
        return (h & 0xFFFFFFu) / double(0xFFFFFFu);
    }

    static double ValueNoise2D(double x, double y) noexcept
    {
        int ix = static_cast<int>(std::floor(x));
        int iy = static_cast<int>(std::floor(y));
        double fx = x - ix;
        double fy = y - iy;

        double ux = fx * fx * (3.0 - 2.0 * fx);
        double uy = fy * fy * (3.0 - 2.0 * fy);

        double v00 = Hash(ix,     iy    );
        double v10 = Hash(ix + 1, iy    );
        double v01 = Hash(ix,     iy + 1);
        double v11 = Hash(ix + 1, iy + 1);

        return v00 + (v10-v00)*ux + (v01-v00)*uy + (v00-v10-v01+v11)*ux*uy;
    }

    // ------------------------------------------------------------------ //
    //  Stochastic streak modulation                                      //
    //                                                                     //
    //  Warped layered value noise in a twisted polar coordinate space.   //
    //  The radial twist shears inner features more than outer ones,      //
    //  producing spiral arms. Domain warping breaks up regularity into   //
    //  turbulent filaments.                                               //
    //                                                                     //
    //  Returns a value in [0, 1] used for both brightness and opacity.   //
    // ------------------------------------------------------------------ //
    [[nodiscard]] double StreakModulation(double r, double phi) const noexcept
    {
        double r_n = std::max(r / d_r_in, 1.0);  // clamp — never below inner edge

        double twist_angle = phi + (15.0 / std::pow(r_n, 1.5));

        double scale = 4.0 * 1.0137;
        double cx = r_n * std::cos(twist_angle) * scale + 1.7321;
        double cy = r_n * std::sin(twist_angle) * scale + 2.7183;

        double nx = cx + ValueNoise2D(cx * 0.3, cy * 0.3) * 0.5;
        double ny = cy + ValueNoise2D(cy * 0.3, cx * 0.3) * 0.5;

        auto Ridge = [&](double x, double y, double f) {
            double v = ValueNoise2D(x * f, y * f);
            v = 1.0 - std::abs(v * 2.0 - 1.0);
            return v * v;
        };

        double n = Ridge(nx, ny, 1.0) * 0.6 + Ridge(nx, ny, 2.5) * 0.4;
        n = 0.1 + n * 0.9;

        auto smoothstep = [](double e0, double e1, double x) {
            double t = std::clamp((x - e0) / (e1 - e0), 0.0, 1.0);
            return t * t * (3.0 - 2.0 * t);
        };

        double inner_fade = smoothstep(d_r_in, d_r_in * 1.05, r);
        double outer_fade = 1.0 - smoothstep(d_r_out * 0.75, d_r_out, r);

        return n * inner_fade * outer_fade;
    }
    
    // ------------------------------------------------------------------ //
    //  Fill hit record                                                    //
    // ------------------------------------------------------------------ //
    void FillRecord(HitRecord& rec, const Ray& ray,
                    double t, double r, double phi, Math::Vec3 n) const noexcept
    {   
        // Hide degenerate inner-edge pixels where flux → 0
        if (r < 1.05 * d_r_in) {
            rec.coverage = 0.0;
            return;
        }

        double flux = PageThorneFlux(r);
        double modulation = StreakModulation(r, phi);
        
        // Ensure a baseline temperature for the outer disk so it isn't pitch black
        constexpr double sigma = 5.670374419e-8;
        double T_phys = (flux > 0.0) ? std::pow(flux / sigma, 0.25) : 0.0;
        double T_eff = std::max(T_phys * d_fcol, 1500.0); // Baseline 1500K glow

        Color disk_color = Color::FromBlackbody(T_eff);

        double flux_norm = flux / (d_flux_peak + 1e-20);
        double emissivity = (flux_norm * 4.0) + (modulation * 0.8) + 0.1;

        double density = 0.3 + 0.7 * StreakModulation(r, phi);

        // correct hit point (fixes subtle rendering bug)
        Math::Vec3 hit_point = Spatial(ray.origin) + t * ray.Direction();

        // Vertical falloff → volumetric feel
        double z = std::abs(hit_point.Z - d_center.Z);
        double vertical = 1.0; 
        if (d_half_h > 1e-6) {
            double z = std::abs(hit_point.Z - d_center.Z);
            // Clamp to avoid values > 1.0 or < 0.0 before cos
            double ratio = std::clamp(z / d_half_h, 0.0, 1.0);
            vertical = std::cos(ratio * (M_PI * 0.5));
        }

        rec.lambda = t;
        rec.point  = ray.origin + t * ray.momentum;
        rec.normal = n;

        // Semi-transparent gas (not solid)
        rec.coverage = std::clamp(modulation * vertical, 0.0, 1.0);
        rec.mat = std::make_shared<Emissive>(disk_color, emissivity);
    }

    // ------------------------------------------------------------------ //
    //  Geometry helpers                                                   //
    // ------------------------------------------------------------------ //
    static Math::Vec3 Spatial(const Math::Vec4& v) noexcept
    {
        return {v.X, v.Y, v.Z};
    }

    void TryCap(const Math::Vec3& ro, const Math::Vec3& dir,
                double t_min, double t_max,
                double z_plane, Math::Vec3 n,
                double& best_t, Math::Vec3& best_n) const noexcept
    {
        double t = (z_plane - ro.Z) / dir.Z;
        if (t < t_min || t > t_max) return;
        Math::Vec3 hit = ro + t * dir;
        double r = std::sqrt(hit.X*hit.X + hit.Y*hit.Y);
        if (r < d_r_in || r > d_r_out) return;
        if (best_t < 0.0 || t < best_t) { best_t = t; best_n = n; }
    }

    void TryCylinder(const Math::Vec3& ro, const Math::Vec3& dir,
                     double t_min, double t_max,
                     double R, bool inward,
                     double& best_t, Math::Vec3& best_n) const noexcept
    {
        double a = dir.X*dir.X + dir.Y*dir.Y;
        if (a < 1e-12) return;
        double bh   = ro.X*dir.X + ro.Y*dir.Y;
        double c    = ro.X*ro.X + ro.Y*ro.Y - R*R;
        double disc = bh*bh - a*c;
        if (disc < 0.0) return;
        double sq = std::sqrt(disc);
        for (double t : {(-bh - sq)/a, (-bh + sq)/a})
        {
            if (t < t_min || t > t_max) continue;
            if (std::abs(ro.Z + t * dir.Z) > d_half_h) continue;
            if (best_t < 0.0 || t < best_t)
            {
                Math::Vec3 hit = ro + t * dir;
                double inv = inward ? -1.0/R : 1.0/R;
                best_n = {hit.X * inv, hit.Y * inv, 0.0};
                best_t = t;
            }
        }
    }

    [[nodiscard]] double spacetime_mass() const noexcept
    {
        if (auto* s = dynamic_cast<const BlackHole::Schwarzschild*>(&d_spacetime))
            return s->params.M;
        if (auto* k = dynamic_cast<const BlackHole::Kerr*>(&d_spacetime))
            return k->params.M;
        return 0.0;
    }

    // ------------------------------------------------------------------ //
    //  Members                                                            //
    // ------------------------------------------------------------------ //
    const BlackHole::Spacetime& d_spacetime;
    Math::Vec4                  d_center;
    double                      d_mdot;
    double                      d_r_in;
    double                      d_r_out;
    double                      d_rg;
    double                      d_fcol;
    double                      d_half_h;
    double                      d_r_peak;
    double                      d_flux_peak;
};