#ifndef TEXTURE_H
#define TEXTURE_H

#include "raytracer/renderer/color.h"
#include "rtw_stb_image.h"
#include "perlin.h"

class texture {
    public:
        virtual ~texture() = default;

        virtual color value(double u, double v, const point3& p) const = 0;
};

class solid_color : public texture {
    public:
        solid_color(const color& albedo) : albedo(albedo) {}

        solid_color(double red, double green, double blue) : solid_color(color(red,green,blue)) {}

        color value(double u, double v, const point3& p) const override {
            return albedo;
}

    private:
        color albedo;
};

class checker_texture : public texture {
    public:
        checker_texture(double scale, shared_ptr<texture> even, shared_ptr<texture> odd)
        : inv_scale(1.0 / scale), even(even), odd(odd) {}

        checker_texture(double scale, const color& c1, const color& c2)
        : checker_texture(scale, make_shared<solid_color>(c1), make_shared<solid_color>(c2)) {}

        color value(double u, double v, const point3& p) const override {
            auto xInteger = int(std::floor(inv_scale * p.x()));
            auto yInteger = int(std::floor(inv_scale * p.y()));
            auto zInteger = int(std::floor(inv_scale * p.z()));

            bool isEven = (xInteger + yInteger + zInteger) % 2 == 0;

            return isEven ? even->value(u, v, p) : odd->value(u, v, p);
        }
    private:
        double inv_scale;
        shared_ptr<texture> even;
        shared_ptr<texture> odd;
};

class image_texture : public texture {
    public:
        image_texture(const char* filename) : image(filename) {}

        color value(double u, double v, const point3& p) const override {
            // If we have no texture data, then return solid cyan as a debugging aid.
            if (image.height() <= 0) return color(0,1,1);

            // Clamp input texture coordinates to [0,1] x [1,0]
            u = interval(0,1).clamp(u);
            v = 1.0 - interval(0,1).clamp(v);  // Flip V to image coordinates

            auto i = int(u * image.width());
            auto j = int(v * image.height());
            auto pixel = image.pixel_data(i,j);

            auto color_scale = 1.0 / 255.0;
            return color(color_scale*pixel[0], color_scale*pixel[1], color_scale*pixel[2]);
        }

    private:
        rtw_image image;
};

enum class noise_mode {
    plain,
    turbulence,
    marble,
    wood
};

class noise_texture : public texture {
  public:
    noise_texture(double scale,
                  noise_mode mode = noise_mode::plain,
                  int turb_depth = 7)
      : scale(scale), mode(mode), turb_depth(turb_depth) {}

    color value(double u, double v, const point3& p) const override {
        switch (mode) {
            case noise_mode::plain: {
                double n = 0.5 * (1.0 + noise.noise(scale * p));
                return color(1, 1, 1) * n;
            }

            case noise_mode::turbulence: {
                double t = noise.turb(scale * p, turb_depth);
                return color(1, 1, 1) * t;
            }

            case noise_mode::marble: {
                double t = noise.turb(scale * p, turb_depth);
                double m = std::sin(scale * p.z()+ 10 * t);
                double v = 0.5 * (1.0 + m);
                return color(0.5, 0.5, 0.5) * v;
            }

            case noise_mode::wood: {
                // Radial distance from Y-axis (tree trunk axis)
                double r = std::sqrt(p.x() * p.x() + p.z() * p.z());

                // Turbulence to warp rings
                double t = noise.turb(scale * p, turb_depth);

                // Rings distorted by turbulence
                double rings = std::sin(scale * r + 10 * t);

                // Map [-1,1] → [0,1]
                double vv = 0.5 * (1.0 + rings);

                // Brownish wood color
                color base(0.4, 0.2, 0.1);

                return base * vv;
            }
        }

        // Fallback (should never hit, but keeps compiler happy)
        return color(0, 0, 0);
    }

  private:
    perlin noise;
    double scale;
    noise_mode mode;
    int turb_depth;
};  

#endif