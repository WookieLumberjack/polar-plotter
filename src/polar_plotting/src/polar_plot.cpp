#include "polar_plotting/polar_plot.hpp"

#include <array>
#include <cmath>
#include <numbers>
#include <string>

#include <implot.h>

namespace polarplot {
namespace {

constexpr double kTwoPi = 2.0 * std::numbers::pi;

// Draw a closed poly-line through the given data points.
void plot_closed_path(const char* id, const double* xs, const double* ys, int n) {
    ImPlot::PlotLine(id, xs, ys, n, {ImPlotProp_Flags, ImPlotLineFlags_Loop});
}

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
// \p head, styled with \p line_color (nullptr for ImPlot's default color
// cycling).
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

bool begin_vector_plot(const char* title, double extent) {
    if (!ImPlot::BeginPlot(title, ImVec2(-1, -1), ImPlotFlags_Equal)) {
        return false;
    }
    ImPlot::SetupAxes("x", "y");
    ImPlot::SetupAxesLimits(-extent, extent, -extent, extent, ImPlotCond_Once);
    return true;
}

void end_vector_plot() { ImPlot::EndPlot(); }

void draw_polar_grid(double max_radius, int rings, int spokes) {
    constexpr int kSegments = 96;
    std::array<double, kSegments> cx{};
    std::array<double, kSegments> cy{};

    for (int r = 1; r <= rings; ++r) {
        const double radius = max_radius * static_cast<double>(r) / rings;
        for (int i = 0; i < kSegments; ++i) {
            const double t = kTwoPi * static_cast<double>(i) / kSegments;
            cx[static_cast<std::size_t>(i)] = radius * std::cos(t);
            cy[static_cast<std::size_t>(i)] = radius * std::sin(t);
        }
        const std::string id = "##ring" + std::to_string(r);
        plot_closed_path(id.c_str(), cx.data(), cy.data(), kSegments);
    }

    for (int s = 0; s < spokes; ++s) {
        const double t = kTwoPi * static_cast<double>(s) / spokes;
        const std::array<double, 2> sx{0.0, max_radius * std::cos(t)};
        const std::array<double, 2> sy{0.0, max_radius * std::sin(t)};
        const std::string id = "##spoke" + std::to_string(s);
        ImPlot::PlotLine(id.c_str(), sx.data(), sy.data(), 2);
    }
}

void draw_arrow(const char* label, Point tail, Point head, double head_frac) {
    const std::string head_id = std::string("##head_") + label;
    plot_arrow_shape(label, head_id, tail, head, head_frac, /*line_color=*/nullptr);
}

void draw_vector(const char* label, Point head, double head_frac) {
    draw_arrow(label, Point{0.0, 0.0}, head, head_frac);
}

void draw_annotation_vector(const char* id, AnnotationVector annotation, double head_frac) {
    // Muted, semi-transparent gray -- distinct from named vectors, which cycle
    // through ImPlot's saturated default colormap.
    constexpr ImVec4 kAnnotationColor{0.55F, 0.55F, 0.55F, 0.65F};

    const Point tail = annotation.start;
    const Point head{tail.x + annotation.vector.x, tail.y + annotation.vector.y};

    const std::string shaft_id = std::string("##annotation_") + id;
    const std::string head_id = std::string("##annotation_head_") + id;
    plot_arrow_shape(shaft_id, head_id, tail, head, head_frac, &kAnnotationColor);
}

}  // namespace polarplot
