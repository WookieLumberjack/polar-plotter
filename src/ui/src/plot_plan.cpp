#include "ui/plot_plan.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

#include "ui/construction.hpp"
#include "vector_math/vec2.hpp"

namespace ui {
namespace {

constexpr double kDegToRad = std::numbers::pi / 180.0;

// Padding applied to the largest shown vector's magnitude before snapping up
// to a nice ring interval, so vector tips don't sit exactly on the outer
// ring. Kept small: the outer ring itself is rarely reached in practice
// (the largest vector often lands mid-plot instead) mostly because of gaps
// between successive "nice" ring intervals (e.g. 1 -> 2 is already a 100%
// jump), not this margin -- a bigger margin only makes that worse, so this
// stays just large enough to keep the tip visibly clear of the ring.
constexpr double kAutoFitMargin = 1.05;

// Fallback ring interval used when every shown vector is (numerically) at
// the origin, so the view never collapses to a zero/degenerate extent.
constexpr double kAutoFitDefaultInterval = 1.0;

polarplot::Point to_point(vecmath::Vec2 v) { return {v.x, v.y}; }

polarplot::AnnotationVector to_annotation(const ConstructionVector& construction) {
    return {to_point(construction.start), to_point(construction.vector)};
}

// Smallest value from the fixed {1, 2, 5} x 10^k "nice number" sequence that
// is >= `value`. `value` must be strictly positive -- callers guard the
// all-origin (value <= 0) case separately with kAutoFitDefaultInterval.
double snap_up_to_nice_step(double value) {
    const double exponent = std::floor(std::log10(value));
    const double decade = std::pow(10.0, exponent);
    // 10x the decade is included so the search always finds a candidate
    // within this decade -- `value < 10 * decade` follows from `exponent`
    // being the floor of value's base-10 log.
    constexpr std::array<double, 4> kSteps{1.0, 2.0, 5.0, 10.0};
    for (const double step : kSteps) {
        const double candidate = step * decade;
        // A tiny relative slack absorbs floating-point round-off from the
        // log10/pow round trip (e.g. value landing a hair above a candidate
        // that should exactly equal it).
        if (candidate >= value * (1.0 - 1e-9)) {
            return candidate;
        }
    }
    // Unreachable: kSteps ends at 10x the decade, which always satisfies the
    // loop's condition per the comment above.
    return kSteps.back() * decade;
}

// Push order matches App::draw_plot's draw/legend order (see
// PlotPlan::derived_vectors' contract comment), not plan_plot's computation
// order for the six named optional fields above -- purely a list-shaped view
// over values plan_plot has already computed, no new computation. Factored
// out of plan_plot to keep that function's cognitive complexity down.
std::vector<PlotPlan::DerivedVector> collect_derived_vectors(const PlotPlan& plan) {
    std::vector<PlotPlan::DerivedVector> derived_vectors;
    if (plan.difference) {
        derived_vectors.push_back({"A - B", *plan.difference});
    }
    if (plan.sum) {
        derived_vectors.push_back({"A + B", *plan.sum});
    }
    if (plan.difference_ba) {
        derived_vectors.push_back({"B - A", *plan.difference_ba});
    }
    if (plan.product) {
        derived_vectors.push_back({"A x B", *plan.product});
    }
    if (plan.quotient_ab) {
        derived_vectors.push_back({"A / B", *plan.quotient_ab});
    }
    if (plan.quotient_ba) {
        derived_vectors.push_back({"B / A", *plan.quotient_ba});
    }
    return derived_vectors;
}

}  // namespace

polarplot::PlotFrame auto_fit_extent(const PlotInputs& inputs) {
    double max_magnitude = std::max(vecmath::magnitude(inputs.a), vecmath::magnitude(inputs.b));
    if (inputs.show_sum) {
        max_magnitude = std::max(max_magnitude, vecmath::magnitude(inputs.a + inputs.b));
    }
    if (inputs.show_difference) {
        max_magnitude = std::max(max_magnitude, vecmath::magnitude(inputs.a - inputs.b));
    }
    if (inputs.show_difference_ba) {
        max_magnitude = std::max(max_magnitude, vecmath::magnitude(inputs.b - inputs.a));
    }
    if (inputs.show_product) {
        max_magnitude = std::max(max_magnitude,
                                 vecmath::magnitude(vecmath::complex_multiply(inputs.a, inputs.b)));
    }
    if (inputs.show_quotient_ab) {
        if (const auto quotient = vecmath::complex_divide(inputs.a, inputs.b)) {
            max_magnitude = std::max(max_magnitude, vecmath::magnitude(*quotient));
        }
    }
    if (inputs.show_quotient_ba) {
        if (const auto quotient = vecmath::complex_divide(inputs.b, inputs.a)) {
            max_magnitude = std::max(max_magnitude, vecmath::magnitude(*quotient));
        }
    }

    if (max_magnitude <= 0.0) {
        return {kAutoFitDefaultInterval, kAutoFitRings};
    }

    const double desired_interval = (max_magnitude * kAutoFitMargin) / kAutoFitRings;
    const double ring_interval = snap_up_to_nice_step(desired_interval);
    return {ring_interval, kAutoFitRings};
}

polarplot::PlotFrame manual_extent(double ring_interval) { return {ring_interval, kAutoFitRings}; }

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
    plan.rotation_indicator_sweep_sign = rotation_sweep_sign(inputs.rotation_direction);
    plan.extent =
        inputs.auto_scale ? auto_fit_extent(inputs) : manual_extent(inputs.manual_ring_interval);

    // Push order below is part of PlotPlan's contract: when present, the
    // entries appear in this fixed order regardless of which toggles
    // produced them -- A - B's tip-to-tail annotation, then B - A's, then the
    // sum's two -- so a caller drawing tip_to_tail_annotations by index (e.g.
    // an "tip_to_tail_<i>" id) gets a stable, predictable id per entry.
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

    if (inputs.show_difference_ba) {
        plan.difference_ba = to_point(inputs.b - inputs.a);
        if (inputs.show_tip_to_tail) {
            plan.tip_to_tail_annotations.push_back(
                to_annotation(tip_to_tail_difference_ba(inputs.a, inputs.b)));
        }
        if (inputs.show_difference_segment) {
            plan.difference_segment_ba = to_annotation(difference_segment_ba(inputs.a, inputs.b));
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
    if (inputs.show_product) {
        plan.product = to_point(vecmath::complex_multiply(inputs.a, inputs.b));
    }
    if (inputs.show_quotient_ab) {
        if (const auto quotient = vecmath::complex_divide(inputs.a, inputs.b)) {
            plan.quotient_ab = to_point(*quotient);
        }
    }
    if (inputs.show_quotient_ba) {
        if (const auto quotient = vecmath::complex_divide(inputs.b, inputs.a)) {
            plan.quotient_ba = to_point(*quotient);
        }
    }

    if (inputs.zero_direction_input_focused || inputs.show_zero_direction_arc_persistent) {
        plan.zero_direction_arc_angle = plan.convention.zero_direction;
    }

    plan.derived_vectors = collect_derived_vectors(plan);

    return plan;
}

}  // namespace ui
