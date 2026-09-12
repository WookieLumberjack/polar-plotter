#include "polar_plotting/polar_plot.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <string>
#include <vector>

#include <implot.h>

namespace polarplot {
namespace {

constexpr double kPi = std::numbers::pi;
constexpr double kTwoPi = 2.0 * kPi;
constexpr double kTopAngle = kPi / 2.0;  // plot "up"/12 o'clock, in raw coordinates
constexpr double kRightAngle = 0.0;      // plot "right"/3 o'clock, in raw coordinates
constexpr int kArcSegments = 48;
// Fixed angular span of the rotation-direction indicator arc -- unlike the
// zero-direction arc's to_angle, this never grows/shrinks with any input.
constexpr double kRotationIndicatorSweep = kPi / 6.0;  // 30 degrees
// Spoke labels sit just outside the outer ring rather than exactly on it.
constexpr double kLabelRadiusFactor = 1.08;
// Headroom the axis view needs beyond a PlotFrame's extent so spoke-degree
// labels (drawn at kLabelRadiusFactor * extent, plus their own text width)
// aren't clipped by the plot's axis limits.
constexpr double kLabelPaddingFactor = 1.15;

// Build the ImPlotSpec used for an arrow's shaft/head: \p line_color when
// non-null, otherwise ImPlot's default per-item color cycling; \p thickness
// is always applied as the line weight.
ImPlotSpec arrow_line_spec(const ImVec4* line_color, float thickness) {
    ImPlotSpec spec;
    if (line_color != nullptr) {
        spec.LineColor = *line_color;
    }
    spec.LineWeight = thickness;
    return spec;
}

// Draw an arrow's shaft (\p shaft_id) and head (\p head_id) from \p tail to
// \p head (already in plotted/drawing coordinates -- callers remap via
// \ref to_plotted_point first), styled with \p line_color (nullptr for
// ImPlot's default color cycling) and \p thickness (line weight in pixels,
// shared by shaft and head).
void plot_arrow_shape(const std::string& shaft_id, const std::string& head_id, Point tail,
                      Point head, double head_frac, const ImVec4* line_color, float thickness) {
    const std::array<double, 2> sx{tail.x, head.x};
    const std::array<double, 2> sy{tail.y, head.y};
    ImPlot::PlotLine(shaft_id.c_str(), sx.data(), sy.data(), 2,
                     arrow_line_spec(line_color, thickness));

    // Resolve whatever color the shaft item actually ended up with -- either
    // the explicit line_color above, or ImPlot's auto-cycled colormap color
    // when line_color is nullptr -- and force the head to that same resolved
    // color. This guarantees shaft and head can never diverge, even though
    // they're drawn as two separate ImPlot items.
    const ImVec4 resolved_color = ImPlot::GetLastItemColor();

    if (head.x == tail.x && head.y == tail.y) {
        return;
    }

    const ArrowheadWings wings = arrowhead_wing_points(tail, head, head_frac);
    const std::array<double, 3> hx{wings.first.x, head.x, wings.second.x};
    const std::array<double, 3> hy{wings.first.y, head.y, wings.second.y};

    ImPlot::PlotLine(head_id.c_str(), hx.data(), hy.data(), 3,
                     arrow_line_spec(&resolved_color, thickness));
}

}  // namespace

ArrowheadWings arrowhead_wing_points(Point tail, Point head, double head_frac) {
    constexpr double kWing = 0.4;  // half-width of the head as a fraction of h

    const double dx = head.x - tail.x;
    const double dy = head.y - tail.y;
    const double len = std::hypot(dx, dy);
    if (len == 0.0) {
        return {head, head};
    }

    const double ux = dx / len;
    const double uy = dy / len;
    const double h = len * head_frac;

    const double back_x = head.x - (h * ux);
    const double back_y = head.y - (h * uy);
    const double wing_x = kWing * h * uy;
    const double wing_y = kWing * h * ux;

    return {Point{back_x + wing_x, back_y - wing_y}, Point{back_x - wing_x, back_y + wing_y}};
}

double inflate_for_labels(PlotFrame frame) { return frame.extent() * kLabelPaddingFactor; }

double apply_angle_convention(double math_angle, AngleConvention convention) {
    const double angle = convention.zero_direction + (convention.angle_sign * math_angle);
    double wrapped = std::fmod(angle, kTwoPi);
    if (wrapped < 0.0) {
        wrapped += kTwoPi;
    }
    return wrapped;
}

Point to_plotted_point(Point p, AngleConvention convention) {
    const double radius = std::hypot(p.x, p.y);
    if (radius == 0.0) {
        return {0.0, 0.0};
    }
    const double math_angle = std::atan2(p.y, p.x);
    const double plotted_angle = apply_angle_convention(math_angle, convention);
    return {radius * std::cos(plotted_angle), radius * std::sin(plotted_angle)};
}

std::vector<RulerTick> ruler_ticks(double ring_interval, int ring_count) {
    std::vector<RulerTick> ticks;
    ticks.reserve(static_cast<std::size_t>(ring_count));
    for (int r = 1; r <= ring_count; ++r) {
        const double position = ring_interval * static_cast<double>(r);
        std::array<char, 32> buf{};
        std::snprintf(buf.data(), buf.size(), "%.6g", position);
        ticks.push_back(RulerTick{position, std::string(buf.data())});
    }
    return ticks;
}

bool begin_vector_plot(const char* title, PlotFrame frame) {
    const double extent = inflate_for_labels(frame);
    // Suppress the rectangular plot-area border ImPlot draws by default; the
    // circular grid (see draw_polar_grid) is the only boundary we want
    // visible. Popped in end_vector_plot -- only when BeginPlot succeeds,
    // matching ImPlot's "only call EndPlot() if BeginPlot() returns true"
    // contract.
    ImPlot::PushStyleColor(ImPlotCol_PlotBorder, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
    constexpr ImPlotFlags kPlotFlags = ImPlotFlags_Equal | ImPlotFlags_NoInputs;
    if (!ImPlot::BeginPlot(title, ImVec2(-1, -1), kPlotFlags)) {
        ImPlot::PopStyleColor();
        return false;
    }
    // The polar grid drawn by draw_polar_grid is the only visual frame we
    // want; suppress the Cartesian x/y axes' lines, ticks, tick labels, and
    // gridlines entirely rather than just hiding the labels.
    constexpr ImPlotAxisFlags kAxisFlags = ImPlotAxisFlags_NoDecorations;
    ImPlot::SetupAxes("x", "y", kAxisFlags, kAxisFlags);
    // Re-apply every frame (ImPlotCond_Always) so leftover pan/zoom state
    // can never drift the limits away from the caller-supplied extent, even
    // though ImPlotFlags_NoInputs already blocks new pan/zoom input.
    ImPlot::SetupAxesLimits(-extent, extent, -extent, extent, ImPlotCond_Always);

    // Secondary vertical axis: a scale ruler on the plot's opposite (right)
    // side, showing the real distance each ring represents. No gridlines (the
    // polar grid already draws the rings) and locked against independent
    // pan/zoom -- it always mirrors the primary y axis' range.
    constexpr ImPlotAxisFlags kRulerFlags =
        ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_Lock;
    ImPlot::SetupAxis(ImAxis_Y2, nullptr, kRulerFlags);
    ImPlot::SetupAxisLimits(ImAxis_Y2, -extent, extent, ImPlotCond_Always);

    const std::vector<RulerTick> ticks = ruler_ticks(frame.ring_interval(), frame.ring_count());
    std::vector<double> positions;
    std::vector<std::string> label_strings;
    std::vector<const char*> label_pointers;
    positions.reserve(ticks.size());
    label_strings.reserve(ticks.size());
    label_pointers.reserve(ticks.size());
    for (const RulerTick& tick : ticks) {
        positions.push_back(tick.position);
        label_strings.push_back(tick.label);
    }
    for (const std::string& label : label_strings) {
        label_pointers.push_back(label.c_str());
    }
    ImPlot::SetupAxisTicks(ImAxis_Y2, positions.data(), static_cast<int>(positions.size()),
                           label_pointers.data());
    return true;
}

void end_vector_plot() {
    ImPlot::EndPlot();
    ImPlot::PopStyleColor();
}

void draw_polar_grid(PlotFrame frame, AngleConvention convention, int spokes) {
    // One muted, fixed color for the whole grid (no per-ring colormap
    // cycling); the outermost ring is drawn heavier than the interior rings
    // and spokes so the plot's boundary reads clearly without a bounding box.
    constexpr ImVec4 kGridColor{0.5F, 0.5F, 0.5F, 0.5F};
    constexpr float kInteriorWeight = 1.0F;
    constexpr float kOuterWeight = 2.5F;

    const double max_radius = frame.extent();
    const int rings = frame.ring_count();

    constexpr int kSegments = 96;
    std::array<double, kSegments> cx{};
    std::array<double, kSegments> cy{};

    for (int r = 1; r <= rings; ++r) {
        const double radius = max_radius * static_cast<double>(r) / rings;
        for (int i = 0; i < kSegments; ++i) {
            const double t = kTwoPi * static_cast<double>(i) / kSegments;
            const double plotted = apply_angle_convention(t, convention);
            cx[static_cast<std::size_t>(i)] = radius * std::cos(plotted);
            cy[static_cast<std::size_t>(i)] = radius * std::sin(plotted);
        }
        const std::string id = "##ring" + std::to_string(r);
        const float weight = (r == rings) ? kOuterWeight : kInteriorWeight;
        const ImPlotSpec spec{ImPlotProp_Flags, ImPlotLineFlags_Loop,  ImPlotProp_LineColor,
                              kGridColor,       ImPlotProp_LineWeight, weight};
        ImPlot::PlotLine(id.c_str(), cx.data(), cy.data(), kSegments, spec);
    }

    for (int s = 0; s < spokes; ++s) {
        const double t = kTwoPi * static_cast<double>(s) / spokes;
        const double plotted = apply_angle_convention(t, convention);
        const std::array<double, 2> sx{0.0, max_radius * std::cos(plotted)};
        const std::array<double, 2> sy{0.0, max_radius * std::sin(plotted)};
        const std::string id = "##spoke" + std::to_string(s);
        const ImPlotSpec spec{ImPlotProp_LineColor, kGridColor, ImPlotProp_LineWeight,
                              kInteriorWeight};
        ImPlot::PlotLine(id.c_str(), sx.data(), sy.data(), 2, spec);
    }

    // Muted, slightly more opaque than the grid lines themselves so the
    // degree labels stay legible without competing with vectors/tip labels.
    constexpr ImVec4 kLabelColor{0.65F, 0.65F, 0.65F, 0.9F};
    for (const SpokeLabel& label : spoke_labels(max_radius, convention, spokes)) {
        ImPlot::Annotation(label.position.x, label.position.y, kLabelColor, ImVec2(0.0F, 0.0F),
                           false, "%s", label.text.c_str());
    }
}

std::vector<SpokeLabel> spoke_labels(double max_radius, AngleConvention convention,
                                     int spoke_count) {
    std::vector<SpokeLabel> labels;
    labels.reserve(static_cast<std::size_t>(spoke_count));

    const double label_radius = max_radius * kLabelRadiusFactor;
    for (int s = 0; s < spoke_count; ++s) {
        const double t = kTwoPi * static_cast<double>(s) / spoke_count;
        const double plotted = apply_angle_convention(t, convention);
        const Point position{label_radius * std::cos(plotted), label_radius * std::sin(plotted)};

        const double degrees = t * (180.0 / kPi);
        const std::string text = std::to_string(static_cast<int>(std::lround(degrees))) + "°";

        labels.push_back(SpokeLabel{position, text});
    }
    return labels;
}

void draw_arrow(const char* label, Point tail, Point head, AngleConvention convention,
                double head_frac, float thickness) {
    const Point ptail = to_plotted_point(tail, convention);
    const Point phead = to_plotted_point(head, convention);
    const std::string head_id = std::string("##head_") + label;
    plot_arrow_shape(label, head_id, ptail, phead, head_frac, /*line_color=*/nullptr, thickness);
}

namespace {

// Tip marker/label styling. Fixed constants -- exact pixel sizes are left to
// implementation time per the spec.
constexpr float kMarkerSize = 6.0F;
constexpr ImVec2 kLabelPixelOffset{8.0F, -8.0F};
constexpr ImVec4 kTipColor{0.9F, 0.9F, 0.9F, 1.0F};

void draw_tip_marker(const char* label, Point tip, TipMarkerStyle marker_style) {
    const std::string id = std::string("##tip_") + label;
    const ImPlotMarker marker =
        (marker_style == TipMarkerStyle::kCrossHair) ? ImPlotMarker_Cross : ImPlotMarker_Circle;
    const ImPlotSpec spec{
        ImPlotProp_Marker,          marker,    ImPlotProp_MarkerSize,      kMarkerSize,
        ImPlotProp_MarkerFillColor, kTipColor, ImPlotProp_MarkerLineColor, kTipColor};
    ImPlot::PlotScatter(id.c_str(), &tip.x, &tip.y, 1, spec);
}

void draw_tip_label(const char* label, Point tip) {
    ImPlot::Annotation(tip.x, tip.y, kTipColor, kLabelPixelOffset, false, "%s", label);
}

}  // namespace

void draw_vector(const char* label, Point head, AngleConvention convention,
                 TipMarkerStyle marker_style, double head_frac, float thickness) {
    draw_arrow(label, Point{0.0, 0.0}, head, convention, head_frac, thickness);

    const Point tip = to_plotted_point(head, convention);
    draw_tip_marker(label, tip, marker_style);
    draw_tip_label(label, tip);
}

void draw_annotation_vector(const char* id, AnnotationVector annotation, AngleConvention convention,
                            double head_frac, float thickness) {
    // Muted, semi-transparent gray -- distinct from named vectors, which cycle
    // through ImPlot's saturated default colormap.
    constexpr ImVec4 kAnnotationColor{0.55F, 0.55F, 0.55F, 0.65F};

    const Point tail = annotation.start;
    const Point head{tail.x + annotation.vector.x, tail.y + annotation.vector.y};
    const Point ptail = to_plotted_point(tail, convention);
    const Point phead = to_plotted_point(head, convention);

    const std::string shaft_id = std::string("##annotation_") + id;
    const std::string head_id = std::string("##annotation_head_") + id;
    plot_arrow_shape(shaft_id, head_id, ptail, phead, head_frac, &kAnnotationColor, thickness);
}

namespace {

// Draw an arc of radius \p radius sweeping from \p from_angle to \p to_angle
// (raw plot coordinates, taken exactly as given -- no shortest-way wrapping),
// plus a chevron arrowhead at the \p to_angle end pointing along the arc's
// local tangent there, under \p style. \p line_id/\p head_id are the ImPlot
// item ids for the arc line and its arrowhead respectively. Shared drawing
// primitive behind \ref draw_angle_arc and \ref draw_rotation_indicator --
// they differ only in how from_angle/to_angle are computed.
void draw_arc_with_head(double radius, double from_angle, double to_angle, ArcStyle style,
                        const char* line_id, const char* head_id) {
    const double delta = to_angle - from_angle;

    std::array<double, kArcSegments + 1> ax{};
    std::array<double, kArcSegments + 1> ay{};
    for (int i = 0; i <= kArcSegments; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(kArcSegments);
        const double angle = from_angle + (delta * t);
        ax[static_cast<std::size_t>(i)] = radius * std::cos(angle);
        ay[static_cast<std::size_t>(i)] = radius * std::sin(angle);
    }

    const ImVec4 color{style.r, style.g, style.b, style.a};
    const ImPlotSpec spec{ImPlotProp_LineColor, color, ImPlotProp_LineWeight, style.thickness};
    ImPlot::PlotLine(line_id, ax.data(), ay.data(), kArcSegments + 1, spec);

    // Chevron arrowhead at the to_angle end, pointing along the arc's local
    // tangent there. The last arc segment is far too short to size the head
    // off directly (it shrinks with kArcSegments), so build a synthetic tail
    // that is exactly the desired head length behind the tip, along that same
    // tangent direction, and hand it to arrowhead_wing_points with
    // head_frac = 1.0 so the full synthetic length becomes the head length.
    constexpr double kArcHeadFrac = 0.12;  // head length as a fraction of radius
    const Point arc_tip{ax[kArcSegments], ay[kArcSegments]};
    const double tangent_dx = ax[kArcSegments] - ax[kArcSegments - 1];
    const double tangent_dy = ay[kArcSegments] - ay[kArcSegments - 1];
    const double tangent_len = std::hypot(tangent_dx, tangent_dy);
    if (tangent_len > 0.0) {
        const double head_len = radius * kArcHeadFrac;
        const double ux = tangent_dx / tangent_len;
        const double uy = tangent_dy / tangent_len;
        const Point synthetic_tail{arc_tip.x - (head_len * ux), arc_tip.y - (head_len * uy)};
        const ArrowheadWings wings = arrowhead_wing_points(synthetic_tail, arc_tip, 1.0);
        const std::array<double, 3> hx{wings.first.x, arc_tip.x, wings.second.x};
        const std::array<double, 3> hy{wings.first.y, arc_tip.y, wings.second.y};
        ImPlot::PlotLine(head_id, hx.data(), hy.data(), 3, spec);
    }
}

}  // namespace

void draw_angle_arc(double radius, double to_angle, ArcStyle style) {
    // Sweep the shorter way from top to to_angle: wrap the delta into
    // (-pi, pi].
    double delta = std::fmod(to_angle - kTopAngle, kTwoPi);
    if (delta <= -kPi) {
        delta += kTwoPi;
    } else if (delta > kPi) {
        delta -= kTwoPi;
    }

    draw_arc_with_head(radius, kTopAngle, kTopAngle + delta, style, "##angle_arc",
                       "##angle_arc_head");
}

RotationIndicatorArc rotation_indicator_arc(double sweep_sign) {
    const double sign = sweep_sign < 0.0 ? -1.0 : 1.0;
    return {.from_angle = kRightAngle, .to_angle = kRightAngle + (sign * kRotationIndicatorSweep)};
}

void draw_rotation_indicator(PlotFrame frame, double sweep_sign, ArcStyle style) {
    const RotationIndicatorArc arc = rotation_indicator_arc(sweep_sign);
    draw_arc_with_head(frame.extent(), arc.from_angle, arc.to_angle, style, "##rotation_indicator",
                       "##rotation_indicator_head");
}

}  // namespace polarplot
