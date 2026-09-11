#include "ui/plot_plan.hpp"

#include <numbers>

#include "ui/construction.hpp"

namespace ui {
namespace {

constexpr double kDegToRad = std::numbers::pi / 180.0;

polarplot::Point to_point(vecmath::Vec2 v) { return {v.x, v.y}; }

polarplot::AnnotationVector to_annotation(const ConstructionVector& construction) {
    return {to_point(construction.start), to_point(construction.vector)};
}

}  // namespace

PlotPlan plan_plot(const PlotInputs& inputs) {
    const vecmath::Vec2 sum = inputs.a + inputs.b;
    const vecmath::Vec2 diff = inputs.a - inputs.b;

    PlotPlan plan;
    plan.a = to_point(inputs.a);
    plan.b = to_point(inputs.b);
    plan.convention = {
        .zero_direction = inputs.zero_direction_deg * kDegToRad,
        .angle_sign = compose_angle_sign(inputs.rotation_direction, inputs.measurement_convention),
    };

    // Push order below is part of PlotPlan's contract: when both are present,
    // the difference's tip-to-tail annotation always precedes the sum's two,
    // so a caller drawing tip_to_tail_annotations by index (e.g. an
    // "tip_to_tail_<i>" id) gets a stable, predictable id per entry.
    if (inputs.show_difference) {
        plan.difference = to_point(diff);
        if (inputs.show_tip_to_tail) {
            plan.tip_to_tail_annotations.push_back(
                to_annotation(tip_to_tail_difference(inputs.a, inputs.b)));
        }
        if (inputs.show_difference_segment) {
            plan.difference_segment = to_annotation(difference_segment(inputs.a, inputs.b));
        }
    }

    if (inputs.show_sum) {
        plan.sum = to_point(sum);
        if (inputs.show_tip_to_tail) {
            const SumConstruction construction = tip_to_tail_sum(inputs.a, inputs.b);
            plan.tip_to_tail_annotations.push_back(to_annotation(construction.b_from_a_tip));
            plan.tip_to_tail_annotations.push_back(to_annotation(construction.a_from_b_tip));
        }
    }

    if (inputs.zero_direction_input_focused || inputs.show_zero_direction_arc_persistent) {
        plan.zero_direction_arc_angle = plan.convention.zero_direction;
    }

    return plan;
}

}  // namespace ui
