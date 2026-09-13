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
    // Plain-arrow derived vectors -- no construction sub-toggles, unlike
    // sum/difference above.
    bool show_difference_ba{false};
    bool show_product{false};
    bool show_quotient_ab{false};
    bool show_quotient_ba{false};
    polarplot::TipMarkerStyle marker_style{polarplot::TipMarkerStyle::kDot};
    double zero_direction_deg{0.0};
    RotationDirection rotation_direction{RotationDirection::CounterClockwise};
    MeasurementConvention measurement_convention{MeasurementConvention::WithRotation};
    // Raw zero-direction-arc lifecycle bools, combined by plan_plot (see
    // PlotPlan::zero_direction_arc_angle) rather than pre-combined by the
    // caller.
    bool zero_direction_input_focused{false};
    bool show_zero_direction_arc_persistent{false};
    // When true (the default), plan_plot derives polarplot::PlotFrame from
    // auto_fit_extent; when false, it uses manual_ring_interval verbatim (see
    // manual_extent) instead, letting a learner inspect unusually small or
    // large vectors independent of their magnitude.
    bool auto_scale{true};
    // Ring interval used verbatim when auto_scale is false. Ignored when
    // auto_scale is true.
    double manual_ring_interval{1.0};
};

/// Number of concentric rings draw_polar_grid lays the auto-fit view out
/// over. `polarplot::PlotFrame::extent()` and `::ring_interval()` are always
/// related by `extent() == ring_interval() * kAutoFitRings`, and
/// `PlotFrame::ring_count()` is always `kAutoFitRings`.
inline constexpr int kAutoFitRings = 4;

/// Computes this frame's auto-fit extent/ring-interval/ring-count from the
/// magnitudes of the vectors currently shown -- \p inputs' A and B always,
/// plus each derived vector (sum, difference, B - A, the complex product
/// A x B, and the two complex quotients A / B, B / A) whenever its
/// respective show_* toggle is on, so a toggled-on derived vector can never
/// render past the visible extent regardless of how large its magnitude is
/// relative to A and B. A quotient with a zero divisor (undefined, see
/// vecmath::complex_divide) is simply excluded rather than contributing an
/// infinite magnitude. Pads the largest magnitude by a
/// margin, then snaps the result up to the smallest "nice" ring interval (a
/// value from the 1/2/5 x 10^k step sequence) such that kAutoFitRings rings
/// comfortably contain it, so the view stays visually stable (no jitter) as
/// inputs change continuously. Degenerate inputs (e.g. all vectors at the
/// origin) fall back to a default, non-zero interval rather than collapsing
/// the view. Pure -- no ImGui/ImPlot dependency.
[[nodiscard]] polarplot::PlotFrame auto_fit_extent(const PlotInputs& inputs);

/// The manual-scale counterpart to auto_fit_extent: uses \p ring_interval
/// verbatim (no snapping/padding) rather than deriving it from any vector's
/// magnitude, while still relating extent to it by
/// `extent == ring_interval * kAutoFitRings` so the ruler and grid never
/// disagree regardless of mode. Pure -- no ImGui/ImPlot dependency.
[[nodiscard]] polarplot::PlotFrame manual_extent(double ring_interval);

/// \c PlotPlan itself is defined in \c polar_plotting (\c polarplot::PlotPlan)
/// rather than here, purely so \c polarplot::draw_scene can take one by const
/// reference without \c polar_plotting depending on \c ui (see CLAUDE.md's
/// module table) -- every field it needs is already expressed in
/// \c polar_plotting's own vocabulary. This alias keeps \c ui::PlotPlan as
/// the name every \c ui caller (and \c tests/plot_plan_tests.cpp) already
/// uses. \c plan_plot() still owns all the toggle logic that decides one:
/// draw_plot() loops over the result and issues the matching polarplot::
/// draw calls -- no toggle logic of its own left over.
using PlotPlan = polarplot::PlotPlan;

/// Decide this frame's PlotPlan from \p inputs. Pure -- internally calls into
/// ui::construction's tip_to_tail_sum/tip_to_tail_difference/
/// difference_segment exactly as App::draw_plot() did before this seam
/// existed, and composes the angle sign via ui::angle_convention.
[[nodiscard]] PlotPlan plan_plot(const PlotInputs& inputs);

}  // namespace ui

#endif  // UI_PLOT_PLAN_HPP
