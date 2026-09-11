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
        plot_closed_path(id.c_str(), cx.data(), cy.data(), kSegments);
    }

    for (int s = 0; s < spokes; ++s) {
        const double t = kTwoPi * static_cast<double>(s) / spokes;
        const double plotted = apply_angle_convention(t, convention);
        const std::array<double, 2> sx{0.0, max_radius * std::cos(plotted)};
        const std::array<double, 2> sy{0.0, max_radius * std::sin(plotted)};
        const std::string id = "##spoke" + std::to_string(s);
        ImPlot::PlotLine(id.c_str(), sx.data(), sy.data(), 2);
    }
}

void draw_arrow(const char* label, Point tail, Point head, AngleConvention convention,
                double head_frac) {
    const Point ptail = to_plotted_point(tail, convention);
    const Point phead = to_plotted_point(head, convention);

    const std::array<double, 2> sx{ptail.x, phead.x};
    const std::array<double, 2> sy{ptail.y, phead.y};
    ImPlot::PlotLine(label, sx.data(), sy.data(), 2);

    const double dx = phead.x - ptail.x;
    const double dy = phead.y - ptail.y;
    const double len = std::hypot(dx, dy);
    if (len == 0.0) {
        return;
    }

    const double ux = dx / len;
    const double uy = dy / len;
    const double h = len * head_frac;
    constexpr double kWing = 0.4;  // half-width of the head as a fraction of h

    const double back_x = phead.x - (h * ux);
    const double back_y = phead.y - (h * uy);
    const double wing_x = kWing * h * uy;
    const double wing_y = kWing * h * ux;

    const std::array<double, 3> hx{back_x + wing_x, phead.x, back_x - wing_x};
    const std::array<double, 3> hy{back_y - wing_y, phead.y, back_y + wing_y};
    const std::string id = std::string("##head_") + label;
    ImPlot::PlotLine(id.c_str(), hx.data(), hy.data(), 3);
}

void draw_vector(const char* label, Point head, AngleConvention convention, double head_frac) {
    draw_arrow(label, Point{0.0, 0.0}, head, convention, head_frac);
}

}  // namespace polarplot
