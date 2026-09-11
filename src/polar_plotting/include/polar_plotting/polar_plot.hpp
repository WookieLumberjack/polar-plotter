#ifndef POLAR_PLOTTING_POLAR_PLOT_HPP
#define POLAR_PLOTTING_POLAR_PLOT_HPP

/// \file
/// Thin helpers for drawing vectors on an ImPlot canvas with a polar reference
/// grid. Depends only on Dear ImGui and ImPlot -- deliberately no dependency on
/// the vector_math module so this can be lifted into another project.

namespace polarplot {

/// A plain point in plot data coordinates.
struct Point {
    double x{0.0};
    double y{0.0};
};

/// Begin an equal-aspect plot centred on the origin, spanning +/- \p extent on
/// both axes. Returns true when the plot is visible; call \ref end_vector_plot
/// exactly once iff this returned true (mirrors ImPlot::BeginPlot).
[[nodiscard]] bool begin_vector_plot(const char* title, double extent);

/// End a plot begun with \ref begin_vector_plot.
void end_vector_plot();

/// Draw concentric rings and radial spokes out to \p max_radius.
void draw_polar_grid(double max_radius, int rings = 4, int spokes = 12);

/// Draw an arrow (shaft + head) from \p tail to \p head, labelled \p label.
/// \p head_frac is the arrowhead length as a fraction of the shaft length.
void draw_arrow(const char* label, Point tail, Point head, double head_frac = 0.12);

/// Convenience: an arrow from the origin to \p head.
void draw_vector(const char* label, Point head, double head_frac = 0.12);

/// A free-vector annotation: an arrow beginning at an explicit \p start point
/// (never assumed to originate at the origin) and displaced by \p vector.
/// \c polar_plotting has no notion of what an annotation vector represents --
/// callers (e.g. \c ui) pass these in purely to be drawn, for things like a
/// tip-to-tail construction step.
struct AnnotationVector {
    Point start;
    Point vector;
};

/// Draw \p annotation as a free-vector arrow, styled distinctly from a named
/// vector (see \ref draw_vector / \ref draw_arrow) and with no tip marker or
/// label of its own. \p id is an ImPlot item id used only for internal
/// bookkeeping -- it is never shown as a legend entry or on-plot label --
/// so pass a value unique among annotations drawn in the same plot this
/// frame.
void draw_annotation_vector(const char* id, AnnotationVector annotation, double head_frac = 0.12);

}  // namespace polarplot

#endif  // POLAR_PLOTTING_POLAR_PLOT_HPP
