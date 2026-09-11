#include "ui/app.hpp"

#include <cmath>
#include <numbers>
#include <utility>

#include <imgui.h>
#include <implot.h>

#include "polar_plotting/polar_plot.hpp"
#include "ui/config.hpp"
#include "vector_math/vec2.hpp"

namespace ui {
namespace {

vecmath::Vec2 to_vec(const std::array<float, 2>& xy) {
    return {static_cast<double>(xy[0]), static_cast<double>(xy[1])};
}

polarplot::Point to_point(vecmath::Vec2 v) { return {v.x, v.y}; }

constexpr double kRadToDeg = 180.0 / std::numbers::pi;
constexpr double kDegToRad = std::numbers::pi / 180.0;

// angle_sign stays fixed at today's default (+1, counterclockwise) until the
// rotation-direction / measurement-convention toggles land in a later ticket.
constexpr double kDefaultAngleSign = 1.0;

}  // namespace

App::App() = default;

App::App(std::filesystem::path config_path) : config_path_(std::move(config_path)) {
    if (auto cfg = load_config(config_path_)) {
        a_.xy = cfg->a;
        b_.xy = cfg->b;
        show_sum_ = cfg->show_sum;
        show_difference_ = cfg->show_difference;
    }
}

App::~App() { save(); }

void App::save() const {
    if (config_path_.empty()) {
        return;
    }
    const Config cfg{a_.xy, b_.xy, show_sum_, show_difference_};
    (void)save_config(config_path_, cfg);
}

void App::render() {
    // A simple side-by-side default layout; the user can move/resize freely and
    // ImGui remembers it in imgui.ini afterwards.
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const float pad = 16.0F;
    const float controls_w = 380.0F;

    ImGui::SetNextWindowPos({vp->WorkPos.x + pad, vp->WorkPos.y + pad}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({controls_w, vp->WorkSize.y - (2 * pad)}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Vectors");
    draw_controls();
    ImGui::End();

    ImGui::SetNextWindowPos({vp->WorkPos.x + controls_w + (2 * pad), vp->WorkPos.y + pad},
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({vp->WorkSize.x - controls_w - (3 * pad), vp->WorkSize.y - (2 * pad)},
                             ImGuiCond_FirstUseEver);
    ImGui::Begin("Polar plot");
    draw_plot();
    ImGui::End();
}

void App::draw_controls() {
    ImGui::TextUnformatted("Enter two vectors in Cartesian components.");
    ImGui::Spacing();

    ImGui::InputFloat2("A (x, y)", a_.xy.data(), "%.3f");
    ImGui::InputFloat2("B (x, y)", b_.xy.data(), "%.3f");

    ImGui::Spacing();
    ImGui::InputFloat("Zero direction (deg)", &zero_direction_deg_, 1.0F, 10.0F, "%.2f");

    ImGui::Spacing();
    ImGui::Checkbox("Show A + B", &show_sum_);
    ImGui::SameLine();
    ImGui::Checkbox("Show A - B", &show_difference_);

    const vecmath::Vec2 a = to_vec(a_.xy);
    const vecmath::Vec2 b = to_vec(b_.xy);
    const vecmath::Polar pa = vecmath::to_polar(a);
    const vecmath::Polar pb = vecmath::to_polar(b);

    ImGui::SeparatorText("Derived quantities");
    ImGui::Text("|A| = %.4f   arg A = %.2f deg", pa.radius, pa.angle_rad * kRadToDeg);
    ImGui::Text("|B| = %.4f   arg B = %.2f deg", pb.radius, pb.angle_rad * kRadToDeg);
    ImGui::Text("A - B = (%.4f, %.4f)   |A - B| = %.4f", (a - b).x, (a - b).y,
                vecmath::magnitude(a - b));
    ImGui::Text("A + B = (%.4f, %.4f)", (a + b).x, (a + b).y);
    ImGui::Text("A . B = %.4f", vecmath::dot(a, b));
    ImGui::Text("angle(A, B) = %.2f deg", vecmath::angle_between(a, b) * kRadToDeg);
}

void App::draw_plot() const {
    const vecmath::Vec2 a = to_vec(a_.xy);
    const vecmath::Vec2 b = to_vec(b_.xy);
    const vecmath::Vec2 diff = a - b;
    const vecmath::Vec2 sum = a + b;

    double extent = 1.0;
    for (const vecmath::Vec2 v : {a, b, diff, sum}) {
        extent = std::max(extent, vecmath::magnitude(v));
    }
    extent *= 1.2;

    const polarplot::AngleConvention convention{
        .zero_direction = static_cast<double>(zero_direction_deg_) * kDegToRad,
        .angle_sign = kDefaultAngleSign,
    };

    if (!polarplot::begin_vector_plot("##polar", extent)) {
        return;
    }
    polarplot::draw_polar_grid(extent, convention);
    polarplot::draw_vector("A", to_point(a), convention);
    polarplot::draw_vector("B", to_point(b), convention);
    if (show_difference_) {
        polarplot::draw_vector("A - B", to_point(diff), convention);
    }
    if (show_sum_) {
        polarplot::draw_vector("A + B", to_point(sum), convention);
    }
    polarplot::end_vector_plot();
}

}  // namespace ui
