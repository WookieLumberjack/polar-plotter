#include "polar_plotting/polar_plot.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <string>
#include <vector>

#include <imgui.h>
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
// shared by shaft and head). The head is always kHeadLengthPixels long on
// screen (see head_frac_for_fixed_pixels), independent of the shaft's
// data-space length or the plot's current zoom.
void plot_arrow_shape(const std::string& shaft_id, const std::string& head_id, Point tail,
                      Point head, const ImVec4* line_color, float thickness) {
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

    // Extend ImPlot's legend hover-bold effect (already applied automatically
    // to the shaft item itself, since that's a real legend entry) to the
    // arrowhead, which is a separate `##`-prefixed ImPlot item with no legend
    // entry of its own and therefore doesn't participate in that automatic
    // scaling on its own -- #71. Matches ImPlot's own internal
    // ITEM_HIGHLIGHT_LINE_SCALE. shaft_id is only ever a real (non `##`)
    // legend entry for a named vector's own shaft (see draw_arrow); an
    // annotation's `##`-prefixed shaft_id never has a legend entry, so this
    // is always false for annotations.
    constexpr float kHoverLineScale = 2.0F;
    const float head_thickness =
        ImPlot::IsLegendEntryHovered(shaft_id.c_str()) ? thickness * kHoverLineScale : thickness;

    const ImVec2 pixel_tail = ImPlot::PlotToPixels(tail.x, tail.y);
    const ImVec2 pixel_head = ImPlot::PlotToPixels(head.x, head.y);
    const double shaft_length_px = std::hypot(static_cast<double>(pixel_head.x - pixel_tail.x),
                                              static_cast<double>(pixel_head.y - pixel_tail.y));
    const double head_frac =
        head_frac_for_fixed_pixels(static_cast<double>(kHeadLengthPixels), shaft_length_px);

    const ArrowheadWings wings = arrowhead_wing_points(tail, head, head_frac);
    const std::array<double, 3> hx{wings.first.x, head.x, wings.second.x};
    const std::array<double, 3> hy{wings.first.y, head.y, wings.second.y};

    ImPlot::PlotLine(head_id.c_str(), hx.data(), hy.data(), 3,
                     arrow_line_spec(&resolved_color, head_thickness));
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

ZeroDirectionTick zero_direction_arc_tick(double radius, double extent) {
    constexpr double kTickHalfLengthFactor = 0.025;
    const double half_length = kTickHalfLengthFactor * extent;
    return {Point{0.0, radius - half_length}, Point{0.0, radius + half_length}};
}

double head_frac_for_fixed_pixels(double head_length_px, double shaft_length_px) {
    constexpr double kMaxHeadFrac = 0.9;
    if (shaft_length_px <= 0.0) {
        return 0.0;
    }
    return std::clamp(head_length_px / shaft_length_px, 0.0, kMaxHeadFrac);
}

double inflate_for_labels(PlotFrame frame) { return frame.extent() * kLabelPaddingFactor; }

AxisHalfRanges axis_half_ranges(double canvas_width_px, double canvas_height_px,
                                double half_range) {
    // Degenerate canvas (e.g. before the first layout pass) -- fall back to
    // equal half-ranges rather than dividing by zero/negative pixel extents.
    if (canvas_width_px <= 0.0 || canvas_height_px <= 0.0) {
        return {half_range, half_range};
    }
    if (canvas_width_px < canvas_height_px) {
        return {half_range, half_range * (canvas_height_px / canvas_width_px)};
    }
    if (canvas_height_px < canvas_width_px) {
        return {half_range * (canvas_width_px / canvas_height_px), half_range};
    }
    return {half_range, half_range};
}

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

Point from_plotted_point(Point p, AngleConvention convention) {
    const double radius = std::hypot(p.x, p.y);
    if (radius == 0.0) {
        return {0.0, 0.0};
    }
    const double plotted_angle = std::atan2(p.y, p.x);
    // Inverse of apply_angle_convention's
    // `plotted = zero_direction + angle_sign * math_angle`: solve for
    // math_angle. angle_sign is always +-1, so dividing by it is the same as
    // multiplying by it.
    const double math_angle = convention.angle_sign * (plotted_angle - convention.zero_direction);
    return {radius * std::cos(math_angle), radius * std::sin(math_angle)};
}

Point snap_angle_to_increment(Point p, double increment_radians) {
    const double radius = std::hypot(p.x, p.y);
    if (radius == 0.0) {
        return {0.0, 0.0};
    }
    const double angle = std::atan2(p.y, p.x);
    const double snapped_angle = std::round(angle / increment_radians) * increment_radians;
    return {radius * std::cos(snapped_angle), radius * std::sin(snapped_angle)};
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

std::vector<double> minor_ruler_ticks(double ring_interval, int ring_count,
                                      int subdivisions_per_ring) {
    std::vector<double> ticks;
    if (subdivisions_per_ring <= 1) {
        return ticks;
    }
    ticks.reserve(static_cast<std::size_t>(ring_count) *
                  static_cast<std::size_t>(subdivisions_per_ring - 1));
    for (int r = 0; r < ring_count; ++r) {
        const double segment_start = ring_interval * static_cast<double>(r);
        for (int k = 1; k < subdivisions_per_ring; ++k) {
            const double offset =
                ring_interval * static_cast<double>(k) / static_cast<double>(subdivisions_per_ring);
            ticks.push_back(segment_start + offset);
        }
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
    // Captured before BeginPlot, since the plot fills whatever space is
    // available (ImVec2(-1, -1) below) -- this is that space's pixel size.
    // Drives axis_half_ranges so the rings stay circular at any aspect ratio
    // (#61) instead of relying on ImPlotFlags_Equal, which fights manual
    // per-axis limits once the canvas isn't square.
    const ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    constexpr ImPlotFlags kPlotFlags = ImPlotFlags_NoInputs;
    if (!ImPlot::BeginPlot(title, ImVec2(-1, -1), kPlotFlags)) {
        ImPlot::PopStyleColor();
        return false;
    }
    // The polar grid drawn by draw_polar_grid is the only visual frame we
    // want; suppress the Cartesian x/y axes' lines, ticks, tick labels, and
    // gridlines entirely rather than just hiding the labels.
    constexpr ImPlotAxisFlags kAxisFlags = ImPlotAxisFlags_NoDecorations;
    ImPlot::SetupAxes("x", "y", kAxisFlags, kAxisFlags);
    // NoButtons: legend visibility is controlled solely by the side panel's
    // checkboxes -- click-to-hide would duplicate that control and, for A/B
    // (which have no checkbox), would let a vector be hidden with no way to
    // bring it back. NoHighlightAxis: suppress ImPlot's default
    // highlight-hovered-legend-entry's-axis behavior (the Y2 ruler lighting
    // up to match the hovered entry's color reads as an unrelated part of the
    // window changing). Location matches ImPlot's own default
    // (ImPlotLocation_NorthWest) -- only the flags are new (#71).
    constexpr ImPlotLegendFlags kLegendFlags =
        ImPlotLegendFlags_NoButtons | ImPlotLegendFlags_NoHighlightAxis;
    ImPlot::SetupLegend(ImPlotLocation_NorthWest, kLegendFlags);
    const AxisHalfRanges half_ranges = axis_half_ranges(static_cast<double>(canvas_size.x),
                                                        static_cast<double>(canvas_size.y), extent);
    // Re-apply every frame (ImPlotCond_Always) so leftover pan/zoom state
    // can never drift the limits away from the caller-supplied extent, even
    // though ImPlotFlags_NoInputs already blocks new pan/zoom input.
    ImPlot::SetupAxesLimits(-half_ranges.x, half_ranges.x, -half_ranges.y, half_ranges.y,
                            ImPlotCond_Always);

    // Secondary vertical axis: a scale ruler on the plot's opposite (right)
    // side, showing the real distance each ring represents. No gridlines (the
    // polar grid already draws the rings) and locked against independent
    // pan/zoom -- it always mirrors the primary y axis' range.
    constexpr ImPlotAxisFlags kRulerFlags =
        ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_Lock;
    ImPlot::SetupAxis(ImAxis_Y2, nullptr, kRulerFlags);
    ImPlot::SetupAxisLimits(ImAxis_Y2, -half_ranges.y, half_ranges.y, ImPlotCond_Always);

    const std::vector<RulerTick> ticks = ruler_ticks(frame.ring_interval(), frame.ring_count());
    // Fixed at implementation time (#63): quarters between each pair of
    // adjacent major rings (and origin-to-first-ring), rendered unlabeled via
    // the same SetupAxisTicks mechanism as the major ticks below (an empty
    // label string), rather than user-configurable.
    constexpr int kMinorSubdivisionsPerRing = 4;
    const std::vector<double> minor_positions =
        minor_ruler_ticks(frame.ring_interval(), frame.ring_count(), kMinorSubdivisionsPerRing);

    std::vector<double> positions;
    std::vector<std::string> label_strings;
    std::vector<const char*> label_pointers;
    positions.reserve(ticks.size() + minor_positions.size());
    label_strings.reserve(ticks.size() + minor_positions.size());
    label_pointers.reserve(ticks.size() + minor_positions.size());
    for (const RulerTick& tick : ticks) {
        positions.push_back(tick.position);
        label_strings.push_back(tick.label);
    }
    for (const double minor_position : minor_positions) {
        positions.push_back(minor_position);
        label_strings.emplace_back();
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
                float thickness, const MarkerColor* line_color) {
    const Point ptail = to_plotted_point(tail, convention);
    const Point phead = to_plotted_point(head, convention);
    const std::string head_id = std::string("##head_") + label;

    if (line_color == nullptr) {
        plot_arrow_shape(label, head_id, ptail, phead, /*line_color=*/nullptr, thickness);
        return;
    }
    const ImVec4 resolved_line_color{line_color->r, line_color->g, line_color->b, line_color->a};
    plot_arrow_shape(label, head_id, ptail, phead, &resolved_line_color, thickness);
}

namespace {

// Tip marker/label styling. Fixed constants -- exact pixel sizes are left to
// implementation time per the spec.
constexpr float kMarkerSize = 6.0F;
constexpr ImVec2 kLabelPixelOffset{8.0F, -8.0F};

void draw_tip_marker(const char* label, Point tip, TipMarkerStyle marker_style,
                     const MarkerColor* marker_color) {
    const std::string id = std::string("##tip_") + label;
    const ImPlotMarker marker =
        (marker_style == TipMarkerStyle::kCrossHair) ? ImPlotMarker_Cross : ImPlotMarker_Circle;
    // Extend the legend hover-bold effect to the tip marker too, which
    // (like the arrowhead in plot_arrow_shape) is a separate `##`-prefixed
    // item with no legend entry of its own -- #71. Matches ImPlot's own
    // internal ITEM_HIGHLIGHT_MARK_SCALE. \p label is the vector's shaft's
    // own legend-entry id (see draw_arrow), so this stays in sync with the
    // shaft's own (automatic) and the arrowhead's (plot_arrow_shape) bolding.
    constexpr float kHoverMarkerScale = 1.25F;
    const float marker_size =
        ImPlot::IsLegendEntryHovered(label) ? kMarkerSize * kHoverMarkerScale : kMarkerSize;
    ImPlotSpec spec{ImPlotProp_Marker, marker, ImPlotProp_MarkerSize, marker_size};
    if (marker_color != nullptr) {
        const ImVec4 color(marker_color->r, marker_color->g, marker_color->b, marker_color->a);
        spec.MarkerFillColor = color;
        spec.MarkerLineColor = color;
    }
    ImPlot::PlotScatter(id.c_str(), &tip.x, &tip.y, 1, spec);
}

// \p marker_color, when non-null, colors the label the same as the tip
// marker/shaft (theme-text default or the hover/drag override -- see \ref
// draw_vector). When null, falls back to whatever ImPlot resolved for the
// item drawn immediately before this call (the arrow head, via
// ImPlot::GetLastItemColor()) rather than any color hardcoded here.
void draw_tip_label(const char* label, Point tip, const MarkerColor* marker_color) {
    const ImVec4 color = (marker_color != nullptr) ? ImVec4(marker_color->r, marker_color->g,
                                                            marker_color->b, marker_color->a)
                                                   : ImPlot::GetLastItemColor();
    ImPlot::Annotation(tip.x, tip.y, color, kLabelPixelOffset, false, "%s", label);
}

}  // namespace

void draw_vector(const char* label, Point head, AngleConvention convention,
                 TipMarkerStyle marker_style, float thickness, const MarkerColor* marker_color,
                 const MarkerColor* line_color) {
    // Draw the tip marker before the arrow shaft/head so it sits behind the
    // arrowhead and peeks out past the tip, instead of being fully covered
    // when the marker is larger than the arrowhead.
    const Point tip = to_plotted_point(head, convention);
    draw_tip_marker(label, tip, marker_style, marker_color);

    draw_arrow(label, Point{0.0, 0.0}, head, convention, thickness, line_color);

    draw_tip_label(label, tip, marker_color);
}

void draw_length_tick(double magnitude, MarkerColor color) {
    // The ruler (Y2) axis' current visible extent -- clamped/ignored against
    // this, not against view_frame's own extent, so the tick always agrees
    // with what's actually on screen right now (see begin_vector_plot: Y2's
    // limits are set from the same PlotFrame that drives the ruler ticks).
    const ImPlotRect limits = ImPlot::GetPlotLimits(ImAxis_X1, ImAxis_Y2);
    if (magnitude > limits.Y.Max) {
        return;
    }

    // A short segment sitting right at the plot's right edge, where the Y2
    // ruler itself is drawn, sized relative to the ruler's own extent so it
    // reads consistently regardless of zoom/manual-scale.
    constexpr double kTickLengthFraction = 0.06;
    constexpr float kTickThickness = 2.0F;
    const double tick_length = limits.Y.Max * kTickLengthFraction;
    const std::array<double, 2> xs{limits.X.Max - tick_length, limits.X.Max};
    const std::array<double, 2> ys{magnitude, magnitude};

    const ImVec4 line_color{color.r, color.g, color.b, color.a};
    const ImPlotSpec spec{ImPlotProp_LineColor, line_color, ImPlotProp_LineWeight, kTickThickness};
    ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
    ImPlot::PlotLine("##length_tick", xs.data(), ys.data(), 2, spec);
}

HoverTarget hover_hit_test(Point mouse, Point tip_a, Point tip_b, double hit_radius) {
    const double dist_a = std::hypot(mouse.x - tip_a.x, mouse.y - tip_a.y);
    const double dist_b = std::hypot(mouse.x - tip_b.x, mouse.y - tip_b.y);
    const bool a_in_range = dist_a <= hit_radius;
    const bool b_in_range = dist_b <= hit_radius;

    if (!a_in_range && !b_in_range) {
        return HoverTarget::kNone;
    }
    if (a_in_range && !b_in_range) {
        return HoverTarget::kA;
    }
    if (b_in_range && !a_in_range) {
        return HoverTarget::kB;
    }
    // Both in range: nearer tip wins; an exact tie favors A.
    return (dist_b < dist_a) ? HoverTarget::kB : HoverTarget::kA;
}

HoverTarget hover_target(Point head_a, Point head_b, AngleConvention convention) {
    // Not ImPlot::IsPlotHovered(): that relies on ImPlot's own per-frame
    // UpdateInput() pass to set plot.Hovered, which is skipped entirely
    // when kPlotFlags (see begin_vector_plot) carries ImPlotFlags_NoInputs
    // (disabling ImPlot's built-in pan/zoom) -- so IsPlotHovered() would
    // always report false here. GetPlotPos()/GetPlotSize() are computed
    // earlier in ImPlot's per-frame setup, before that gate, so a manual
    // rect containment check (mirroring ImPlot's own FrameHovered pattern
    // for subplots) works regardless of NoInputs.
    const ImVec2 plot_min_px = ImPlot::GetPlotPos();
    const ImVec2 plot_size_px = ImPlot::GetPlotSize();
    const ImVec2 mouse_px = ImGui::GetMousePos();
    const bool mouse_in_plot_rect =
        mouse_px.x >= plot_min_px.x && mouse_px.x <= plot_min_px.x + plot_size_px.x &&
        mouse_px.y >= plot_min_px.y && mouse_px.y <= plot_min_px.y + plot_size_px.y;
    const bool plot_hovered =
        mouse_in_plot_rect &&
        ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows |
                               ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    if (!plot_hovered) {
        return HoverTarget::kNone;
    }

    const Point tip_a = to_plotted_point(head_a, convention);
    const Point tip_b = to_plotted_point(head_b, convention);
    const ImVec2 pixel_a = ImPlot::PlotToPixels(tip_a.x, tip_a.y);
    const ImVec2 pixel_b = ImPlot::PlotToPixels(tip_b.x, tip_b.y);
    const ImVec2 mouse = ImGui::GetMousePos();

    const double hit_radius_px = std::max<double>(static_cast<double>(kMarkerSize), 10.0);
    return hover_hit_test(Point{static_cast<double>(mouse.x), static_cast<double>(mouse.y)},
                          Point{static_cast<double>(pixel_a.x), static_cast<double>(pixel_a.y)},
                          Point{static_cast<double>(pixel_b.x), static_cast<double>(pixel_b.y)},
                          hit_radius_px);
}

InteractionState resolve_interaction_state(bool was_dragging, bool is_hover_target,
                                           bool mouse_pressed, bool mouse_down) {
    if (was_dragging) {
        return mouse_down ? InteractionState::kDragging : InteractionState::kReleased;
    }
    if (is_hover_target && mouse_pressed) {
        return InteractionState::kDragging;
    }
    return is_hover_target ? InteractionState::kHovered : InteractionState::kIdle;
}

Point clamp_to_extent(Point p, double extent) {
    return {std::clamp(p.x, -extent, extent), std::clamp(p.y, -extent, extent)};
}

Point clamp_to_rect(Point p, PixelRect rect) {
    return {std::clamp(p.x, rect.min.x, rect.max.x), std::clamp(p.y, rect.min.y, rect.max.y)};
}

InteractiveVectorResult draw_interactive_vector(
    const char* label, Point head, AngleConvention convention, TipMarkerStyle marker_style,
    bool is_hover_target, bool was_dragging, float thickness, bool auto_scale,
    double visible_extent, const MarkerColor* default_marker_color, const MarkerColor* line_color) {
    constexpr ImGuiMouseButton kDragButton = ImGuiMouseButton_Left;
    const bool mouse_down = ImGui::IsMouseDown(kDragButton);
    const bool mouse_pressed = ImGui::IsMouseClicked(kDragButton);
    const InteractionState state =
        resolve_interaction_state(was_dragging, is_hover_target, mouse_pressed, mouse_down);

    Point updated_head = head;
    if (state == InteractionState::kDragging) {
        // Clamp the mouse position to the plot canvas' own pixel-space
        // bounds first (#44/#49): this keeps the drag tracking the cursor
        // even once it strays outside the canvas (e.g. into a side panel or
        // outside the window) while the button is still held, rather than
        // freezing or canceling.
        const ImVec2 plot_min_px = ImPlot::GetPlotPos();
        const ImVec2 plot_size_px = ImPlot::GetPlotSize();
        const ImVec2 mouse_px = ImGui::GetMousePos();
        const PixelRect plot_rect{
            Point{static_cast<double>(plot_min_px.x), static_cast<double>(plot_min_px.y)},
            Point{static_cast<double>(plot_min_px.x + plot_size_px.x),
                  static_cast<double>(plot_min_px.y + plot_size_px.y)}};
        const Point clamped_px = clamp_to_rect(
            Point{static_cast<double>(mouse_px.x), static_cast<double>(mouse_px.y)}, plot_rect);
        const ImPlotPoint mouse_plot = ImPlot::PixelsToPlot(
            ImVec2(static_cast<float>(clamped_px.x), static_cast<float>(clamped_px.y)));

        Point plotted{mouse_plot.x, mouse_plot.y};
        // 15 degree snap increment, in plotted/visual space -- see #50.
        // Applied before the manual-scale clamp below: snapping rotates the
        // point (e.g. toward a square's corner), which can otherwise push it
        // back outside `visible_extent` after an already-clamped point was
        // snapped, so the clamp must run last to stay the binding
        // constraint (#49's "never leave the visible extent" invariant).
        constexpr double kSnapIncrementRadians = kPi / 12.0;
        if (ImGui::GetIO().KeyShift) {
            plotted = snap_angle_to_increment(plotted, kSnapIncrementRadians);
        }
        // Manual-scale clamp (#44/#49): only when auto-scale is off, so the
        // existing auto-fit behavior (extent grows with the vector's
        // magnitude) stays unaffected when auto-scale is on.
        if (!auto_scale) {
            plotted = clamp_to_extent(plotted, visible_extent);
        }
        updated_head = from_plotted_point(plotted, convention);
    }

    const MarkerColor* marker_color = default_marker_color;
    if (state == InteractionState::kDragging) {
        marker_color = &kDraggingMarkerColor;
    } else if (state == InteractionState::kHovered ||
               (state == InteractionState::kReleased && is_hover_target)) {
        marker_color = &kHoverMarkerColor;
    }

    draw_vector(label, updated_head, convention, marker_style, thickness, marker_color, line_color);

    return {updated_head, state};
}

void draw_annotation_vector(const char* id, AnnotationVector annotation, AngleConvention convention,
                            float thickness) {
    // Muted, semi-transparent gray -- distinct from named vectors, which cycle
    // through ImPlot's saturated default colormap.
    constexpr ImVec4 kAnnotationColor{0.55F, 0.55F, 0.55F, 0.65F};

    const Point tail = annotation.start;
    const Point head{tail.x + annotation.vector.x, tail.y + annotation.vector.y};
    const Point ptail = to_plotted_point(tail, convention);
    const Point phead = to_plotted_point(head, convention);

    const std::string shaft_id = std::string("##annotation_") + id;
    const std::string head_id = std::string("##annotation_head_") + id;
    plot_arrow_shape(shaft_id, head_id, ptail, phead, &kAnnotationColor, thickness);
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

RotationIndicatorLabel rotation_indicator_label(double extent, double sweep_sign) {
    const RotationIndicatorArc arc = rotation_indicator_arc(sweep_sign);
    const double mid_angle = (arc.from_angle + arc.to_angle) / 2.0;
    const double label_radius = extent * kLabelRadiusFactor;
    const Point position{label_radius * std::cos(mid_angle), label_radius * std::sin(mid_angle)};
    return {position, "Rot."};
}

void draw_rotation_indicator(PlotFrame frame, double sweep_sign, ArcStyle style) {
    const RotationIndicatorArc arc = rotation_indicator_arc(sweep_sign);
    draw_arc_with_head(frame.extent(), arc.from_angle, arc.to_angle, style, "##rotation_indicator",
                       "##rotation_indicator_head");

    const RotationIndicatorLabel label = rotation_indicator_label(frame.extent(), sweep_sign);
    const ImVec4 color{style.r, style.g, style.b, style.a};
    ImPlot::Annotation(label.position.x, label.position.y, color, ImVec2(0.0F, 0.0F), false, "%s",
                       label.text.c_str());
}

}  // namespace polarplot
