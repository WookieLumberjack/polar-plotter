#include "ui/app.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

#include <imgui.h>
#include <implot.h>

#include "polar_plotting/polar_plot.hpp"
#include "ui/config.hpp"
#include "ui/construction.hpp"
#include "ui/zero_direction.hpp"
#include "vector_math/vec2.hpp"

namespace ui {
namespace {

vecmath::Vec2 to_vec(const std::array<float, 2>& xy) {
    return {static_cast<double>(xy[0]), static_cast<double>(xy[1])};
}

polarplot::Point to_point(vecmath::Vec2 v) { return {v.x, v.y}; }

polarplot::AnnotationVector to_annotation(const ConstructionVector& construction) {
    return {to_point(construction.start), to_point(construction.vector)};
}

constexpr double kRadToDeg = 180.0 / std::numbers::pi;
constexpr double kDegToRad = std::numbers::pi / 180.0;

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
    bool zero_direction_focused = ImGui::IsItemFocused();

    PlainZeroDirection plain = raw_to_plain_zero_direction(zero_direction_deg_);
    int side_index = plain.side == ZeroDirectionSide::kLeft ? 0 : 1;
    bool plain_changed = false;

    ImGui::PushID("zero_direction_plain");
    ImGui::TextUnformatted("Zero direction, plain language:");
    plain_changed |= ImGui::Combo("Side", &side_index, "left\0right\0\0");
    zero_direction_focused |= ImGui::IsItemFocused();
    plain_changed |= ImGui::InputFloat("deg of top", &plain.degrees_from_top, 1.0F, 10.0F, "%.2f");
    zero_direction_focused |= ImGui::IsItemFocused();
    ImGui::PopID();

    if (plain_changed) {
        plain.side = side_index == 0 ? ZeroDirectionSide::kLeft : ZeroDirectionSide::kRight;
        plain.degrees_from_top = std::clamp(plain.degrees_from_top, 0.0F, 180.0F);
        zero_direction_deg_ = plain_to_raw_zero_direction(plain);
    }

    zero_direction_input_focused_ = zero_direction_focused;

    ImGui::Spacing();
    ImGui::TextUnformatted("Rotation direction");
    ImGui::SameLine();
    const bool rotation_is_ccw = rotation_direction_ == RotationDirection::CounterClockwise;
    if (ImGui::RadioButton("CCW##rotation_direction", rotation_is_ccw)) {
        rotation_direction_ = RotationDirection::CounterClockwise;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("CW##rotation_direction", !rotation_is_ccw)) {
        rotation_direction_ = RotationDirection::Clockwise;
    }

    ImGui::TextUnformatted("Measurement convention");
    ImGui::SameLine();
    const bool measurement_is_with = measurement_convention_ == MeasurementConvention::WithRotation;
    if (ImGui::RadioButton("With rotation##measurement_convention", measurement_is_with)) {
        measurement_convention_ = MeasurementConvention::WithRotation;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Against rotation##measurement_convention", !measurement_is_with)) {
        measurement_convention_ = MeasurementConvention::AgainstRotation;
    }

    ImGui::Spacing();
    ImGui::Checkbox("Show A + B", &show_sum_);
    ImGui::SameLine();
    ImGui::Checkbox("Show A - B", &show_difference_);
    ImGui::Checkbox("Show tip-to-tail construction", &show_tip_to_tail_);

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
        .angle_sign = compose_angle_sign(rotation_direction_, measurement_convention_),
    };

    if (!polarplot::begin_vector_plot("##polar", extent)) {
        return;
    }
    polarplot::draw_polar_grid(extent, convention);
    polarplot::draw_vector("A", to_point(a), convention);
    polarplot::draw_vector("B", to_point(b), convention);
    if (show_difference_) {
        polarplot::draw_vector("A - B", to_point(diff), convention);
        if (show_tip_to_tail_) {
            const ConstructionVector construction = tip_to_tail_difference(a, b);
            polarplot::draw_annotation_vector("diff_neg_b_from_a_tip", to_annotation(construction),
                                              convention);
        }
    }
    if (show_sum_) {
        polarplot::draw_vector("A + B", to_point(sum), convention);
        if (show_tip_to_tail_) {
            const SumConstruction construction = tip_to_tail_sum(a, b);
            polarplot::draw_annotation_vector("sum_b_from_a_tip",
                                              to_annotation(construction.b_from_a_tip), convention);
            polarplot::draw_annotation_vector("sum_a_from_b_tip",
                                              to_annotation(construction.a_from_b_tip), convention);
        }
    }
    if (zero_direction_input_focused_) {
        polarplot::draw_angle_arc(extent * 0.85, convention.zero_direction);
    }
    polarplot::end_vector_plot();
}

}  // namespace ui
