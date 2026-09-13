#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "polar_plotting/polar_plot.hpp"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
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

TEST_CASE("zero_direction_arc_tick straddles the arc's draw radius along the vertical spoke",
          "[polar_plot][zero_direction]") {
    // radius = extent * 0.85 (the arc's own draw radius, as computed by the
    // caller); half the tick length is 0.025 * extent.
    const polarplot::ZeroDirectionTick tick = polarplot::zero_direction_arc_tick(8.5, 10.0);
    REQUIRE_THAT(tick.inner.x, WithinAbs(0.0, 1e-12));
    REQUIRE_THAT(tick.inner.y, WithinAbs(8.25, 1e-12));
    REQUIRE_THAT(tick.outer.x, WithinAbs(0.0, 1e-12));
    REQUIRE_THAT(tick.outer.y, WithinAbs(8.75, 1e-12));
}

TEST_CASE("zero_direction_arc_tick collapses to a point at radius for a zero extent",
          "[polar_plot][zero_direction]") {
    const polarplot::ZeroDirectionTick tick = polarplot::zero_direction_arc_tick(5.0, 0.0);
    REQUIRE_THAT(tick.inner.x, WithinAbs(0.0, 1e-12));
    REQUIRE_THAT(tick.inner.y, WithinAbs(5.0, 1e-12));
    REQUIRE_THAT(tick.outer.x, WithinAbs(0.0, 1e-12));
    REQUIRE_THAT(tick.outer.y, WithinAbs(5.0, 1e-12));
}

TEST_CASE("head_frac_for_fixed_pixels converts a fixed pixel length into the matching fraction",
          "[polar_plot][arrowhead]") {
    SECTION("shaft much longer than the target: fraction is a small slice of it") {
        // 14px head on a 200px shaft => 0.07.
        CHECK_THAT(polarplot::head_frac_for_fixed_pixels(14.0, 200.0), WithinRel(0.07));
    }

    SECTION("shaft exactly the target length: would be 1.0 unclamped, so still hits the max") {
        CHECK_THAT(polarplot::head_frac_for_fixed_pixels(14.0, 14.0), WithinRel(0.9));
    }

    SECTION("shaft shorter than the target: clamped to the max fraction, never exceeding it") {
        // Would be > 1.0 unclamped (14 / 5 = 2.8); clamped to 0.9 so the head
        // never overshoots past the tail.
        CHECK_THAT(polarplot::head_frac_for_fixed_pixels(14.0, 5.0), WithinRel(0.9));
    }

    SECTION("zero or negative shaft length: no visible head rather than a division by zero") {
        CHECK(polarplot::head_frac_for_fixed_pixels(14.0, 0.0) == 0.0);
        CHECK(polarplot::head_frac_for_fixed_pixels(14.0, -3.0) == 0.0);
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

TEST_CASE("from_plotted_point round-trips with to_plotted_point",
          "[polar_plot][angle_convention]") {
    const std::vector<polarplot::Point> points{
        {0.0, 0.0}, {1.0, 0.0},  {0.0, 1.0},   {-1.0, 0.0}, {0.0, -1.0},
        {3.0, 4.0}, {-2.0, 5.0}, {-3.0, -4.0}, {5.0, -2.0}, {0.5, 0.25},
    };
    const std::vector<AngleConvention> conventions{
        {.zero_direction = 0.0, .angle_sign = 1.0},
        {.zero_direction = kPi / 2.0, .angle_sign = 1.0},
        {.zero_direction = kPi, .angle_sign = 1.0},
        {.zero_direction = 3.0 * kPi / 2.0, .angle_sign = 1.0},
        {.zero_direction = 0.0, .angle_sign = -1.0},
        {.zero_direction = kPi / 3.0, .angle_sign = -1.0},
        {.zero_direction = -kPi / 4.0, .angle_sign = 1.0},
        {.zero_direction = -kPi / 4.0, .angle_sign = -1.0},
    };

    for (const auto& p : points) {
        for (const auto& convention : conventions) {
            const polarplot::Point plotted = polarplot::to_plotted_point(p, convention);
            const polarplot::Point back = polarplot::from_plotted_point(plotted, convention);
            REQUIRE_THAT(back.x, WithinAbs(p.x, 1e-9));
            REQUIRE_THAT(back.y, WithinAbs(p.y, 1e-9));
        }
    }
}

TEST_CASE("snap_angle_to_increment preserves radius and snaps to the nearest increment",
          "[polar_plot][snap]") {
    constexpr double kIncrement = kPi / 12.0;  // 15 degrees.

    SECTION("exact multiple is unchanged") {
        const polarplot::Point p{std::cos(kIncrement * 2.0) * 3.0,
                                 std::sin(kIncrement * 2.0) * 3.0};
        const polarplot::Point snapped = polarplot::snap_angle_to_increment(p, kIncrement);
        REQUIRE_THAT(snapped.x, WithinAbs(p.x, 1e-9));
        REQUIRE_THAT(snapped.y, WithinAbs(p.y, 1e-9));
    }

    SECTION("angle just past a multiple snaps down to it, radius untouched") {
        const double radius = 7.0;
        const double angle = (kIncrement * 3.0) + (kIncrement * 0.1);
        const polarplot::Point p{radius * std::cos(angle), radius * std::sin(angle)};
        const polarplot::Point snapped = polarplot::snap_angle_to_increment(p, kIncrement);
        const double expected_angle = kIncrement * 3.0;
        REQUIRE_THAT(snapped.x, WithinAbs(radius * std::cos(expected_angle), 1e-9));
        REQUIRE_THAT(snapped.y, WithinAbs(radius * std::sin(expected_angle), 1e-9));
        REQUIRE_THAT(std::hypot(snapped.x, snapped.y), WithinAbs(radius, 1e-9));
    }

    SECTION("angle just before a multiple snaps up to it") {
        const double radius = 2.5;
        const double angle = (kIncrement * 5.0) - (kIncrement * 0.2);
        const polarplot::Point p{radius * std::cos(angle), radius * std::sin(angle)};
        const polarplot::Point snapped = polarplot::snap_angle_to_increment(p, kIncrement);
        const double expected_angle = kIncrement * 5.0;
        REQUIRE_THAT(snapped.x, WithinAbs(radius * std::cos(expected_angle), 1e-9));
        REQUIRE_THAT(snapped.y, WithinAbs(radius * std::sin(expected_angle), 1e-9));
    }

    SECTION("wraps correctly near the 0/2pi boundary") {
        const double radius = 1.0;
        // 354 degrees, closer to a full turn (360) than to 345; should snap to 0.
        const double angle = (kIncrement * 23.0) + (kIncrement * 0.6);
        const polarplot::Point p{radius * std::cos(angle), radius * std::sin(angle)};
        const polarplot::Point snapped = polarplot::snap_angle_to_increment(p, kIncrement);
        REQUIRE_THAT(snapped.x, WithinAbs(radius, 1e-9));
        REQUIRE_THAT(snapped.y, WithinAbs(0.0, 1e-9));
    }

    SECTION("the origin maps to itself regardless of increment") {
        const polarplot::Point snapped =
            polarplot::snap_angle_to_increment(polarplot::Point{0.0, 0.0}, kIncrement);
        REQUIRE_THAT(snapped.x, WithinAbs(0.0, 1e-12));
        REQUIRE_THAT(snapped.y, WithinAbs(0.0, 1e-12));
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

TEST_CASE(
    "rotation_indicator_label is angularly centered on the arc's sweep midpoint with a fixed "
    "\"Rot.\" text",
    "[polar_plot][rotation_indicator]") {
    constexpr double kExtent = 5.0;
    constexpr double kExpectedRadius = kExtent * kLabelRadiusFactor;

    SECTION("counterclockwise sweep") {
        const polarplot::RotationIndicatorLabel label =
            polarplot::rotation_indicator_label(kExtent, 1.0);
        CHECK(label.text == "Rot.");

        const polarplot::RotationIndicatorArc arc = polarplot::rotation_indicator_arc(1.0);
        const double mid_angle = (arc.from_angle + arc.to_angle) / 2.0;
        REQUIRE_THAT(label.position.x, WithinAbs(kExpectedRadius * std::cos(mid_angle), 1e-9));
        REQUIRE_THAT(label.position.y, WithinAbs(kExpectedRadius * std::sin(mid_angle), 1e-9));
    }

    SECTION("clockwise sweep") {
        const polarplot::RotationIndicatorLabel label =
            polarplot::rotation_indicator_label(kExtent, -1.0);
        CHECK(label.text == "Rot.");

        const polarplot::RotationIndicatorArc arc = polarplot::rotation_indicator_arc(-1.0);
        const double mid_angle = (arc.from_angle + arc.to_angle) / 2.0;
        REQUIRE_THAT(label.position.x, WithinAbs(kExpectedRadius * std::cos(mid_angle), 1e-9));
        REQUIRE_THAT(label.position.y, WithinAbs(kExpectedRadius * std::sin(mid_angle), 1e-9));
    }

    SECTION("only sweep_sign's sign matters, matching rotation_indicator_arc") {
        const polarplot::RotationIndicatorLabel small =
            polarplot::rotation_indicator_label(kExtent, 0.001);
        const polarplot::RotationIndicatorLabel large =
            polarplot::rotation_indicator_label(kExtent, 1000.0);
        REQUIRE_THAT(small.position.x, WithinAbs(large.position.x, 1e-9));
        REQUIRE_THAT(small.position.y, WithinAbs(large.position.y, 1e-9));
        CHECK(small.text == "Rot.");
        CHECK(large.text == "Rot.");
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

TEST_CASE("minor_ruler_ticks places subdivisions between the origin and each ring",
          "[polar_plot][minor_ruler_ticks]") {
    const std::vector<double> ticks = polarplot::minor_ruler_ticks(1.0, 2, 4);

    // 2 rings * 3 interior subdivisions per ring segment (quarters minus the
    // shared endpoint with the next major tick) = 6.
    REQUIRE(ticks.size() == 6);
    REQUIRE_THAT(ticks[0], WithinAbs(0.25, 1e-12));
    REQUIRE_THAT(ticks[1], WithinAbs(0.5, 1e-12));
    REQUIRE_THAT(ticks[2], WithinAbs(0.75, 1e-12));
    REQUIRE_THAT(ticks[3], WithinAbs(1.25, 1e-12));
    REQUIRE_THAT(ticks[4], WithinAbs(1.5, 1e-12));
    REQUIRE_THAT(ticks[5], WithinAbs(1.75, 1e-12));
}

TEST_CASE("minor_ruler_ticks scales with a custom ring_interval and subdivisions_per_ring",
          "[polar_plot][minor_ruler_ticks]") {
    const std::vector<double> ticks = polarplot::minor_ruler_ticks(2.0, 1, 5);

    REQUIRE(ticks.size() == 4);
    REQUIRE_THAT(ticks[0], WithinAbs(0.4, 1e-12));
    REQUIRE_THAT(ticks[1], WithinAbs(0.8, 1e-12));
    REQUIRE_THAT(ticks[2], WithinAbs(1.2, 1e-12));
    REQUIRE_THAT(ticks[3], WithinAbs(1.6, 1e-12));
}

TEST_CASE("minor_ruler_ticks yields no ticks when subdivisions_per_ring is 1 or less",
          "[polar_plot][minor_ruler_ticks]") {
    CHECK(polarplot::minor_ruler_ticks(1.0, 4, 1).empty());
    CHECK(polarplot::minor_ruler_ticks(1.0, 4, 0).empty());
}

TEST_CASE("PlotFrame's extent is derived from its ring_interval and ring_count",
          "[polar_plot][plot_frame]") {
    SECTION("default ring_count") {
        constexpr polarplot::PlotFrame frame{2.0, 4};
        REQUIRE_THAT(frame.extent(), WithinAbs(8.0, 1e-12));
    }

    SECTION("a non-default ring_count") {
        constexpr polarplot::PlotFrame frame{0.5, 3};
        REQUIRE_THAT(frame.extent(), WithinAbs(1.5, 1e-12));
    }
}

TEST_CASE("hover_hit_test misses when the mouse is outside both hit radii", "[polar_plot][hover]") {
    const polarplot::HoverTarget hit =
        polarplot::hover_hit_test(/*mouse=*/{100.0, 100.0}, /*tip_a=*/{0.0, 0.0},
                                  /*tip_b=*/{50.0, 0.0}, /*hit_radius=*/10.0);
    REQUIRE(hit == polarplot::HoverTarget::kNone);
}

TEST_CASE("hover_hit_test hits A when only A's tip is in range", "[polar_plot][hover]") {
    const polarplot::HoverTarget hit =
        polarplot::hover_hit_test(/*mouse=*/{2.0, 0.0}, /*tip_a=*/{0.0, 0.0},
                                  /*tip_b=*/{50.0, 0.0}, /*hit_radius=*/10.0);
    REQUIRE(hit == polarplot::HoverTarget::kA);
}

TEST_CASE("hover_hit_test hits B when only B's tip is in range", "[polar_plot][hover]") {
    const polarplot::HoverTarget hit =
        polarplot::hover_hit_test(/*mouse=*/{51.0, 0.0}, /*tip_a=*/{0.0, 0.0},
                                  /*tip_b=*/{50.0, 0.0}, /*hit_radius=*/10.0);
    REQUIRE(hit == polarplot::HoverTarget::kB);
}

TEST_CASE("hover_hit_test picks the nearer tip when both are in range", "[polar_plot][hover]") {
    // A is at distance 3 from the mouse, B at distance 8 -- both within the
    // radius 10 hit range, but A is nearer.
    const polarplot::HoverTarget nearer_a =
        polarplot::hover_hit_test(/*mouse=*/{0.0, 0.0}, /*tip_a=*/{3.0, 0.0},
                                  /*tip_b=*/{0.0, 8.0}, /*hit_radius=*/10.0);
    REQUIRE(nearer_a == polarplot::HoverTarget::kA);

    const polarplot::HoverTarget nearer_b =
        polarplot::hover_hit_test(/*mouse=*/{0.0, 0.0}, /*tip_a=*/{0.0, 8.0},
                                  /*tip_b=*/{3.0, 0.0}, /*hit_radius=*/10.0);
    REQUIRE(nearer_b == polarplot::HoverTarget::kB);
}

TEST_CASE("hover_hit_test favors A on an exact tie", "[polar_plot][hover]") {
    const polarplot::HoverTarget hit =
        polarplot::hover_hit_test(/*mouse=*/{0.0, 0.0}, /*tip_a=*/{5.0, 0.0},
                                  /*tip_b=*/{0.0, 5.0}, /*hit_radius=*/10.0);
    REQUIRE(hit == polarplot::HoverTarget::kA);
}

TEST_CASE("hover_hit_test treats the hit radius boundary as inclusive", "[polar_plot][hover]") {
    const polarplot::HoverTarget hit =
        polarplot::hover_hit_test(/*mouse=*/{0.0, 0.0}, /*tip_a=*/{10.0, 0.0},
                                  /*tip_b=*/{50.0, 0.0}, /*hit_radius=*/10.0);
    REQUIRE(hit == polarplot::HoverTarget::kA);
}

TEST_CASE("resolve_interaction_state stays idle when not hovered and not dragging",
          "[polar_plot][interaction]") {
    const polarplot::InteractionState state = polarplot::resolve_interaction_state(
        /*was_dragging=*/false, /*is_hover_target=*/false, /*mouse_pressed=*/false,
        /*mouse_down=*/false);
    REQUIRE(state == polarplot::InteractionState::kIdle);
}

TEST_CASE("resolve_interaction_state reports hovered when targeted but not pressed",
          "[polar_plot][interaction]") {
    const polarplot::InteractionState state = polarplot::resolve_interaction_state(
        /*was_dragging=*/false, /*is_hover_target=*/true, /*mouse_pressed=*/false,
        /*mouse_down=*/false);
    REQUIRE(state == polarplot::InteractionState::kHovered);
}

TEST_CASE("resolve_interaction_state starts dragging on press while hovered",
          "[polar_plot][interaction]") {
    const polarplot::InteractionState state = polarplot::resolve_interaction_state(
        /*was_dragging=*/false, /*is_hover_target=*/true, /*mouse_pressed=*/true,
        /*mouse_down=*/true);
    REQUIRE(state == polarplot::InteractionState::kDragging);
}

TEST_CASE("resolve_interaction_state does not start a drag from a press elsewhere",
          "[polar_plot][interaction]") {
    const polarplot::InteractionState state = polarplot::resolve_interaction_state(
        /*was_dragging=*/false, /*is_hover_target=*/false, /*mouse_pressed=*/true,
        /*mouse_down=*/true);
    REQUIRE(state == polarplot::InteractionState::kIdle);
}

TEST_CASE("resolve_interaction_state keeps dragging while the button stays held",
          "[polar_plot][interaction]") {
    // Once dragging, continues regardless of whether the cursor is still
    // within hit range of the (possibly moved) tip.
    const polarplot::InteractionState state = polarplot::resolve_interaction_state(
        /*was_dragging=*/true, /*is_hover_target=*/false, /*mouse_pressed=*/false,
        /*mouse_down=*/true);
    REQUIRE(state == polarplot::InteractionState::kDragging);
}

TEST_CASE("resolve_interaction_state releases when the button is let go mid-drag",
          "[polar_plot][interaction]") {
    const polarplot::InteractionState state = polarplot::resolve_interaction_state(
        /*was_dragging=*/true, /*is_hover_target=*/true, /*mouse_pressed=*/false,
        /*mouse_down=*/false);
    REQUIRE(state == polarplot::InteractionState::kReleased);
}

TEST_CASE("inflate_for_labels inflates a PlotFrame's extent by a fixed headroom factor",
          "[polar_plot][plot_frame]") {
    SECTION("scales proportionally to extent") {
        constexpr polarplot::PlotFrame frame{2.0, 4};
        const double inflated = polarplot::inflate_for_labels(frame);

        CHECK(inflated > frame.extent());
        REQUIRE_THAT(inflated / frame.extent(), WithinAbs(1.15, 1e-12));
    }

    SECTION("the padding factor is independent of ring_interval/ring_count, only extent matters") {
        constexpr polarplot::PlotFrame small{2.0, 1};
        constexpr polarplot::PlotFrame large{0.5, 4};

        REQUIRE_THAT(polarplot::inflate_for_labels(small),
                     WithinAbs(polarplot::inflate_for_labels(large), 1e-12));
    }
}

// #61: axis_half_ranges replaces ImPlotFlags_Equal so the rings stay
// circular (never stretched or clipped) at any canvas pixel aspect ratio.
TEST_CASE("axis_half_ranges on a square canvas gives equal half-ranges on both axes",
          "[polar_plot][axis_half_ranges]") {
    const polarplot::AxisHalfRanges ranges = polarplot::axis_half_ranges(800.0, 800.0, 5.0);
    REQUIRE_THAT(ranges.x, WithinAbs(5.0, 1e-12));
    REQUIRE_THAT(ranges.y, WithinAbs(5.0, 1e-12));
}

TEST_CASE("axis_half_ranges on a wide canvas gives x a larger half-range than y",
          "[polar_plot][axis_half_ranges]") {
    // Twice as wide as tall: y (the shorter screen dimension) gets exactly
    // half_range, x gets padded by the pixel excess.
    const polarplot::AxisHalfRanges ranges = polarplot::axis_half_ranges(1000.0, 500.0, 5.0);
    REQUIRE_THAT(ranges.y, WithinAbs(5.0, 1e-12));
    REQUIRE_THAT(ranges.x, WithinAbs(10.0, 1e-12));
    CHECK(ranges.x > ranges.y);
}

TEST_CASE("axis_half_ranges on a tall canvas gives y a larger half-range than x",
          "[polar_plot][axis_half_ranges]") {
    // Twice as tall as wide: x (the shorter screen dimension) gets exactly
    // half_range, y gets padded by the pixel excess.
    const polarplot::AxisHalfRanges ranges = polarplot::axis_half_ranges(500.0, 1000.0, 5.0);
    REQUIRE_THAT(ranges.x, WithinAbs(5.0, 1e-12));
    REQUIRE_THAT(ranges.y, WithinAbs(10.0, 1e-12));
    CHECK(ranges.y > ranges.x);
}

TEST_CASE("axis_half_ranges falls back to equal half-ranges for degenerate canvas dimensions",
          "[polar_plot][axis_half_ranges]") {
    SECTION("zero width") {
        const polarplot::AxisHalfRanges ranges = polarplot::axis_half_ranges(0.0, 600.0, 5.0);
        REQUIRE_THAT(ranges.x, WithinAbs(5.0, 1e-12));
        REQUIRE_THAT(ranges.y, WithinAbs(5.0, 1e-12));
    }

    SECTION("zero height") {
        const polarplot::AxisHalfRanges ranges = polarplot::axis_half_ranges(600.0, 0.0, 5.0);
        REQUIRE_THAT(ranges.x, WithinAbs(5.0, 1e-12));
        REQUIRE_THAT(ranges.y, WithinAbs(5.0, 1e-12));
    }

    SECTION("both dimensions zero") {
        const polarplot::AxisHalfRanges ranges = polarplot::axis_half_ranges(0.0, 0.0, 5.0);
        REQUIRE_THAT(ranges.x, WithinAbs(5.0, 1e-12));
        REQUIRE_THAT(ranges.y, WithinAbs(5.0, 1e-12));
    }

    SECTION("negative dimension") {
        const polarplot::AxisHalfRanges ranges = polarplot::axis_half_ranges(-100.0, 600.0, 5.0);
        REQUIRE_THAT(ranges.x, WithinAbs(5.0, 1e-12));
        REQUIRE_THAT(ranges.y, WithinAbs(5.0, 1e-12));
    }
}

// #49: manual-scale drag clamp -- a dragged tip can never leave the currently
// visible extent when auto-scale is off.
TEST_CASE("clamp_to_extent leaves a point inside the extent unchanged",
          "[polar_plot][interaction][clamp]") {
    const polarplot::Point clamped = polarplot::clamp_to_extent({3.0, -2.0}, /*extent=*/5.0);
    REQUIRE_THAT(clamped.x, WithinAbs(3.0, 1e-12));
    REQUIRE_THAT(clamped.y, WithinAbs(-2.0, 1e-12));
}

TEST_CASE("clamp_to_extent clamps each axis independently to +/- extent",
          "[polar_plot][interaction][clamp]") {
    const polarplot::Point clamped = polarplot::clamp_to_extent({10.0, -10.0}, /*extent=*/5.0);
    REQUIRE_THAT(clamped.x, WithinAbs(5.0, 1e-12));
    REQUIRE_THAT(clamped.y, WithinAbs(-5.0, 1e-12));
}

TEST_CASE("clamp_to_extent treats the boundary as inclusive", "[polar_plot][interaction][clamp]") {
    const polarplot::Point clamped = polarplot::clamp_to_extent({5.0, 5.0}, /*extent=*/5.0);
    REQUIRE_THAT(clamped.x, WithinAbs(5.0, 1e-12));
    REQUIRE_THAT(clamped.y, WithinAbs(5.0, 1e-12));
}

// #49: mouse-leaves-canvas clamp -- a drag keeps tracking the mouse position
// clamped to the plot's pixel-space edge rather than freezing or canceling.
TEST_CASE("clamp_to_rect leaves a point inside the rect unchanged",
          "[polar_plot][interaction][clamp]") {
    constexpr polarplot::PixelRect rect{/*min=*/{0.0, 0.0}, /*max=*/{100.0, 200.0}};
    const polarplot::Point clamped = polarplot::clamp_to_rect({50.0, 150.0}, rect);
    REQUIRE_THAT(clamped.x, WithinAbs(50.0, 1e-12));
    REQUIRE_THAT(clamped.y, WithinAbs(150.0, 1e-12));
}

TEST_CASE("clamp_to_rect clamps a point beyond the rect to its nearest edge",
          "[polar_plot][interaction][clamp]") {
    constexpr polarplot::PixelRect rect{/*min=*/{0.0, 0.0}, /*max=*/{100.0, 200.0}};

    const polarplot::Point beyond_max = polarplot::clamp_to_rect({150.0, 250.0}, rect);
    REQUIRE_THAT(beyond_max.x, WithinAbs(100.0, 1e-12));
    REQUIRE_THAT(beyond_max.y, WithinAbs(200.0, 1e-12));

    const polarplot::Point beyond_min = polarplot::clamp_to_rect({-50.0, -20.0}, rect);
    REQUIRE_THAT(beyond_min.x, WithinAbs(0.0, 1e-12));
    REQUIRE_THAT(beyond_min.y, WithinAbs(0.0, 1e-12));
}

TEST_CASE("clamp_to_rect clamps each axis independently", "[polar_plot][interaction][clamp]") {
    constexpr polarplot::PixelRect rect{/*min=*/{0.0, 0.0}, /*max=*/{100.0, 200.0}};
    const polarplot::Point clamped = polarplot::clamp_to_rect({150.0, 100.0}, rect);
    REQUIRE_THAT(clamped.x, WithinAbs(100.0, 1e-12));
    REQUIRE_THAT(clamped.y, WithinAbs(100.0, 1e-12));
}

// #83: hover_target's pixel-rect containment check, extracted as a pure,
// tested predicate mirroring clamp_to_rect's (Point, PixelRect) argument
// order so the two read as a matched pair.
TEST_CASE("point_in_rect reports a point strictly inside the rect as true",
          "[polar_plot][interaction]") {
    constexpr polarplot::PixelRect rect{/*min=*/{0.0, 0.0}, /*max=*/{100.0, 200.0}};
    CHECK(polarplot::point_in_rect({50.0, 150.0}, rect));
}

TEST_CASE(
    "point_in_rect reports a point strictly outside the rect on each axis independently as "
    "false",
    "[polar_plot][interaction]") {
    constexpr polarplot::PixelRect rect{/*min=*/{0.0, 0.0}, /*max=*/{100.0, 200.0}};

    CHECK_FALSE(polarplot::point_in_rect({-50.0, 100.0}, rect));
    CHECK_FALSE(polarplot::point_in_rect({150.0, 100.0}, rect));
    CHECK_FALSE(polarplot::point_in_rect({50.0, -20.0}, rect));
    CHECK_FALSE(polarplot::point_in_rect({50.0, 250.0}, rect));
}

TEST_CASE(
    "point_in_rect treats each edge as inclusive, matching clamp_to_rect's boundary "
    "convention",
    "[polar_plot][interaction]") {
    constexpr polarplot::PixelRect rect{/*min=*/{0.0, 0.0}, /*max=*/{100.0, 200.0}};

    CHECK(polarplot::point_in_rect({0.0, 100.0}, rect));
    CHECK(polarplot::point_in_rect({100.0, 100.0}, rect));
    CHECK(polarplot::point_in_rect({50.0, 0.0}, rect));
    CHECK(polarplot::point_in_rect({50.0, 200.0}, rect));
}

// #82: resolve_drag_target composes shift-snap then manual-scale clamp, in
// that documented order, for draw_interactive_vector's dragging branch.
TEST_CASE("resolve_drag_target passes the point through unchanged with no snap and auto-scale on",
          "[polar_plot][interaction]") {
    const polarplot::Point resolved = polarplot::resolve_drag_target(
        {3.0, -2.0}, /*shift_snap=*/false, /*auto_scale=*/true, /*visible_extent=*/5.0);
    REQUIRE_THAT(resolved.x, WithinAbs(3.0, 1e-12));
    REQUIRE_THAT(resolved.y, WithinAbs(-2.0, 1e-12));
}

TEST_CASE("resolve_drag_target snaps the angle when shift_snap is set and auto-scale is on",
          "[polar_plot][interaction]") {
    // Mirrors snap_angle_to_increment's own "angle just past a multiple snaps
    // down to it" case: the fixed 15 degree increment (#50), radius untouched.
    constexpr double kIncrement = kPi / 12.0;
    const double radius = 7.0;
    const double angle = (kIncrement * 3.0) + (kIncrement * 0.1);
    const polarplot::Point plotted{radius * std::cos(angle), radius * std::sin(angle)};

    const polarplot::Point resolved = polarplot::resolve_drag_target(
        plotted, /*shift_snap=*/true, /*auto_scale=*/true, /*visible_extent=*/5.0);

    const double expected_angle = kIncrement * 3.0;
    REQUIRE_THAT(resolved.x, WithinAbs(radius * std::cos(expected_angle), 1e-9));
    REQUIRE_THAT(resolved.y, WithinAbs(radius * std::sin(expected_angle), 1e-9));
    REQUIRE_THAT(std::hypot(resolved.x, resolved.y), WithinAbs(radius, 1e-9));
}

TEST_CASE("resolve_drag_target clamps to the extent when shift_snap is unset and auto-scale is off",
          "[polar_plot][interaction]") {
    const polarplot::Point resolved = polarplot::resolve_drag_target(
        {10.0, -10.0}, /*shift_snap=*/false, /*auto_scale=*/false, /*visible_extent=*/5.0);
    REQUIRE_THAT(resolved.x, WithinAbs(5.0, 1e-12));
    REQUIRE_THAT(resolved.y, WithinAbs(-5.0, 1e-12));
}

TEST_CASE("resolve_drag_target snaps before clamping, so a snap that exits the extent is caught",
          "[polar_plot][interaction]") {
    // #49's original motivating case: a point within the extent on both axes
    // pre-snap can still land outside it once snapped, because snapping
    // rotates the point (preserving radius) rather than moving it inward.
    // extent=5, radius=8, angle=5 degrees (closer to the 0 degree multiple
    // than to 15): snapping first pins the angle to 0 (giving (8, 0)), then
    // the manual-scale clamp catches the now out-of-extent x, landing on
    // (5, 0) -- not the different point a clamp-then-snap order would give.
    constexpr double kIncrement = kPi / 12.0;
    const double radius = 8.0;
    const double angle = kIncrement * (5.0 / 15.0);  // 5 degrees.
    const polarplot::Point plotted{radius * std::cos(angle), radius * std::sin(angle)};

    const polarplot::Point resolved = polarplot::resolve_drag_target(
        plotted, /*shift_snap=*/true, /*auto_scale=*/false, /*visible_extent=*/5.0);

    REQUIRE_THAT(resolved.x, WithinAbs(5.0, 1e-9));
    REQUIRE_THAT(resolved.y, WithinAbs(0.0, 1e-9));
}

TEST_CASE("resolve_drag_target treats the extent boundary as inclusive",
          "[polar_plot][interaction]") {
    const polarplot::Point resolved = polarplot::resolve_drag_target(
        {5.0, 5.0}, /*shift_snap=*/false, /*auto_scale=*/false, /*visible_extent=*/5.0);
    REQUIRE_THAT(resolved.x, WithinAbs(5.0, 1e-12));
    REQUIRE_THAT(resolved.y, WithinAbs(5.0, 1e-12));
}
