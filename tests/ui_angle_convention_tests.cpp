#include <catch2/catch_test_macros.hpp>

#include "ui/angle_convention.hpp"

using ui::compose_angle_sign;
using ui::MeasurementConvention;
using ui::RotationDirection;

TEST_CASE("angle sign is the product of rotation direction and measurement convention",
          "[angle_convention]") {
    CHECK(compose_angle_sign(RotationDirection::CounterClockwise,
                             MeasurementConvention::WithRotation) == 1.0);
    CHECK(compose_angle_sign(RotationDirection::CounterClockwise,
                             MeasurementConvention::AgainstRotation) == -1.0);
    CHECK(compose_angle_sign(RotationDirection::Clockwise, MeasurementConvention::WithRotation) ==
          -1.0);
    CHECK(compose_angle_sign(RotationDirection::Clockwise,
                             MeasurementConvention::AgainstRotation) == 1.0);
}

TEST_CASE("flipping either toggle alone flips the sign", "[angle_convention]") {
    const double baseline = compose_angle_sign(RotationDirection::CounterClockwise,
                                               MeasurementConvention::WithRotation);

    SECTION("rotation direction alone") {
        const double flipped =
            compose_angle_sign(RotationDirection::Clockwise, MeasurementConvention::WithRotation);
        CHECK(flipped == -baseline);
    }

    SECTION("measurement convention alone") {
        const double flipped = compose_angle_sign(RotationDirection::CounterClockwise,
                                                  MeasurementConvention::AgainstRotation);
        CHECK(flipped == -baseline);
    }

    SECTION("both toggles together returns to baseline") {
        const double flipped = compose_angle_sign(RotationDirection::Clockwise,
                                                  MeasurementConvention::AgainstRotation);
        CHECK(flipped == baseline);
    }
}
