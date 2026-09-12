#ifndef POLAR_PLOTTING_POLAR_PLOT_HPP
#define POLAR_PLOTTING_POLAR_PLOT_HPP

/// \file
/// Thin helpers for drawing vectors on an ImPlot canvas with a polar reference
/// grid. Depends only on Dear ImGui and ImPlot -- deliberately no dependency on
/// the vector_math module so this can be lifted into another project.

#include <cstdint>

namespace polarplot {

/// A plain point in plot data coordinates.
struct Point {
    double x{0.0};
    double y{0.0};
};

/// Where 0 is drawn on the plot (\p zero_direction, radians, in the plot's
/// own coordinate frame) and which way angle increases as it grows
/// (\p angle_sign, +1 or -1). Owned entirely by \c polar_plotting: it carries
/// no notion of "rotation direction" or "measurement convention" -- callers
/// resolve those into a single sign before handing it in (see
/// docs/adr/0001-polar-plotting-receives-only-composed-angle-sign.md).
struct AngleConvention {
    double zero_direction{0.0};
    double angle_sign{1.0};
};

/// Map a math-convention angle (radians, measured from the +x axis,
/// increasing counterclockwise) to the angle actually used for drawing under
/// \p convention: `plotted_angle = zero_direction + angle_sign * math_angle`,
/// normalized to a full turn, i.e. the returned value lies in [0, 2*pi).
/// Pure function -- the test seam for the angle-convention concept.
[[nodiscard]] double apply_angle_convention(double math_angle, AngleConvention convention);

/// Remap \p p under \p convention: preserves its radius and rotates/reflects
/// its angle via \ref apply_angle_convention. The origin maps to itself.
[[nodiscard]] Point to_plotted_point(Point p, AngleConvention convention);

/// The two "wing" endpoints of a chevron arrowhead pointing from \p tail
/// toward \p head; the tip is \p head itself and is not part of this return
/// value. \c first/\c second are symmetric about the tail-to-head line.
struct ArrowheadWings {
    Point first;
    Point second;
};

/// Compute the wing endpoints of a chevron arrowhead for an arrow shaft
/// running from \p tail to \p head, with head length \p head_frac (fraction
/// of the tail-to-head distance) and a fixed wing half-width relative to
/// that head length. Pure function -- the test seam for arrowhead geometry,
/// shared by every arrow-drawing entry point below plus \ref draw_angle_arc.
/// When \p tail and \p head coincide (zero-length segment), both wings
/// coincide with \p head.
[[nodiscard]] ArrowheadWings arrowhead_wing_points(Point tail, Point head, double head_frac);

/// Begin an equal-aspect plot centred on the origin, spanning +/- \p extent on
/// both axes. Returns true when the plot is visible; call \ref end_vector_plot
/// exactly once iff this returned true (mirrors ImPlot::BeginPlot).
[[nodiscard]] bool begin_vector_plot(const char* title, double extent);

/// End a plot begun with \ref begin_vector_plot.
void end_vector_plot();

/// Draw concentric rings and radial spokes out to \p max_radius, laid out
/// according to \p convention.
void draw_polar_grid(double max_radius, AngleConvention convention, int rings = 4, int spokes = 12);

/// Style of the tip marker drawn at every named vector's tip. A single value
/// applies to every named vector on a plot -- there is no per-vector styling.
enum class TipMarkerStyle : std::uint8_t {
    kDot,
    kCrossHair,
};

/// Draw an arrow (shaft + head) from \p tail to \p head, labelled \p label.
/// \p tail and \p head are given in math convention and remapped via
/// \p convention before drawing. \p head_frac is the arrowhead length as a
/// fraction of the shaft length. \p thickness is the shaft/head line weight
/// in pixels, applied to both. This is the bare drawing primitive -- it
/// carries no tip marker or tip label; use \ref draw_vector for a named
/// vector, which always gets both.
void draw_arrow(const char* label, Point tail, Point head, AngleConvention convention,
                double head_frac = 0.12, float thickness = 2.0F);

/// Draw a named vector: an arrow from the origin to \p head, plus a tip
/// marker (styled per \p marker_style) and a tip label showing \p label,
/// both drawn unconditionally -- even when \p head is exactly the origin, in
/// which case the marker and label sit at the origin. This is what makes a
/// vector a "named vector" as opposed to a bare annotation arrow drawn via
/// \ref draw_arrow directly; \c polar_plotting has no notion of *why* a
/// vector is named, only that this entry point always marks and labels it.
/// \p thickness is the shaft/head line weight in pixels.
void draw_vector(const char* label, Point head, AngleConvention convention,
                 TipMarkerStyle marker_style, double head_frac = 0.12, float thickness = 2.0F);

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
/// frame. \p annotation's \c start and \c vector endpoint are given in math
/// convention and remapped via \p convention before drawing, exactly like
/// \ref draw_arrow. \p thickness is the shaft/head line weight in pixels.
void draw_annotation_vector(const char* id, AnnotationVector annotation, AngleConvention convention,
                            double head_frac = 0.12, float thickness = 2.0F);

/// Visual style for \ref draw_angle_arc: an RGBA color (components in [0, 1])
/// and a line thickness (pixels). Kept as a plain struct -- rather than an
/// ImGui/ImPlot type -- so this header doesn't need to include their headers.
struct ArcStyle {
    float r{1.0F};
    float g{0.65F};
    float b{0.0F};
    float a{1.0F};
    float thickness{2.0F};
};

/// Draw an arc of radius \p radius from the plot's fixed "up"/top reference
/// direction (12 o'clock, i.e. the plot's own +y axis) around to \p to_angle
/// (radians, in the plot's own raw coordinate frame -- the same frame
/// \c AngleConvention::zero_direction is expressed in), taking the shorter
/// way around. A pure "draw this arc now" primitive: it has no notion of
/// "transient" vs. "persistent" annotation lifecycles -- callers decide when
/// to call it each frame.
void draw_angle_arc(double radius, double to_angle, ArcStyle style = {});

}  // namespace polarplot

#endif  // POLAR_PLOTTING_POLAR_PLOT_HPP
