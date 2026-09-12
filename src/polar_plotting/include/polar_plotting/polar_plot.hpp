#ifndef POLAR_PLOTTING_POLAR_PLOT_HPP
#define POLAR_PLOTTING_POLAR_PLOT_HPP

/// \file
/// Thin helpers for drawing vectors on an ImPlot canvas with a polar reference
/// grid. Depends only on Dear ImGui and ImPlot -- deliberately no dependency on
/// the vector_math module so this can be lifted into another project.

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

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

/// The three related numbers describing one frame's polar view: the plotted
/// axis extent (+/- \c extent() on both axes), the radius spacing between
/// rings (\c ring_interval()), and how many rings are drawn
/// (\c ring_count()). Always related by `extent() == ring_interval() *
/// ring_count()` -- enforced by construction, since \c extent() is derived
/// rather than stored, so there is no second number for it to disagree with.
/// Owned entirely by \c polar_plotting, exactly like \ref AngleConvention --
/// it carries no vector-magnitude reasoning of its own; callers (e.g.
/// \c ui::plot_plan) compute \p ring_interval/\p ring_count and hand them to
/// the constructor.
class PlotFrame {
public:
    /// Builds a frame from \p ring_interval and \p ring_count, both of which
    /// must be strictly positive -- asserted as a precondition, not clamped
    /// or silently repaired, so a caller bug surfaces immediately instead of
    /// producing silently-wrong plot geometry.
    constexpr PlotFrame(double ring_interval, int ring_count)
        : ring_interval_(ring_interval), ring_count_(ring_count) {
        assert(ring_interval > 0.0 && "PlotFrame requires a strictly positive ring_interval");
        assert(ring_count > 0 && "PlotFrame requires a strictly positive ring_count");
    }

    [[nodiscard]] constexpr double extent() const {
        return ring_interval_ * static_cast<double>(ring_count_);
    }
    [[nodiscard]] constexpr double ring_interval() const { return ring_interval_; }
    [[nodiscard]] constexpr int ring_count() const { return ring_count_; }

private:
    double ring_interval_;
    int ring_count_;
};

/// Inflate \p frame's \c extent by a fixed headroom factor so the
/// spoke-degree labels drawn just outside the outer ring (see
/// \ref spoke_labels) have room to render without being clipped by the axis
/// view. Pure function -- the test seam for label-headroom padding.
[[nodiscard]] double inflate_for_labels(PlotFrame frame);

/// Map a math-convention angle (radians, measured from the +x axis,
/// increasing counterclockwise) to the angle actually used for drawing under
/// \p convention: `plotted_angle = zero_direction + angle_sign * math_angle`,
/// normalized to a full turn, i.e. the returned value lies in [0, 2*pi).
/// Pure function -- the test seam for the angle-convention concept.
[[nodiscard]] double apply_angle_convention(double math_angle, AngleConvention convention);

/// Remap \p p under \p convention: preserves its radius and rotates/reflects
/// its angle via \ref apply_angle_convention. The origin maps to itself.
[[nodiscard]] Point to_plotted_point(Point p, AngleConvention convention);

/// The pure inverse of \ref to_plotted_point: given \p p already in
/// plotted/visual space, undo the zero-direction/angle-sign remap and
/// recover the point in raw math-convention space. Preserves radius; the
/// origin maps to itself. Satisfies
/// `from_plotted_point(to_plotted_point(p, c), c) == p` for any \p p and
/// convention \p c (within floating-point tolerance).
[[nodiscard]] Point from_plotted_point(Point p, AngleConvention convention);

/// Snap \p p's angle to the nearest multiple of \p increment_radians,
/// preserving \p p's radius exactly -- only the angle changes. \p p is taken
/// in whatever space it's given (see #50: callers snap a point already in
/// plotted/visual space, i.e. after \ref to_plotted_point's remap, so the
/// snap aligns with the grid as drawn rather than the raw math angle). The
/// origin maps to itself (no angle to snap at zero radius). Pure function --
/// the test seam for shift-to-snap angle logic.
[[nodiscard]] Point snap_angle_to_increment(Point p, double increment_radians);

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

/// One explicit tick on the vertical scale ruler (see \ref begin_vector_plot):
/// \p position is the tick's value along the plot's y axis (i.e. a ring's
/// radius) and \p label is its formatted text.
struct RulerTick {
    double position;
    std::string label;
};

/// Compute the vertical scale ruler's ticks: one per ring, at radius
/// `ring_interval * r` for `r` in `[1, ring_count]`, labelled with that
/// radius value. Pure function -- the test seam for the ruler's tick
/// placement, mirroring \ref spoke_labels for the grid's spokes.
[[nodiscard]] std::vector<RulerTick> ruler_ticks(double ring_interval, int ring_count = 4);

/// Begin an equal-aspect plot centred on the origin, spanning +/-
/// \ref inflate_for_labels "inflate_for_labels(frame)" on both axes (headroom
/// beyond \p frame's own extent so spoke-degree labels aren't clipped), plus a
/// locked secondary vertical axis (ImPlot's Y2) on the plot's right side
/// showing a scale ruler: explicit ticks at each ring's radius (see
/// \ref ruler_ticks), computed from \p frame's \c ring_interval and
/// \c ring_count -- callers must pass the same \p frame to \ref draw_polar_grid
/// and \ref draw_rotation_indicator this frame so the ruler and the grid never
/// disagree. Returns true when the plot is visible; call \ref end_vector_plot
/// exactly once iff this returned true (mirrors ImPlot::BeginPlot).
[[nodiscard]] bool begin_vector_plot(const char* title, PlotFrame frame);

/// End a plot begun with \ref begin_vector_plot.
void end_vector_plot();

/// Draw concentric rings and radial spokes out to \p frame's \c extent (with
/// \p frame's \c ring_count rings), laid out according to \p convention, plus
/// a degree label just outside each spoke (see \ref spoke_labels). Pass the
/// same \p frame given to \ref begin_vector_plot this frame.
void draw_polar_grid(PlotFrame frame, AngleConvention convention, int spokes = 12);

/// One spoke's degree label: \p position is where it should be drawn (in the
/// plot's own drawing coordinates, already remapped via \p convention) and
/// \p text is its formatted content, e.g. `"30°"`.
struct SpokeLabel {
    Point position;
    std::string text;
};

/// Compute the position and text of each spoke's degree label for a grid out
/// to \p max_radius with \p spoke_count evenly spaced spokes. Spoke `s`'s
/// label text is always its un-rotated math angle (`s * 360 / spoke_count`
/// degrees, formatted with a `°` suffix) -- so spoke 0 is always "0°" -- while
/// its label *position* is remapped via \ref apply_angle_convention under
/// \p convention, at a radius just beyond \p max_radius, so that the labels'
/// positions rotate/mirror with the grid while their text always reads the
/// plain math angle. Pure function -- the test seam for spoke labeling.
[[nodiscard]] std::vector<SpokeLabel> spoke_labels(double max_radius, AngleConvention convention,
                                                   int spoke_count = 12);

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

/// RGBA color override for a named vector's tip marker (components in
/// [0, 1]). Kept as a plain struct -- rather than an ImGui/ImPlot type -- so
/// this header doesn't need to include their headers, matching \ref ArcStyle.
struct MarkerColor {
    float r{0.0F};
    float g{0.0F};
    float b{0.0F};
    float a{1.0F};
};

/// The fixed, shared marker color used for a hovered tip (see #44/#47):
/// applies identically to A or B, never per-vector-tinted, and is distinct
/// from \ref draw_vector's default idle marker color.
inline constexpr MarkerColor kHoverMarkerColor{1.0F, 0.85F, 0.2F, 1.0F};

/// Draw a named vector: an arrow from the origin to \p head, plus a tip
/// marker (styled per \p marker_style) and a tip label showing \p label,
/// both drawn unconditionally -- even when \p head is exactly the origin, in
/// which case the marker and label sit at the origin. This is what makes a
/// vector a "named vector" as opposed to a bare annotation arrow drawn via
/// \ref draw_arrow directly; \c polar_plotting has no notion of *why* a
/// vector is named, only that this entry point always marks and labels it.
/// \p thickness is the shaft/head line weight in pixels. \p marker_color, when
/// non-null, overrides the tip marker's fill/line color (e.g. with
/// \ref kHoverMarkerColor for a hover cue); it is left null by every existing
/// caller, so the default appearance (today's fixed idle color) is unchanged.
/// The override applies only to the tip marker -- the shaft, arrowhead, and
/// tip label always keep their normal color.
void draw_vector(const char* label, Point head, AngleConvention convention,
                 TipMarkerStyle marker_style, double head_frac = 0.12, float thickness = 2.0F,
                 const MarkerColor* marker_color = nullptr);

/// Which of A's or B's tip (if either) is the target of a hover/drag
/// interaction this frame; \c kNone when neither is within hit range of the
/// cursor. See #44 for the broader interaction state machine this is one
/// piece of.
enum class HoverTarget : std::uint8_t {
    kNone,
    kA,
    kB,
};

/// Pure hit-test: given the mouse position and both named vectors' tips
/// (\p mouse, \p tip_a, \p tip_b -- all three in the same consistent space,
/// e.g. pixel coordinates, so plain Euclidean distance is meaningful), decide
/// which tip (if any) is hit within \p hit_radius of the mouse, applying
/// #44's overlap tie-break: when both A's and B's tips are within
/// \p hit_radius of the mouse, the nearer one wins; an exact tie (equal
/// distance) favors A. Independent of ImGui/ImPlot state -- the test seam for
/// hover/drag hit-testing.
[[nodiscard]] HoverTarget hover_hit_test(Point mouse, Point tip_a, Point tip_b, double hit_radius);

/// Impure integration point: resolves \p head_a/\p head_b (math-convention
/// space, like \ref draw_vector's \p head) to their on-screen tip positions
/// under \p convention, queries the live mouse position, and applies
/// \ref hover_hit_test with the fixed pixel hit-radius rule from #44
/// (`max(marker's own pixel size, 10px)`). Must be called between
/// \ref begin_vector_plot / \ref end_vector_plot this frame, after A and B's
/// positions for the frame are known. Returns \c kNone when the plot itself
/// isn't hovered.
[[nodiscard]] HoverTarget hover_target(Point head_a, Point head_b, AngleConvention convention);

/// This frame's interaction state for one vector driven by
/// \ref draw_interactive_vector: \c kIdle (untouched), \c kHovered (cursor
/// within hit range, button up), \c kDragging (button pressed while hovered,
/// still held -- possibly no longer hovered, since a drag continues even if
/// the tip moves out from under the cursor), or \c kReleased (was dragging
/// last frame, button now up -- exactly one frame, then back to \c kIdle or
/// \c kHovered). See #44/#48.
enum class InteractionState : std::uint8_t {
    kIdle,
    kHovered,
    kDragging,
    kReleased,
};

/// Pure decision logic behind \ref draw_interactive_vector's state machine:
/// given whether this vector \p was_dragging as of the previous frame
/// (cross-frame state owned by the caller -- \c polar_plotting keeps none of
/// its own), whether it \p is_hover_target this frame (from
/// \ref hover_target), and the mouse's \p mouse_pressed (true only on the
/// press edge, e.g. ImGui's \c IsMouseClicked) / \p mouse_down (held, e.g.
/// \c IsMouseDown) state, decides this frame's \ref InteractionState:
/// dragging continues for as long as the button stays held once started
/// (regardless of \p is_hover_target), a drag only *starts* on a press that
/// coincides with \p is_hover_target, and letting go while dragging yields
/// exactly one \c kReleased frame. Pure function, independent of
/// ImGui/ImPlot -- the test seam for this state machine.
[[nodiscard]] InteractionState resolve_interaction_state(bool was_dragging, bool is_hover_target,
                                                         bool mouse_pressed, bool mouse_down);

/// The fixed, shared marker color used for an actively-dragging tip (see
/// #44/#48): applies identically to A or B, never per-vector-tinted, and
/// distinct from both \ref draw_vector's default idle color and
/// \ref kHoverMarkerColor.
inline constexpr MarkerColor kDraggingMarkerColor{1.0F, 0.25F, 0.25F, 1.0F};

/// Result of \ref draw_interactive_vector: \p head is the vector's head this
/// frame, in the same math-convention space \ref draw_vector's \p head
/// parameter takes (updated live while dragging via \ref from_plotted_point;
/// frozen at the release position on and after a \c kReleased frame), and
/// \p state is this frame's \ref InteractionState -- pass it back in as next
/// frame's \p was_dragging (true iff \c kDragging).
struct InteractiveVectorResult {
    Point head;
    InteractionState state{InteractionState::kIdle};
};

/// Impure integration point layering #44/#48's hover/drag mechanics on top of
/// \ref draw_vector for one named vector (A or B). \p head is the vector's
/// current head (math-convention space); \p is_hover_target is this frame's
/// hit-test result for *this* vector (from \ref hover_target, computed once
/// per frame across both A and B, before calling this for either); \p
/// was_dragging is this same vector's \ref InteractiveVectorResult::state
/// from last frame, reduced to a bool (see \ref resolve_interaction_state).
/// Hand-rolled via \c ImGui::IsMouseDown/IsMouseClicked and
/// \c ImPlot::GetPlotMousePos -- deliberately not \c ImPlot::DragPoint, which
/// would replace the existing tip marker with its own plain circular marker.
/// Must be called between \ref begin_vector_plot/\ref end_vector_plot, after
/// \p is_hover_target for this frame is known. Draws the vector exactly like
/// \ref draw_vector (arrow + tip marker + tip label), with the tip marker
/// recolored to \ref kHoverMarkerColor or \ref kDraggingMarkerColor per the
/// resolved \ref InteractionState (or left at its normal idle color), and
/// returns the updated head position plus that state for the caller (e.g.
/// \c ui::App) to store back into its own vector state.
///
/// Shift-to-snap (#50): while dragging, holding Shift (checked live every
/// frame via \c ImGui::GetIO().KeyShift) snaps the head's angle to the
/// nearest 15 degree increment in plotted/visual space -- i.e. the drag
/// position is snapped via \ref snap_angle_to_increment *after*
/// \ref to_plotted_point's remap and before converting back with
/// \ref from_plotted_point, so it aligns with the grid as drawn regardless
/// of \p convention. Magnitude/radius is never snapped. Releasing Shift
/// mid-drag takes effect the very next frame.
[[nodiscard]] InteractiveVectorResult draw_interactive_vector(
    const char* label, Point head, AngleConvention convention, TipMarkerStyle marker_style,
    bool is_hover_target, bool was_dragging, double head_frac = 0.12, float thickness = 2.0F);

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

/// Visual style for \ref draw_angle_arc and \ref draw_rotation_indicator: an
/// RGBA color (components in [0, 1]) and a line thickness (pixels). Kept as a
/// plain struct -- rather than an ImGui/ImPlot type -- so this header doesn't
/// need to include their headers.
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

/// The start/end angle (radians, plot's raw coordinate frame) of the
/// rotation-direction indicator arc drawn by \ref draw_rotation_indicator for
/// a given \p sweep_sign. Centered on the plot's right/3-o'clock reference
/// direction (as opposed to \ref draw_angle_arc's top/12-o'clock reference),
/// so the two arcs never occupy the same position regardless of \p sweep_sign
/// or the zero-direction arc's angle. Only \p sweep_sign's sign matters (its
/// magnitude is ignored, and a value of exactly 0.0 is treated as
/// counterclockwise). Pure function -- the test seam for this geometry.
struct RotationIndicatorArc {
    double from_angle;
    double to_angle;
};
[[nodiscard]] RotationIndicatorArc rotation_indicator_arc(double sweep_sign);

/// Draw a short curved-arrow arc of radius \p frame's \c extent on the outer
/// ring, indicating a rotation direction: \p sweep_sign > 0 curves
/// counterclockwise, < 0 clockwise (only the sign is used -- see
/// \ref rotation_indicator_arc). Takes only this plain sweep-sign double,
/// never a domain "rotation direction" or "measurement convention" type (see
/// docs/agents' ADR 0001), so this stays liftable outside the app. Visually
/// distinct from \ref draw_angle_arc's zero-direction arc: fixed short span,
/// centered on the plot's right/3-o'clock reference direction rather than
/// growing from straight up. A pure "draw this now" primitive, like
/// \ref draw_angle_arc -- callers decide when to call it each frame. Pass the
/// same \p frame given to \ref begin_vector_plot this frame.
void draw_rotation_indicator(PlotFrame frame, double sweep_sign, ArcStyle style = {});

}  // namespace polarplot

#endif  // POLAR_PLOTTING_POLAR_PLOT_HPP
