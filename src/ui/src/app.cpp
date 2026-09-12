#include "ui/app.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <string>
#include <utility>

#include <imgui.h>
#include <implot.h>

#include "polar_plotting/polar_plot.hpp"
#include "ui/config.hpp"
#include "ui/plot_plan.hpp"
#include "ui/zero_direction.hpp"
#include "vector_math/vec2.hpp"

namespace ui {
namespace {

vecmath::Vec2 to_vec(const std::array<float, 2>& xy) {
    return {static_cast<double>(xy[0]), static_cast<double>(xy[1])};
}

constexpr double kRadToDeg = 180.0 / std::numbers::pi;

// Draw a pair of mutually-exclusive radio buttons on the same line (\p
// label_a first, then \p label_b) and return the updated "is \p label_a
// selected" state. Caller supplies the current state via \p is_a; widget IDs
// are exactly \p label_a / \p label_b, so behavior/labels are unchanged from
// writing the pair out longhand.
bool draw_binary_radio(const char* label_a, const char* label_b, bool is_a) {
    if (ImGui::RadioButton(label_a, is_a)) {
        is_a = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton(label_b, !is_a)) {
        is_a = false;
    }
    return is_a;
}

}  // namespace

App::App() = default;

App::App(std::filesystem::path config_path) : config_path_(std::move(config_path)) {
    if (auto cfg = load_config(config_path_)) {
        a_.xy = cfg->a;
        b_.xy = cfg->b;
        show_sum_ = cfg->show_sum;
        show_difference_ = cfg->show_difference;
        line_width_ = cfg->line_width;
        auto_scale_ = cfg->auto_scale;
        manual_ring_interval_ = cfg->manual_ring_interval;
    }
}

App::~App() { save(); }

void App::save() const {
    if (config_path_.empty()) {
        return;
    }
    const Config cfg{
        a_.xy, b_.xy, show_sum_, show_difference_, line_width_, auto_scale_, manual_ring_interval_};
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

    ImGui::Checkbox("Keep zero-direction arc visible", &show_zero_direction_arc_persistent_);

    ImGui::Spacing();
    ImGui::TextUnformatted("Rotation direction");
    ImGui::SameLine();
    const bool rotation_is_ccw =
        draw_binary_radio("CCW##rotation_direction", "CW##rotation_direction",
                          rotation_direction_ == RotationDirection::CounterClockwise);
    rotation_direction_ =
        rotation_is_ccw ? RotationDirection::CounterClockwise : RotationDirection::Clockwise;

    ImGui::TextUnformatted("Measurement convention");
    ImGui::SameLine();
    const bool measurement_is_with = draw_binary_radio(
        "With rotation##measurement_convention", "Against rotation##measurement_convention",
        measurement_convention_ == MeasurementConvention::WithRotation);
    measurement_convention_ = measurement_is_with ? MeasurementConvention::WithRotation
                                                  : MeasurementConvention::AgainstRotation;

    ImGui::Spacing();
    ImGui::Checkbox("Show A + B", &show_sum_);
    ImGui::SameLine();
    ImGui::Checkbox("Show A - B", &show_difference_);
    ImGui::Checkbox("Show tip-to-tail construction", &show_tip_to_tail_);
    ImGui::Checkbox("Show difference segment", &show_difference_segment_);

    ImGui::Spacing();
    ImGui::TextUnformatted("Tip marker style");
    ImGui::SameLine();
    const bool is_dot =
        draw_binary_radio("Dot", "Cross-hair", marker_style_ == polarplot::TipMarkerStyle::kDot);
    marker_style_ =
        is_dot ? polarplot::TipMarkerStyle::kDot : polarplot::TipMarkerStyle::kCrossHair;

    ImGui::SliderFloat("Line width", &line_width_, 1.0F, 6.0F, "%.1f");

    ImGui::Spacing();
    ImGui::Checkbox("Auto-scale", &auto_scale_);
    if (!auto_scale_) {
        // Logarithmic: the interval spans three decades (0.1 to 100), and a
        // plain linear slider would leave the bottom of that range
        // (differences of a few hundredths) unreachable with any usable
        // precision.
        ImGui::SliderFloat("Ring interval", &manual_ring_interval_, 0.1F, 100.0F, "%.3f",
                           ImGuiSliderFlags_Logarithmic);
    }

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

    const PlotPlan plan = plan_plot(PlotInputs{
        .a = a,
        .b = b,
        .show_sum = show_sum_,
        .show_difference = show_difference_,
        .show_tip_to_tail = show_tip_to_tail_,
        .show_difference_segment = show_difference_segment_,
        .marker_style = marker_style_,
        .zero_direction_deg = static_cast<double>(zero_direction_deg_),
        .rotation_direction = rotation_direction_,
        .measurement_convention = measurement_convention_,
        .zero_direction_input_focused = zero_direction_input_focused_,
        .show_zero_direction_arc_persistent = show_zero_direction_arc_persistent_,
        .auto_scale = auto_scale_,
        .manual_ring_interval = static_cast<double>(manual_ring_interval_),
    });

    // The grid's outer ring is drawn at exactly `plan.extent.extent` -- the
    // same PlotFrame handed to begin_vector_plot and draw_rotation_indicator
    // -- so the scale ruler's tick positions always land exactly on the
    // rings they label. begin_vector_plot inflates its own axis view a bit
    // beyond that extent internally (rather than shrinking the grid inside
    // an unchanged view, which would move the rings off the ruler's ticks)
    // so the grid's spoke degree labels, drawn just outside the outer ring,
    // have room without getting clipped.
    const double extent = plan.extent.extent;

    if (!polarplot::begin_vector_plot("##polar", plan.extent)) {
        return;
    }
    polarplot::draw_polar_grid(plan.extent, plan.convention);
    polarplot::draw_vector("A", plan.a, plan.convention, marker_style_, /*head_frac=*/0.12,
                           line_width_);
    polarplot::draw_vector("B", plan.b, plan.convention, marker_style_, /*head_frac=*/0.12,
                           line_width_);
    if (plan.difference) {
        polarplot::draw_vector("A - B", *plan.difference, plan.convention, marker_style_,
                               /*head_frac=*/0.12, line_width_);
    }
    if (plan.sum) {
        polarplot::draw_vector("A + B", *plan.sum, plan.convention, marker_style_,
                               /*head_frac=*/0.12, line_width_);
    }
    // Positional ids: fine because draw_annotation_vector's id is never shown
    // (see polar_plot.hpp), only needs to be unique per frame, and
    // PlotPlan::tip_to_tail_annotations' push order is a documented contract
    // (see plot_plan.cpp) -- not derived from branching here.
    for (std::size_t i = 0; i < plan.tip_to_tail_annotations.size(); ++i) {
        const std::string id = "tip_to_tail_" + std::to_string(i);
        polarplot::draw_annotation_vector(id.c_str(), plan.tip_to_tail_annotations[i],
                                          plan.convention, /*head_frac=*/0.12, line_width_);
    }
    if (plan.difference_segment) {
        polarplot::draw_annotation_vector("diff_segment_b_to_a_tip", *plan.difference_segment,
                                          plan.convention, /*head_frac=*/0.12, line_width_);
    }
    if (plan.zero_direction_arc_angle) {
        polarplot::ArcStyle arc_style{};
        arc_style.thickness = line_width_;
        polarplot::draw_angle_arc(extent * 0.85, *plan.zero_direction_arc_angle, arc_style);
    }
    // Always drawn, right on the outer ring (as opposed to the zero-direction
    // arc's slightly inset radius above) and centered on the plot's
    // 3-o'clock reference rather than 12-o'clock, so it never visually
    // overlaps that arc; distinct color reinforces the two are unrelated.
    polarplot::ArcStyle rotation_indicator_style{};
    rotation_indicator_style.r = 0.2F;
    rotation_indicator_style.g = 0.6F;
    rotation_indicator_style.b = 1.0F;
    rotation_indicator_style.a = 1.0F;
    rotation_indicator_style.thickness = line_width_;
    polarplot::draw_rotation_indicator(plan.extent, plan.rotation_indicator_sweep_sign,
                                       rotation_indicator_style);
    polarplot::end_vector_plot();
}

}  // namespace ui
