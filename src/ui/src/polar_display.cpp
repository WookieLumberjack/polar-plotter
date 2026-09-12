#include "ui/polar_display.hpp"

#include <cmath>
#include <numbers>

namespace ui {
namespace {

constexpr double kRadToDeg = 180.0 / std::numbers::pi;
constexpr double kDegToRad = std::numbers::pi / 180.0;

/// Normalize \p degrees to [0, 360).
float normalize_degrees_0_360(float degrees) {
    float wrapped = std::fmod(degrees, 360.0F);
    if (wrapped < 0.0F) {
        wrapped += 360.0F;
    }
    return wrapped;
}

}  // namespace

PolarDisplay to_polar_display(vecmath::Vec2 v) {
    const vecmath::Polar polar = vecmath::to_polar(v);
    return {static_cast<float>(polar.radius),
            normalize_degrees_0_360(static_cast<float>(polar.angle_rad * kRadToDeg))};
}

vecmath::Vec2 from_polar_display(PolarDisplay display) {
    return vecmath::from_polar(static_cast<double>(display.amplitude),
                               static_cast<double>(display.phase_deg) * kDegToRad);
}

PolarDisplay canonicalize_polar_display(PolarDisplay display) {
    float amplitude = display.amplitude;
    float phase_deg = display.phase_deg;
    if (amplitude < 0.0F) {
        amplitude = -amplitude;
        phase_deg += 180.0F;
    }
    return {amplitude, normalize_degrees_0_360(phase_deg)};
}

}  // namespace ui
