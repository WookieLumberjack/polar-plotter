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
    ImPlot::PlotLine(id, xs, ys, n, ImPlotLineFlags_Loop);
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
    const std::array<double, 2> sx{tail.x, head.x};
    const std::array<double, 2> sy{tail.y, head.y};
    ImPlot::PlotLine(label, sx.data(), sy.data(), 2);

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
    const std::string id = std::string("##head_") + label;
    ImPlot::PlotLine(id.c_str(), hx.data(), hy.data(), 3);
}

void draw_vector(const char* label, Point head, double head_frac) {
    draw_arrow(label, Point{0.0, 0.0}, head, head_frac);
}

}  // namespace polarplot
