#ifndef CONSTANT_MEDIUM_H
#define CONSTANT_MEDIUM_H

#include "hittable.h"
#include "raytracer/materials/material.h"
#include "raytracer/textures/texture.h"

class constant_medium : public hittable {
  public:
    constant_medium(shared_ptr<hittable> boundary, double density, shared_ptr<texture> tex)
      : boundary(boundary), neg_inv_density(-1/density),
        phase_function(make_shared<isotropic>(tex))
    {}

    constant_medium(shared_ptr<hittable> boundary, double density, const color& albedo)
      : boundary(boundary), neg_inv_density(-1/density),
        phase_function(make_shared<isotropic>(albedo))
    {}

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        hit_record rec1, rec2;

        if (!boundary->hit(r, interval::universe, rec1))
            return false;

        if (!boundary->hit(r, interval(rec1.t+0.0001, infinity), rec2))
            return false;

        if (rec1.t < ray_t.min) rec1.t = ray_t.min;
        if (rec2.t > ray_t.max) rec2.t = ray_t.max;

        if (rec1.t >= rec2.t)
            return false;

        if (rec1.t < 0)
            rec1.t = 0;

        auto ray_length = r.direction().length();
        auto distance_inside_boundary = (rec2.t - rec1.t) * ray_length;

        // deterministic RNG derived from the ray
        uint32_t seed = 0;
        seed ^= static_cast<uint32_t>(r.origin().x() * 73856093);
        seed ^= static_cast<uint32_t>(r.origin().y() * 19349663);
        seed ^= static_cast<uint32_t>(r.origin().z() * 83492791);
        seed ^= static_cast<uint32_t>(r.direction().x() * 2654435761);
        seed ^= static_cast<uint32_t>(r.direction().y() * 1597334677);
        seed ^= static_cast<uint32_t>(r.direction().z() * 3812015801);

        RNG rng(seed);

        auto hit_distance = neg_inv_density * std::log(rng.next_double());

        if (hit_distance > distance_inside_boundary)
            return false;

        rec.t = rec1.t + hit_distance / ray_length;
        rec.p = r.at(rec.t);

        rec.normal = vec3(1,0,0);  // arbitrary
        rec.front_face = true;     // also arbitrary
        rec.mat = phase_function;

        return true;
    }

    aabb bounding_box() const override { return boundary->bounding_box(); }

  private:
    shared_ptr<hittable> boundary;
    double neg_inv_density;
    shared_ptr<material> phase_function;
};

#endif