#include <optional>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "polar_plotting/polar_plot.hpp"
#include "ui/plot_plan.hpp"
#include "vector_math/vec2.hpp"

using Catch::Matchers::WithinRel;
using polarplot::PlotFrame;
using ui::PlotInputs;
using ui::PlotPlan;
using vecmath::Vec2;

namespace {

bool points_equal(polarplot::Point p, polarplot::Point q) { return p.x == q.x && p.y == q.y; }

bool annotations_equal(const polarplot::AnnotationVector& u, const polarplot::AnnotationVector& v) {
    return points_equal(u.start, v.start) && points_equal(u.vector, v.vector);
}

// Fails the current test and returns a default-constructed T if `opt` is
// empty, otherwise returns its value. A real `if` (rather than
// REQUIRE(opt.has_value()) followed by a separate dereference) so the
// checked-ness is visible to static analysis at the point of access, and a
// standalone function so branching here doesn't add to a TEST_CASE's own
// cognitive complexity.
template <typename T>
T require_value(const std::optional<T>& opt) {
    if (opt) {
        return *opt;
    }
    FAIL("expected optional value to be set");
    return T{};
}

constexpr Vec2 kA{3.0, 1.0};
constexpr Vec2 kB{-1.0, 2.0};

}  // namespace

TEST_CASE("plan_plot always populates a, b, and the composed convention", "[plot_plan]") {
    const PlotInputs inputs{.a = kA, .b = kB};

    const PlotPlan plan = ui::plan_plot(inputs);

    CHECK(points_equal(plan.a, {kA.x, kA.y}));
    CHECK(points_equal(plan.b, {kB.x, kB.y}));
    CHECK(plan.convention.zero_direction == 0.0);
    CHECK(plan.convention.angle_sign == 1.0);
    CHECK_FALSE(plan.sum.has_value());
    CHECK_FALSE(plan.difference.has_value());
    CHECK(plan.tip_to_tail_annotations.empty());
    CHECK_FALSE(plan.difference_segment.has_value());
    CHECK_FALSE(plan.zero_direction_arc_angle.has_value());
    CHECK_FALSE(plan.difference_ba.has_value());
    CHECK_FALSE(plan.product.has_value());
    CHECK_FALSE(plan.quotient_ab.has_value());
    CHECK_FALSE(plan.quotient_ba.has_value());
}

TEST_CASE("plan_plot populates the plain-arrow derived vectors only when toggled on",
          "[plot_plan]") {
    SECTION("all four off: none present") {
        const PlotPlan plan = ui::plan_plot(PlotInputs{.a = kA, .b = kB});
        CHECK_FALSE(plan.difference_ba.has_value());
        CHECK_FALSE(plan.product.has_value());
        CHECK_FALSE(plan.quotient_ab.has_value());
        CHECK_FALSE(plan.quotient_ba.has_value());
    }

    SECTION("show_difference_ba on: B - A present") {
        const PlotPlan plan =
            ui::plan_plot(PlotInputs{.a = kA, .b = kB, .show_difference_ba = true});
        const Vec2 expected = kB - kA;
        CHECK(points_equal(require_value(plan.difference_ba), {expected.x, expected.y}));
    }

    SECTION("show_product on: A x B present") {
        const PlotPlan plan = ui::plan_plot(PlotInputs{.a = kA, .b = kB, .show_product = true});
        const Vec2 expected = vecmath::complex_multiply(kA, kB);
        CHECK(points_equal(require_value(plan.product), {expected.x, expected.y}));
    }

    SECTION("show_quotient_ab on: A / B present") {
        const PlotPlan plan = ui::plan_plot(PlotInputs{.a = kA, .b = kB, .show_quotient_ab = true});
        const Vec2 expected = *vecmath::complex_divide(kA, kB);
        CHECK(points_equal(require_value(plan.quotient_ab), {expected.x, expected.y}));
    }

    SECTION("show_quotient_ba on: B / A present") {
        const PlotPlan plan = ui::plan_plot(PlotInputs{.a = kA, .b = kB, .show_quotient_ba = true});
        const Vec2 expected = *vecmath::complex_divide(kB, kA);
        CHECK(points_equal(require_value(plan.quotient_ba), {expected.x, expected.y}));
    }

    SECTION("show_quotient_ab on but B is zero: quotient stays absent despite the toggle") {
        const PlotPlan plan =
            ui::plan_plot(PlotInputs{.a = kA, .b = Vec2{0.0, 0.0}, .show_quotient_ab = true});
        CHECK_FALSE(plan.quotient_ab.has_value());
    }
}

TEST_CASE("sum shown/hidden x tip-to-tail on/off", "[plot_plan]") {
    SECTION("sum hidden: no sum, no tip-to-tail annotations from the sum side") {
        const PlotPlan plan = ui::plan_plot(
            PlotInputs{.a = kA, .b = kB, .show_sum = false, .show_tip_to_tail = true});
        CHECK_FALSE(plan.sum.has_value());
        CHECK(plan.tip_to_tail_annotations.empty());
    }

    SECTION("sum shown, tip-to-tail off: sum present, no annotations") {
        const PlotPlan plan = ui::plan_plot(
            PlotInputs{.a = kA, .b = kB, .show_sum = true, .show_tip_to_tail = false});
        CHECK(points_equal(require_value(plan.sum), {(kA + kB).x, (kA + kB).y}));
        CHECK(plan.tip_to_tail_annotations.empty());
    }

    SECTION("sum shown, tip-to-tail on: sum present, both parallelogram paths included") {
        const PlotPlan plan =
            ui::plan_plot(PlotInputs{.a = kA, .b = kB, .show_sum = true, .show_tip_to_tail = true});
        REQUIRE(plan.sum.has_value());
        REQUIRE(plan.tip_to_tail_annotations.size() == 2);
        CHECK(annotations_equal(plan.tip_to_tail_annotations[0], {{kA.x, kA.y}, {kB.x, kB.y}}));
        CHECK(annotations_equal(plan.tip_to_tail_annotations[1], {{kB.x, kB.y}, {kA.x, kA.y}}));
    }
}

TEST_CASE("difference shown/hidden x tip-to-tail on/off x difference-segment on/off",
          "[plot_plan]") {
    SECTION("difference hidden: no difference, no annotations regardless of the other toggles") {
        const PlotPlan plan = ui::plan_plot(PlotInputs{.a = kA,
                                                       .b = kB,
                                                       .show_difference = false,
                                                       .show_tip_to_tail = true,
                                                       .show_difference_segment = true});
        CHECK_FALSE(plan.difference.has_value());
        CHECK(plan.tip_to_tail_annotations.empty());
        CHECK_FALSE(plan.difference_segment.has_value());
    }

    SECTION("difference shown, both overlays off: difference present, no annotations") {
        const PlotPlan plan = ui::plan_plot(PlotInputs{.a = kA,
                                                       .b = kB,
                                                       .show_difference = true,
                                                       .show_tip_to_tail = false,
                                                       .show_difference_segment = false});
        CHECK(points_equal(require_value(plan.difference), {(kA - kB).x, (kA - kB).y}));
        CHECK(plan.tip_to_tail_annotations.empty());
        CHECK_FALSE(plan.difference_segment.has_value());
    }

    SECTION("difference shown, tip-to-tail on only: one annotation, -b from a's tip") {
        const PlotPlan plan = ui::plan_plot(PlotInputs{.a = kA,
                                                       .b = kB,
                                                       .show_difference = true,
                                                       .show_tip_to_tail = true,
                                                       .show_difference_segment = false});
        REQUIRE(plan.tip_to_tail_annotations.size() == 1);
        CHECK(annotations_equal(plan.tip_to_tail_annotations[0], {{kA.x, kA.y}, {-kB.x, -kB.y}}));
        CHECK_FALSE(plan.difference_segment.has_value());
    }

    SECTION("difference shown, difference-segment on only: segment from b's tip to a's tip") {
        const PlotPlan plan = ui::plan_plot(PlotInputs{.a = kA,
                                                       .b = kB,
                                                       .show_difference = true,
                                                       .show_tip_to_tail = false,
                                                       .show_difference_segment = true});
        CHECK(plan.tip_to_tail_annotations.empty());
        CHECK(annotations_equal(require_value(plan.difference_segment),
                                {{kB.x, kB.y}, {(kA - kB).x, (kA - kB).y}}));
    }

    SECTION("difference shown, both overlays on: both present together") {
        const PlotPlan plan = ui::plan_plot(PlotInputs{.a = kA,
                                                       .b = kB,
                                                       .show_difference = true,
                                                       .show_tip_to_tail = true,
                                                       .show_difference_segment = true});
        REQUIRE(plan.tip_to_tail_annotations.size() == 1);
        REQUIRE(plan.difference_segment.has_value());
    }
}

TEST_CASE("zero-direction arc: transient focus x persistent toggle", "[plot_plan]") {
    constexpr double kZeroDirectionDeg = 45.0;

    SECTION("neither focused nor persistent: no arc") {
        const PlotPlan plan =
            ui::plan_plot(PlotInputs{.a = kA,
                                     .b = kB,
                                     .zero_direction_deg = kZeroDirectionDeg,
                                     .zero_direction_input_focused = false,
                                     .show_zero_direction_arc_persistent = false});
        CHECK_FALSE(plan.zero_direction_arc_angle.has_value());
    }

    SECTION("focused only: arc at the composed zero direction") {
        const PlotPlan plan =
            ui::plan_plot(PlotInputs{.a = kA,
                                     .b = kB,
                                     .zero_direction_deg = kZeroDirectionDeg,
                                     .zero_direction_input_focused = true,
                                     .show_zero_direction_arc_persistent = false});
        CHECK(require_value(plan.zero_direction_arc_angle) == plan.convention.zero_direction);
    }

    SECTION("persistent only: arc shown") {
        const PlotPlan plan = ui::plan_plot(PlotInputs{.a = kA,
                                                       .b = kB,
                                                       .zero_direction_deg = kZeroDirectionDeg,
                                                       .zero_direction_input_focused = false,
                                                       .show_zero_direction_arc_persistent = true});
        CHECK(require_value(plan.zero_direction_arc_angle) == plan.convention.zero_direction);
    }

    SECTION("both focused and persistent: arc still shown (OR, not exclusive)") {
        const PlotPlan plan = ui::plan_plot(PlotInputs{.a = kA,
                                                       .b = kB,
                                                       .zero_direction_deg = kZeroDirectionDeg,
                                                       .zero_direction_input_focused = true,
                                                       .show_zero_direction_arc_persistent = true});
        CHECK(require_value(plan.zero_direction_arc_angle) == plan.convention.zero_direction);
    }
}

TEST_CASE("auto_fit_extent only folds in magnitudes of vectors currently shown", "[plot_plan]") {
    SECTION("sum and difference off: extent driven by A and B alone") {
        const PlotFrame extent = ui::auto_fit_extent(PlotInputs{
            .a = {3.0, 0.0}, .b = {0.0, 0.0}, .show_sum = false, .show_difference = false});
        // max magnitude 3, margin 1.2 => target 3.6, desired interval 0.9 =>
        // snaps up to 1.0 (next of the 1/2/5 sequence).
        CHECK_THAT(extent.ring_interval(), WithinRel(1.0));
        CHECK_THAT(extent.extent(), WithinRel(4.0));
    }

    SECTION(
        "a huge B is ignored by the extent when show flags leave it out of view (still "
        "counted -- A and B are always shown)") {
        // A and B are always drawn, so both always count -- this documents
        // that fact rather than an opt-out.
        const PlotFrame extent =
            ui::auto_fit_extent(PlotInputs{.a = {1.0, 0.0}, .b = {8.0, 0.0}, .show_sum = false});
        // max magnitude 8, target 9.6, desired interval 2.4 => snaps to 5.
        CHECK_THAT(extent.ring_interval(), WithinRel(5.0));
        CHECK_THAT(extent.extent(), WithinRel(20.0));
    }

    SECTION("sum shown grows the extent to cover it when it's the largest vector") {
        const PlotFrame extent = ui::auto_fit_extent(PlotInputs{
            .a = {3.0, 0.0}, .b = {3.0, 0.0}, .show_sum = true, .show_difference = false});
        // sum = (6, 0), magnitude 6, target 7.2, desired 1.8 => snaps to 2.
        CHECK_THAT(extent.ring_interval(), WithinRel(2.0));
        CHECK_THAT(extent.extent(), WithinRel(8.0));
    }

    SECTION("sum hidden: its magnitude does not affect the extent even though it's the largest") {
        const PlotFrame extent = ui::auto_fit_extent(PlotInputs{
            .a = {3.0, 0.0}, .b = {3.0, 0.0}, .show_sum = false, .show_difference = false});
        // Ignoring the (hidden) sum's magnitude of 6, only A/B (magnitude 3)
        // count: target 3.6, desired 0.9 => snaps to 1.
        CHECK_THAT(extent.ring_interval(), WithinRel(1.0));
        CHECK_THAT(extent.extent(), WithinRel(4.0));
    }

    SECTION("difference shown grows the extent to cover it when it's the largest vector") {
        const PlotFrame extent = ui::auto_fit_extent(PlotInputs{
            .a = {5.0, 0.0}, .b = {-5.0, 0.0}, .show_sum = false, .show_difference = true});
        // difference = (10, 0), magnitude 10, target 12, desired 3 => snaps to 5.
        CHECK_THAT(extent.ring_interval(), WithinRel(5.0));
        CHECK_THAT(extent.extent(), WithinRel(20.0));
    }
}

TEST_CASE("auto_fit_extent edge cases: zero, very small, and very large vectors", "[plot_plan]") {
    SECTION("zero vectors (all inputs at the origin) fall back to a non-degenerate default") {
        const PlotFrame extent = ui::auto_fit_extent(PlotInputs{.a = {0.0, 0.0}, .b = {0.0, 0.0}});
        CHECK(extent.ring_interval() > 0.0);
        CHECK_THAT(extent.extent(), WithinRel(extent.ring_interval() * 4.0));
    }

    SECTION("a very small vector still snaps to a small, non-zero nice interval") {
        const PlotFrame extent =
            ui::auto_fit_extent(PlotInputs{.a = {0.003, 0.0}, .b = {0.0, 0.0}});
        // max magnitude 0.003, target 0.0036, desired 0.0009 => snaps to 0.001.
        CHECK_THAT(extent.ring_interval(), WithinRel(0.001));
        CHECK_THAT(extent.extent(), WithinRel(0.004));
    }

    SECTION("a very large vector snaps to a large nice interval, never zero or degenerate") {
        const PlotFrame extent =
            ui::auto_fit_extent(PlotInputs{.a = {1234.0, 0.0}, .b = {0.0, 0.0}});
        // max magnitude 1234, target 1480.8, desired 370.2 => snaps to 500.
        CHECK_THAT(extent.ring_interval(), WithinRel(500.0));
        CHECK_THAT(extent.extent(), WithinRel(2000.0));
    }
}

TEST_CASE("auto_fit_extent's extent is always exactly ring_interval * kAutoFitRings",
          "[plot_plan]") {
    for (const double magnitude : {0.0, 0.07, 1.0, 3.6, 42.0, 9999.0}) {
        const PlotFrame extent =
            ui::auto_fit_extent(PlotInputs{.a = {magnitude, 0.0}, .b = {0.0, 0.0}});
        CHECK_THAT(extent.extent(), WithinRel(extent.ring_interval() * 4.0));
    }
}

TEST_CASE("auto_fit_extent: nearby magnitudes snapping to the same interval don't jitter",
          "[plot_plan]") {
    const PlotFrame low = ui::auto_fit_extent(PlotInputs{.a = {2.5, 0.0}, .b = {0.0, 0.0}});
    const PlotFrame high = ui::auto_fit_extent(PlotInputs{.a = {2.6, 0.0}, .b = {0.0, 0.0}});
    CHECK_THAT(low.ring_interval(), WithinRel(high.ring_interval()));
    CHECK_THAT(low.extent(), WithinRel(high.extent()));
}

TEST_CASE("plan_plot's extent field matches auto_fit_extent for the same inputs", "[plot_plan]") {
    const PlotInputs inputs{.a = kA, .b = kB, .show_sum = true, .show_difference = true};
    const PlotPlan plan = ui::plan_plot(inputs);
    const PlotFrame expected = ui::auto_fit_extent(inputs);

    CHECK_THAT(plan.extent.ring_interval(), WithinRel(expected.ring_interval()));
    CHECK_THAT(plan.extent.extent(), WithinRel(expected.extent()));
}

TEST_CASE("manual scale override: manual interval replaces the auto-fit calculation",
          "[plot_plan]") {
    // A's magnitude (3) would auto-fit to ring_interval 1.0 (see the
    // auto_fit_extent test above with the same A) -- picking a very
    // different manual interval demonstrates it's not being used.
    const PlotInputs inputs{.a = kA, .b = kB, .auto_scale = false, .manual_ring_interval = 7.5};

    const PlotPlan plan = ui::plan_plot(inputs);

    CHECK_THAT(plan.extent.ring_interval(), WithinRel(7.5));
}

TEST_CASE("manual scale override: extent == interval * kAutoFitRings still holds in manual mode",
          "[plot_plan]") {
    for (const double interval : {0.1, 1.0, 3.0, 42.0, 100.0}) {
        const PlotInputs inputs{
            .a = kA, .b = kB, .auto_scale = false, .manual_ring_interval = interval};
        const PlotPlan plan = ui::plan_plot(inputs);
        CHECK_THAT(plan.extent.ring_interval(), WithinRel(interval));
        CHECK_THAT(plan.extent.extent(), WithinRel(interval * ui::kAutoFitRings));
    }
}

TEST_CASE(
    "manual scale override: auto_scale on (default) ignores manual_ring_interval and still "
    "auto-fits",
    "[plot_plan]") {
    const PlotInputs inputs{.a = kA, .b = kB, .manual_ring_interval = 999.0};
    const PlotPlan plan = ui::plan_plot(inputs);
    const PlotFrame expected = ui::auto_fit_extent(inputs);
    CHECK_THAT(plan.extent.ring_interval(), WithinRel(expected.ring_interval()));
}

TEST_CASE("rotation indicator: plan_plot derives a plain sweep sign from rotation_direction alone",
          "[plot_plan]") {
    // The derived value must be a plain double -- never ui::RotationDirection
    // or ui::MeasurementConvention (see
    // docs/adr/0001-polar-plotting-receives-only-composed-angle-sign.md) --
    // comparing it against a double literal below exercises that.
    SECTION("counterclockwise composes to a positive sweep sign") {
        const PlotPlan plan = ui::plan_plot(PlotInputs{
            .a = kA, .b = kB, .rotation_direction = ui::RotationDirection::CounterClockwise});
        CHECK(plan.rotation_indicator_sweep_sign == 1.0);
    }

    SECTION("clockwise composes to a negative sweep sign") {
        const PlotPlan plan = ui::plan_plot(
            PlotInputs{.a = kA, .b = kB, .rotation_direction = ui::RotationDirection::Clockwise});
        CHECK(plan.rotation_indicator_sweep_sign == -1.0);
    }

    SECTION("measurement_convention does not affect the sweep sign (unlike angle_sign)") {
        const PlotPlan with_rotation = ui::plan_plot(
            PlotInputs{.a = kA,
                       .b = kB,
                       .rotation_direction = ui::RotationDirection::CounterClockwise,
                       .measurement_convention = ui::MeasurementConvention::WithRotation});
        const PlotPlan against_rotation = ui::plan_plot(
            PlotInputs{.a = kA,
                       .b = kB,
                       .rotation_direction = ui::RotationDirection::CounterClockwise,
                       .measurement_convention = ui::MeasurementConvention::AgainstRotation});
        CHECK(with_rotation.rotation_indicator_sweep_sign == 1.0);
        CHECK(against_rotation.rotation_indicator_sweep_sign == 1.0);
    }
}

TEST_CASE("manual_extent uses the given interval verbatim and keeps the extent relationship",
          "[plot_plan]") {
    for (const double interval : {0.1, 1.0, 3.0, 42.0, 100.0}) {
        const PlotFrame extent = ui::manual_extent(interval);
        CHECK_THAT(extent.ring_interval(), WithinRel(interval));
        CHECK_THAT(extent.extent(), WithinRel(interval * ui::kAutoFitRings));
    }
}
