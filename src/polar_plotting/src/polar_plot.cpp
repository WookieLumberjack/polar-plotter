#include "polar_plotting/polar_plot.hpp"

#include <array>
#include <cmath>
#include <numbers>
#include <string>

#include <implot.h>

namespace polarplot {
namespace {

constexpr double kTwoPi = 2.0 * std::numbers::pi;

// Build the ImPlotSpec used for an arrow's shaft/head: \p line_color when
// non-null, otherwise ImPlot's default per-item color cycling.
ImPlotSpec arrow_line_spec(const ImVec4* line_color) {
    ImPlotSpec spec;
    if (line_color != nullptr) {
        spec.LineColor = *line_color;
    }
    return spec;
}

// Draw an arrow's shaft (\p shaft_id) and head (\p head_id) from \p tail to
// \p head (already in plotted/drawing coordinates -- callers remap via
// \ref to_plotted_point first), styled with \p line_color (nullptr for
// ImPlot's default color cycling).
void plot_arrow_shape(const std::string& shaft_id, const std::string& head_id, Point tail,
                      Point head, double head_frac, const ImVec4* line_color) {
    const std::array<double, 2> sx{tail.x, head.x};
    const std::array<double, 2> sy{tail.y, head.y};
    ImPlot::PlotLine(shaft_id.c_str(), sx.data(), sy.data(), 2, arrow_line_spec(line_color));

    const double dx = head.x - tail.x;
    const double dy = head.y - tail.y;
    const double len = std::hypot(dx, dy);
    if (len == 0.0) {
        return;
    }

    const double ux = dx / len;
    const double uy = dy / len;
    const double h = len * head_frac;
    constexpr double kWing = 0.4;  // half-width of the head as a fraction of h

    const double back_x = head.x - (h * ux);
    const double back_y = head.y - (h * uy);
    const double wing_x = kWing * h * uy;
    const double wing_y = kWing * h * ux;

    const std::array<double, 3> hx{back_x + wing_x, head.x, back_x - wing_x};
    const std::array<double, 3> hy{back_y - wing_y, head.y, back_y + wing_y};

    ImPlot::PlotLine(head_id.c_str(), hx.data(), hy.data(), 3, arrow_line_spec(line_color));
}

}  // namespace

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

bool begin_vector_plot(const char* title, double extent) {
    if (!ImPlot::BeginPlot(title, ImVec2(-1, -1), ImPlotFlags_Equal)) {
        return false;
    }
    ImPlot::SetupAxes("x", "y");
    ImPlot::SetupAxesLimits(-extent, extent, -extent, extent, ImPlotCond_Once);
    return true;
}

void end_vector_plot() { ImPlot::EndPlot(); }

void draw_polar_grid(double max_radius, AngleConvention convention, int rings, int spokes) {
    // One muted, fixed color for the whole grid (no per-ring colormap
    // cycling); the outermost ring is drawn heavier than the interior rings
    // and spokes so the plot's boundary reads clearly without a bounding box.
    constexpr ImVec4 kGridColor{0.5F, 0.5F, 0.5F, 0.5F};
    constexpr float kInteriorWeight = 1.0F;
    constexpr float kOuterWeight = 2.5F;

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
}

void draw_arrow(const char* label, Point tail, Point head, AngleConvention convention,
                double head_frac) {
    const Point ptail = to_plotted_point(tail, convention);
    const Point phead = to_plotted_point(head, convention);
    const std::string head_id = std::string("##head_") + label;
    plot_arrow_shape(label, head_id, ptail, phead, head_frac, /*line_color=*/nullptr);
}

void draw_vector(const char* label, Point head, AngleConvention convention, double head_frac) {
    draw_arrow(label, Point{0.0, 0.0}, head, convention, head_frac);
}

void draw_annotation_vector(const char* id, AnnotationVector annotation, AngleConvention convention,
                            double head_frac) {
    // Muted, semi-transparent gray -- distinct from named vectors, which cycle
    // through ImPlot's saturated default colormap.
    constexpr ImVec4 kAnnotationColor{0.55F, 0.55F, 0.55F, 0.65F};

    const Point tail = annotation.start;
    const Point head{tail.x + annotation.vector.x, tail.y + annotation.vector.y};
    const Point ptail = to_plotted_point(tail, convention);
    const Point phead = to_plotted_point(head, convention);

    const std::string shaft_id = std::string("##annotation_") + id;
    const std::string head_id = std::string("##annotation_head_") + id;
    plot_arrow_shape(shaft_id, head_id, ptail, phead, head_frac, &kAnnotationColor);
}

}  // namespace polarplot
