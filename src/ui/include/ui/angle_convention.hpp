#ifndef UI_ANGLE_CONVENTION_HPP
#define UI_ANGLE_CONVENTION_HPP

#include <cstdint>

/// \file
/// The two independent, learner-facing toggles that compose into the single
/// `angle_sign` `polar_plotting` needs (see
/// docs/adr/0001-polar-plotting-receives-only-composed-angle-sign.md).
/// `polar_plotting` never sees \ref RotationDirection or
/// \ref MeasurementConvention individually -- only the composed result of
/// \ref compose_angle_sign.

namespace ui {

/// Which way the plot's rotation runs, as chosen by the learner.
enum class RotationDirection : std::uint8_t {
    Clockwise,
    CounterClockwise,
};

/// Whether a plotted angle increases with the rotation direction or against
/// it, as chosen by the learner.
enum class MeasurementConvention : std::uint8_t {
    WithRotation,
    AgainstRotation,
};

/// Compose the two independent toggles into the single angle sign
/// `polar_plotting` needs: `angle_sign = rotation_direction * measurement_convention`,
/// each represented as +-1 (`CounterClockwise`/`WithRotation` map to +1,
/// `Clockwise`/`AgainstRotation` map to -1). Pure function -- the test seam
/// for this composition; `angle_sign` must always be derived by calling this,
/// never set independently.
[[nodiscard]] double compose_angle_sign(RotationDirection rotation_direction,
                                        MeasurementConvention measurement_convention);

}  // namespace ui

#endif  // UI_ANGLE_CONVENTION_HPP
