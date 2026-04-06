#pragma once

#include <vector>
#include <memory>
#include "hittable.h"
#include "geodesic_segment.h"

class HittableList : public Hittable {
public:
    HittableList() = default;

    void Add(std::shared_ptr<Hittable> object)
    {
        objects.push_back(object);
    }

    void Clear()
    {
        objects.clear();
    }

    [[nodiscard]]
    bool Intersect(const Ray& ray,
                   double lambda_min,
                   double lambda_max,
                   HitRecord& rec) const noexcept override
    {
        HitRecord temp_rec;
        bool hit_anything = false;
        double closest_so_far = lambda_max;

        for (const auto& object : objects)
        {
            if (object->Intersect(ray,
                                  lambda_min,
                                  closest_so_far,
                                  temp_rec))
            {
                hit_anything = true;
                closest_so_far = temp_rec.lambda;
                rec = temp_rec;
            }
        }

        return hit_anything;
    }
    
    // Better intersection
    bool IntersectSegment(const GeodesicSegment& seg, HitRecord& rec) const
    {
        constexpr int N = 8;

        for (int i = 0; i <= N; ++i)
        {
            double t = double(i) / double(N);

            Math::Vec4 x = seg.a.x * (1.0 - t) + seg.b.x * t;

            Ray ray(x, seg.a.p); // tangent approximation

            if (Intersect(ray, 0.0, 1e7, rec))
                return true;
        }

        return false;
    }
private:
    std::vector<std::shared_ptr<Hittable>> objects;
};
