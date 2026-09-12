#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

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

TEST_CASE("arrowhead_wing_points returns symmetric wings for a known tail/head/fraction",
          "[polar_plot][arrowhead]") {
    SECTION("horizontal shaft") {
        const polarplot::ArrowheadWings wings = polarplot::arrowhead_wing_points(
            polarplot::Point{0.0, 0.0}, polarplot::Point{10.0, 0.0}, 0.5);
        REQUIRE_THAT(wings.first.x, WithinAbs(5.0, 1e-12));
        REQUIRE_THAT(wings.first.y, WithinAbs(-2.0, 1e-12));
        REQUIRE_THAT(wings.second.x, WithinAbs(5.0, 1e-12));
        REQUIRE_THAT(wings.second.y, WithinAbs(2.0, 1e-12));
    }

    SECTION("vertical shaft") {
        const polarplot::ArrowheadWings wings = polarplot::arrowhead_wing_points(
            polarplot::Point{0.0, 0.0}, polarplot::Point{0.0, 10.0}, 0.5);
        REQUIRE_THAT(wings.first.x, WithinAbs(2.0, 1e-12));
        REQUIRE_THAT(wings.first.y, WithinAbs(5.0, 1e-12));
        REQUIRE_THAT(wings.second.x, WithinAbs(-2.0, 1e-12));
        REQUIRE_THAT(wings.second.y, WithinAbs(5.0, 1e-12));
    }
}

TEST_CASE("arrowhead_wing_points collapses to the head for a zero-length segment",
          "[polar_plot][arrowhead]") {
    const polarplot::Point coincident{3.0, 4.0};
    const polarplot::ArrowheadWings wings =
        polarplot::arrowhead_wing_points(coincident, coincident, 0.5);
    REQUIRE_THAT(wings.first.x, WithinAbs(3.0, 1e-12));
    REQUIRE_THAT(wings.first.y, WithinAbs(4.0, 1e-12));
    REQUIRE_THAT(wings.second.x, WithinAbs(3.0, 1e-12));
    REQUIRE_THAT(wings.second.y, WithinAbs(4.0, 1e-12));
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

namespace {
// Mirrors the label-radius bump used by polarplot::spoke_labels: labels sit
// just outside the outer ring rather than exactly on it.
constexpr double kLabelRadiusFactor = 1.08;
}  // namespace

TEST_CASE("spoke_labels places each of the 12 default spokes under the identity convention",
          "[polar_plot][spoke_labels]") {
    constexpr AngleConvention identity{.zero_direction = 0.0, .angle_sign = 1.0};
    constexpr double kMaxRadius = 5.0;

    const std::vector<polarplot::SpokeLabel> labels = polarplot::spoke_labels(kMaxRadius, identity);

    REQUIRE(labels.size() == 12);

    const std::array<const char*, 12> expected_text{"0°",   "30°",  "60°",  "90°",  "120°", "150°",
                                                    "180°", "210°", "240°", "270°", "300°", "330°"};

    for (std::size_t s = 0; s < labels.size(); ++s) {
        CAPTURE(s);
        CHECK(labels[s].text == expected_text[s]);

        const double t = kTwoPi * static_cast<double>(s) / 12.0;
        const double expected_radius = kMaxRadius * kLabelRadiusFactor;
        REQUIRE_THAT(labels[s].position.x, WithinAbs(expected_radius * std::cos(t), 1e-9));
        REQUIRE_THAT(labels[s].position.y, WithinAbs(expected_radius * std::sin(t), 1e-9));
    }
}

TEST_CASE("spoke_labels text stays the math angle while position follows the convention",
          "[polar_plot][spoke_labels]") {
    constexpr double kMaxRadius = 2.0;
    constexpr double kExpectedRadius = kMaxRadius * kLabelRadiusFactor;

    SECTION("a rotated zero_direction moves the position but not the '0°' text") {
        constexpr AngleConvention rotated{.zero_direction = kPi / 2.0, .angle_sign = 1.0};
        const std::vector<polarplot::SpokeLabel> labels =
            polarplot::spoke_labels(kMaxRadius, rotated);

        REQUIRE(labels.size() == 12);
        CHECK(labels[0].text == "0°");
        REQUIRE_THAT(labels[0].position.x, WithinAbs(0.0, 1e-9));
        REQUIRE_THAT(labels[0].position.y, WithinAbs(kExpectedRadius, 1e-9));

        CHECK(labels[1].text == "30°");
        const double expected_angle = (kPi / 2.0) + (kTwoPi / 12.0);
        REQUIRE_THAT(labels[1].position.x,
                     WithinAbs(kExpectedRadius * std::cos(expected_angle), 1e-9));
        REQUIRE_THAT(labels[1].position.y,
                     WithinAbs(kExpectedRadius * std::sin(expected_angle), 1e-9));
    }

    SECTION("a flipped angle_sign mirrors which way positions increase, but text is unchanged") {
        constexpr AngleConvention flipped{.zero_direction = 0.0, .angle_sign = -1.0};
        const std::vector<polarplot::SpokeLabel> labels =
            polarplot::spoke_labels(kMaxRadius, flipped);

        REQUIRE(labels.size() == 12);
        CHECK(labels[0].text == "0°");
        REQUIRE_THAT(labels[0].position.x, WithinAbs(kExpectedRadius, 1e-9));
        REQUIRE_THAT(labels[0].position.y, WithinAbs(0.0, 1e-9));

        CHECK(labels[1].text == "30°");
        const double t = kTwoPi / 12.0;
        REQUIRE_THAT(labels[1].position.x, WithinAbs(kExpectedRadius * std::cos(-t), 1e-9));
        REQUIRE_THAT(labels[1].position.y, WithinAbs(kExpectedRadius * std::sin(-t), 1e-9));
    }
}

TEST_CASE("ruler_ticks places one tick per ring at multiples of ring_interval",
          "[polar_plot][ruler_ticks]") {
    const std::vector<polarplot::RulerTick> ticks = polarplot::ruler_ticks(2.0, 4);

    REQUIRE(ticks.size() == 4);
    REQUIRE_THAT(ticks[0].position, WithinAbs(2.0, 1e-12));
    REQUIRE_THAT(ticks[1].position, WithinAbs(4.0, 1e-12));
    REQUIRE_THAT(ticks[2].position, WithinAbs(6.0, 1e-12));
    REQUIRE_THAT(ticks[3].position, WithinAbs(8.0, 1e-12));
    CHECK(ticks[0].label == "2");
    CHECK(ticks[1].label == "4");
    CHECK(ticks[2].label == "6");
    CHECK(ticks[3].label == "8");
}

TEST_CASE(
    "rotation_indicator_arc is centered on the right/3-o'clock reference, "
    "distinct from draw_angle_arc's top reference",
    "[polar_plot][rotation_indicator]") {
    SECTION("positive sweep_sign sweeps counterclockwise from 0") {
        const polarplot::RotationIndicatorArc arc = polarplot::rotation_indicator_arc(1.0);
        REQUIRE_THAT(arc.from_angle, WithinAbs(0.0, 1e-12));
        CHECK(arc.to_angle > arc.from_angle);
    }

    SECTION("negative sweep_sign sweeps clockwise from 0") {
        const polarplot::RotationIndicatorArc arc = polarplot::rotation_indicator_arc(-1.0);
        REQUIRE_THAT(arc.from_angle, WithinAbs(0.0, 1e-12));
        CHECK(arc.to_angle < arc.from_angle);
    }

    SECTION("only the sign matters, not the magnitude") {
        const polarplot::RotationIndicatorArc small = polarplot::rotation_indicator_arc(0.001);
        const polarplot::RotationIndicatorArc large = polarplot::rotation_indicator_arc(1000.0);
        REQUIRE_THAT(small.to_angle, WithinAbs(large.to_angle, 1e-12));
    }

    SECTION("exactly zero is treated as counterclockwise") {
        const polarplot::RotationIndicatorArc arc = polarplot::rotation_indicator_arc(0.0);
        CHECK(arc.to_angle > arc.from_angle);
    }
}

TEST_CASE("ruler_ticks respects a custom ring_count", "[polar_plot][ruler_ticks]") {
    const std::vector<polarplot::RulerTick> ticks = polarplot::ruler_ticks(0.5, 2);

    REQUIRE(ticks.size() == 2);
    REQUIRE_THAT(ticks[0].position, WithinAbs(0.5, 1e-12));
    REQUIRE_THAT(ticks[1].position, WithinAbs(1.0, 1e-12));
    CHECK(ticks[0].label == "0.5");
    CHECK(ticks[1].label == "1");
}
