#ifndef UI_PLOT_PLAN_HPP
#define UI_PLOT_PLAN_HPP

/// \file
/// Decides *what* App::draw_plot() should draw this frame, separately from
/// *how* it's drawn. Pure -- no ImGui/ImPlot dependency -- so the decision
/// logic (which toggles produce which annotations, the zero-direction arc's
/// transient-vs-persistent OR) is unit-testable without booting the app.
/// Sits one level above ui::construction's pure geometry: construction
/// computes *where* a construction vector goes; plan_plot decides *whether*
/// to include it.

#include <optional>
#include <vector>

#include "polar_plotting/polar_plot.hpp"
#include "ui/angle_convention.hpp"
#include "vector_math/vec2.hpp"

namespace ui {

/// Everything App::draw_plot() currently reads off its own members to decide
/// what to draw this frame, bundled into one value. Assembled fresh each
/// frame in App::draw_plot(); App's own member layout is unchanged.
struct PlotInputs {
    vecmath::Vec2 a;
    vecmath::Vec2 b;
    bool show_sum{false};
    bool show_difference{false};
    bool show_tip_to_tail{false};
    bool show_difference_segment{false};
    polarplot::TipMarkerStyle marker_style{polarplot::TipMarkerStyle::kDot};
    double zero_direction_deg{0.0};
    RotationDirection rotation_direction{RotationDirection::CounterClockwise};
    MeasurementConvention measurement_convention{MeasurementConvention::WithRotation};
    // Raw zero-direction-arc lifecycle bools, combined by plan_plot (see
    // PlotPlan::zero_direction_arc_angle) rather than pre-combined by the
    // caller.
    bool zero_direction_input_focused{false};
    bool show_zero_direction_arc_persistent{false};
};

/// Number of concentric rings draw_polar_grid lays the auto-fit view out
/// over. `PlotExtent::extent` and `::ring_interval` are always related by
/// `extent == ring_interval * kAutoFitRings`.
inline constexpr int kAutoFitRings = 4;

/// The auto-fit view geometry for one frame: `extent` is the plot's overall
/// radius (feed it to begin_vector_plot's axis limits), `ring_interval` is
/// the spacing between draw_polar_grid's rings. Always related by
/// `extent == ring_interval * kAutoFitRings` -- see auto_fit_extent.
struct PlotExtent {
    double ring_interval{1.0};
    double extent{static_cast<double>(kAutoFitRings)};
};

/// Computes this frame's auto-fit extent/ring-interval from the magnitudes of
/// the vectors currently shown -- \p inputs' A and B always, plus the sum
/// and/or difference when their respective show_sum/show_difference toggle
/// is on. Pads the largest magnitude by a margin, then snaps the result up to
/// the smallest "nice" ring interval (a value from the 1/2/5 x 10^k step
/// sequence) such that kAutoFitRings rings comfortably contain it, so the
/// view stays visually stable (no jitter) as inputs change continuously.
/// Degenerate inputs (e.g. all vectors at the origin) fall back to a default,
/// non-zero interval rather than collapsing the view. Pure -- no ImGui/ImPlot
/// dependency.
[[nodiscard]] PlotExtent auto_fit_extent(const PlotInputs& inputs);

/// A closed description of everything on-plot this frame. draw_plot() loops
/// over this and issues the matching polarplot:: draw calls -- no toggle
/// logic of its own left over.
struct PlotPlan {
    polarplot::Point a;
    polarplot::Point b;
    std::optional<polarplot::Point> sum;
    std::optional<polarplot::Point> difference;
    // 0, 1 (difference's tip-to-tail only), or 2 (sum's parallelogram)
    // entries, depending on which toggles are on.
    std::vector<polarplot::AnnotationVector> tip_to_tail_annotations;
    std::optional<polarplot::AnnotationVector> difference_segment;
    // Present iff the arc should be drawn this frame (transient-focused OR
    // persistent-toggle), holding the angle to draw it at.
    std::optional<double> zero_direction_arc_angle;
    // Composed once from PlotInputs' angle-convention fields, needed by the
    // caller to draw everything above.
    polarplot::AngleConvention convention;
    // See auto_fit_extent -- the same values must drive both draw_polar_grid
    // and the plot's axis limits so they never disagree.
    PlotExtent extent;
};

/// Decide this frame's PlotPlan from \p inputs. Pure -- internally calls into
/// ui::construction's tip_to_tail_sum/tip_to_tail_difference/
/// difference_segment exactly as App::draw_plot() did before this seam
/// existed, and composes the angle sign via ui::angle_convention.
[[nodiscard]] PlotPlan plan_plot(const PlotInputs& inputs);

}  // namespace ui

#endif  // UI_PLOT_PLAN_HPP
