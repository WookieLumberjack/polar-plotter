#include "ui/angle_convention.hpp"

namespace ui {
namespace {

double to_sign(RotationDirection direction) {
    return direction == RotationDirection::CounterClockwise ? 1.0 : -1.0;
}

double to_sign(MeasurementConvention convention) {
    return convention == MeasurementConvention::WithRotation ? 1.0 : -1.0;
}

}  // namespace

double compose_angle_sign(RotationDirection rotation_direction,
                          MeasurementConvention measurement_convention) {
    return to_sign(rotation_direction) * to_sign(measurement_convention);
}

}  // namespace ui
