#include <cmath>
#include <numbers>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "polar_plotting/polar_plot.hpp"

using Catch::Matchers::WithinAbs;
using polarplot::AngleConvention;

namespace {
constexpr double kPi = std::numbers::pi;
constexpr double kTwoPi = 2.0 * kPi;
}  // namespace

TEST_CASE("angle convention identity is the current default behavior",
          "[polar_plot][angle_convention]") {
    constexpr AngleConvention convention{.zero_direction = 0.0, .angle_sign = 1.0};

    REQUIRE_THAT(polarplot::apply_angle_convention(0.0, convention), WithinAbs(0.0, 1e-12));
    REQUIRE_THAT(polarplot::apply_angle_convention(kPi / 2.0, convention),
                 WithinAbs(kPi / 2.0, 1e-12));
    REQUIRE_THAT(polarplot::apply_angle_convention(kPi, convention), WithinAbs(kPi, 1e-12));
}

TEST_CASE("angle convention offsets by zero_direction", "[polar_plot][angle_convention]") {
    constexpr AngleConvention convention{.zero_direction = kPi / 2.0, .angle_sign = 1.0};

    REQUIRE_THAT(polarplot::apply_angle_convention(0.0, convention), WithinAbs(kPi / 2.0, 1e-12));
    REQUIRE_THAT(polarplot::apply_angle_convention(kPi / 2.0, convention), WithinAbs(kPi, 1e-12));
}

TEST_CASE("angle convention flips direction with a negative angle_sign",
          "[polar_plot][angle_convention]") {
    constexpr AngleConvention convention{.zero_direction = 0.0, .angle_sign = -1.0};

    REQUIRE_THAT(polarplot::apply_angle_convention(kPi / 2.0, convention),
                 WithinAbs(3.0 * kPi / 2.0, 1e-12));
    REQUIRE_THAT(polarplot::apply_angle_convention(kPi, convention), WithinAbs(kPi, 1e-12));
}

TEST_CASE("angle convention normalizes to a full turn at the 0/2pi boundary",
          "[polar_plot][angle_convention]") {
    SECTION("exactly one full turn wraps to zero") {
        constexpr AngleConvention convention{.zero_direction = 0.0, .angle_sign = 1.0};
        REQUIRE_THAT(polarplot::apply_angle_convention(kTwoPi, convention), WithinAbs(0.0, 1e-9));
    }

    SECTION("zero_direction plus math angle crossing 2pi wraps forward") {
        constexpr AngleConvention convention{.zero_direction = 3.0 * kPi / 2.0, .angle_sign = 1.0};
        // 3pi/2 + pi = 5pi/2, which should wrap to pi/2.
        REQUIRE_THAT(polarplot::apply_angle_convention(kPi, convention),
                     WithinAbs(kPi / 2.0, 1e-9));
    }

    SECTION("negative results wrap backward into [0, 2pi)") {
        constexpr AngleConvention convention{.zero_direction = -kPi / 4.0, .angle_sign = 1.0};
        REQUIRE_THAT(polarplot::apply_angle_convention(0.0, convention),
                     WithinAbs(7.0 * kPi / 4.0, 1e-9));
    }

    SECTION("negative angle_sign crossing the boundary also wraps forward") {
        constexpr AngleConvention convention{.zero_direction = 0.0, .angle_sign = -1.0};
        // -(-pi/4) would be pi/4, but math_angle here is positive small: use a case that
        // actually goes negative: angle_sign * math_angle = -(pi/6).
        REQUIRE_THAT(polarplot::apply_angle_convention(kPi / 6.0, convention),
                     WithinAbs(kTwoPi - (kPi / 6.0), 1e-9));
    }
}

TEST_CASE("to_plotted_point preserves radius and remaps angle", "[polar_plot][angle_convention]") {
    constexpr AngleConvention identity{.zero_direction = 0.0, .angle_sign = 1.0};
    const polarplot::Point p{1.0, 0.0};
    const polarplot::Point mapped = polarplot::to_plotted_point(p, identity);
    REQUIRE_THAT(mapped.x, WithinAbs(1.0, 1e-12));
    REQUIRE_THAT(mapped.y, WithinAbs(0.0, 1e-12));

    SECTION("a quarter-turn zero_direction rotates the point") {
        constexpr AngleConvention rotated{.zero_direction = kPi / 2.0, .angle_sign = 1.0};
        const polarplot::Point out = polarplot::to_plotted_point(p, rotated);
        REQUIRE_THAT(out.x, WithinAbs(0.0, 1e-9));
        REQUIRE_THAT(out.y, WithinAbs(1.0, 1e-9));
    }

    SECTION("the origin is unaffected by any convention") {
        constexpr AngleConvention rotated{.zero_direction = kPi / 3.0, .angle_sign = -1.0};
        const polarplot::Point out =
            polarplot::to_plotted_point(polarplot::Point{0.0, 0.0}, rotated);
        REQUIRE_THAT(out.x, WithinAbs(0.0, 1e-12));
        REQUIRE_THAT(out.y, WithinAbs(0.0, 1e-12));
    }
}
