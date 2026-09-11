#include "ui/zero_direction.hpp"

#include <cmath>

namespace ui {
namespace {

/// Normalize \p degrees to (-180, 180].
float normalize_signed_degrees(float degrees) {
    float wrapped = std::fmod(degrees, 360.0F);
    if (wrapped <= -180.0F) {
        wrapped += 360.0F;
    } else if (wrapped > 180.0F) {
        wrapped -= 360.0F;
    }
    return wrapped;
}

}  // namespace

PlainZeroDirection raw_to_plain_zero_direction(float raw_degrees) {
    const float offset_from_top = normalize_signed_degrees(raw_degrees - 90.0F);

    if (offset_from_top > 0.0F) {
        return {ZeroDirectionSide::kLeft, offset_from_top};
    }
    if (offset_from_top < 0.0F) {
        return {ZeroDirectionSide::kRight, -offset_from_top};
    }
    return {ZeroDirectionSide::kRight, 0.0F};
}

float plain_to_raw_zero_direction(PlainZeroDirection plain) {
    const float signed_offset =
        plain.side == ZeroDirectionSide::kLeft ? plain.degrees_from_top : -plain.degrees_from_top;
    return 90.0F + signed_offset;
}

}  // namespace ui
