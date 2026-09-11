#include <catch2/catch_test_macros.hpp>

#include "polar_plotting/polar_plot.hpp"
#include "ui/plot_plan.hpp"
#include "vector_math/vec2.hpp"

using ui::PlotInputs;
using ui::PlotPlan;
using vecmath::Vec2;

namespace {

bool points_equal(polarplot::Point p, polarplot::Point q) { return p.x == q.x && p.y == q.y; }

bool annotations_equal(const polarplot::AnnotationVector& u, const polarplot::AnnotationVector& v) {
    return points_equal(u.start, v.start) && points_equal(u.vector, v.vector);
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
        REQUIRE(plan.sum.has_value());
        CHECK(points_equal(*plan.sum, {(kA + kB).x, (kA + kB).y}));
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
        REQUIRE(plan.difference.has_value());
        CHECK(points_equal(*plan.difference, {(kA - kB).x, (kA - kB).y}));
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
        REQUIRE(plan.difference_segment.has_value());
        CHECK(annotations_equal(*plan.difference_segment,
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
        REQUIRE(plan.zero_direction_arc_angle.has_value());
        CHECK(*plan.zero_direction_arc_angle == plan.convention.zero_direction);
    }

    SECTION("persistent only: arc shown") {
        const PlotPlan plan = ui::plan_plot(PlotInputs{.a = kA,
                                                       .b = kB,
                                                       .zero_direction_deg = kZeroDirectionDeg,
                                                       .zero_direction_input_focused = false,
                                                       .show_zero_direction_arc_persistent = true});
        REQUIRE(plan.zero_direction_arc_angle.has_value());
        CHECK(*plan.zero_direction_arc_angle == plan.convention.zero_direction);
    }

    SECTION("both focused and persistent: arc still shown (OR, not exclusive)") {
        const PlotPlan plan = ui::plan_plot(PlotInputs{.a = kA,
                                                       .b = kB,
                                                       .zero_direction_deg = kZeroDirectionDeg,
                                                       .zero_direction_input_focused = true,
                                                       .show_zero_direction_arc_persistent = true});
        REQUIRE(plan.zero_direction_arc_angle.has_value());
        CHECK(*plan.zero_direction_arc_angle == plan.convention.zero_direction);
    }
}
