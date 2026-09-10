#include "vector_math/vec2.hpp"

#include <algorithm>
#include <cmath>

namespace vecmath {

double magnitude(Vec2 v) { return std::hypot(v.x, v.y); }

double angle(Vec2 v) {
    if (is_zero(v)) {
        return 0.0;
    }
    return std::atan2(v.y, v.x);
}

Vec2 from_polar(double radius, double angle_rad) {
    return {radius * std::cos(angle_rad), radius * std::sin(angle_rad)};
}

Vec2 from_polar(Polar p) { return from_polar(p.radius, p.angle_rad); }

Polar to_polar(Vec2 v) { return {magnitude(v), angle(v)}; }

bool is_zero(Vec2 v, double epsilon) { return magnitude_squared(v) <= epsilon * epsilon; }

Vec2 normalized(Vec2 v) {
    const double len = magnitude(v);
    if (len == 0.0) {
        return {0.0, 0.0};
    }
    return v / len;
}

double angle_between(Vec2 a, Vec2 b) {
    const double denom = magnitude(a) * magnitude(b);
    if (denom == 0.0) {
        return 0.0;
    }
    // Clamp guards against round-off pushing the ratio outside [-1, 1].
    const double c = std::clamp(dot(a, b) / denom, -1.0, 1.0);
    return std::acos(c);
}

}  // namespace vecmath
