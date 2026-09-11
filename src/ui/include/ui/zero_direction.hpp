#ifndef UI_ZERO_DIRECTION_HPP
#define UI_ZERO_DIRECTION_HPP

#include <cstdint>

/// \file
/// Pure conversion between the raw zero-direction angle (as stored by
/// App::zero_direction_deg_) and a plain-language "N degrees left/right of
/// top" view of the same value. No ImGui/ImPlot dependency -- this is the
/// independently-testable seam for the plain-language zero-direction input.

namespace ui {

/// Which side of "top" (12 o'clock) a plain-language zero-direction offset
/// lies on.
enum class ZeroDirectionSide : std::uint8_t { kLeft, kRight };

/// A zero-direction angle expressed in plain language: "N degrees left/right
/// of top". \p degrees_from_top is always in [0, 180].
struct PlainZeroDirection {
    ZeroDirectionSide side{ZeroDirectionSide::kRight};
    float degrees_from_top{0.0F};
};

/// Convert a raw zero-direction angle (degrees, the same raw angle passed as
/// \c AngleConvention::zero_direction -- 0 deg is the plot's raw +x axis)
/// into its plain-language "N deg left/right of top" form. Top is 90 deg in
/// that raw frame; increasing raw angle sweeps counterclockwise, which reads
/// as moving left on screen, so raw angles above 90 deg are "left of top" and
/// raw angles below 90 deg are "right of top".
///
/// \p raw_degrees may be any finite value (including negative or beyond a
/// full turn); the result is always normalized to the principal value.
/// Exactly at top (0 deg from top) and exactly opposite top (180 deg from
/// top, where "left" and "right" coincide) are inherently ambiguous; both
/// pick a canonical side (kRight and kLeft respectively) so the conversion
/// stays a well-defined function.
[[nodiscard]] PlainZeroDirection raw_to_plain_zero_direction(float raw_degrees);

/// Inverse of \ref raw_to_plain_zero_direction: recover the raw zero-direction
/// angle (degrees) implied by a plain-language offset from top.
[[nodiscard]] float plain_to_raw_zero_direction(PlainZeroDirection plain);

}  // namespace ui

#endif  // UI_ZERO_DIRECTION_HPP
