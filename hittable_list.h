#pragma once

#include <vector>
#include <memory>
#include "hittable.h"

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

private:
    std::vector<std::shared_ptr<Hittable>> objects;
};
