#include "ui/app.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <optional>
#include <string>
#include <utility>

#include <imgui.h>
#include <implot.h>

#include "polar_plotting/polar_plot.hpp"
#include "ui/config.hpp"
#include "ui/derived_vectors.hpp"
#include "ui/plot_plan.hpp"
#include "ui/polar_display.hpp"
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

void App::draw_vector_input(const char* label_prefix, VectorInput& input) {
    PolarDisplay display =
        input.polar_active_ ? input.pending_polar_ : to_polar_display(to_vec(input.xy));

    const std::string amplitude_label = std::string(label_prefix) + ": Amplitude";
    const std::string phase_label = std::string(label_prefix) + ": Phase (deg)";
    const std::string real_label = std::string(label_prefix) + ": Real";
    const std::string imag_label = std::string(label_prefix) + ": Imag";

    const bool amp_changed =
        ImGui::InputFloat(amplitude_label.c_str(), &display.amplitude, 0.0F, 0.0F, "%.3f");
    const bool amp_focused = ImGui::IsItemFocused();
    const bool phase_changed =
        ImGui::InputFloat(phase_label.c_str(), &display.phase_deg, 0.0F, 0.0F, "%.2f");
    const bool phase_focused = ImGui::IsItemFocused();
    const bool real_changed =
        ImGui::InputFloat(real_label.c_str(), input.xy.data(), 0.0F, 0.0F, "%.3f");
    const bool imag_changed =
        ImGui::InputFloat(imag_label.c_str(), input.xy.data() + 1, 0.0F, 0.0F, "%.3f");

    const bool polar_focused_now = amp_focused || phase_focused;

    if (real_changed || imag_changed) {
        input.polar_active_ = false;
        return;
    }
    if (amp_changed || phase_changed || polar_focused_now) {
        input.pending_polar_ = display;
        const vecmath::Vec2 v = from_polar_display(display);
        input.xy = {static_cast<float>(v.x), static_cast<float>(v.y)};
        input.polar_active_ = true;
        return;
    }
    if (input.polar_active_) {
        const PolarDisplay canonical = canonicalize_polar_display(input.pending_polar_);
        const vecmath::Vec2 v = from_polar_display(canonical);
        input.xy = {static_cast<float>(v.x), static_cast<float>(v.y)};
        input.polar_active_ = false;
    }
}

void App::draw_derived_vectors_table(const DerivedVectors& derived) {
    struct Row {
        const char* name{nullptr};
        std::optional<vecmath::Vec2> vector;
    };
    const std::array<Row, 6> rows{{
        {"A + B", derived.sum},
        {"A - B", derived.difference_ab},
        {"B - A", derived.difference_ba},
        {"A x B", derived.product},
        {"A / B", derived.quotient_ab},
        {"B / A", derived.quotient_ba},
    }};

    if (!ImGui::BeginTable("derived_vectors", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        return;
    }
    ImGui::TableSetupColumn("Vector");
    ImGui::TableSetupColumn("Amplitude");
    ImGui::TableSetupColumn("Phase (deg)");
    ImGui::TableSetupColumn("Real");
    ImGui::TableSetupColumn("Imag");
    ImGui::TableHeadersRow();

    for (const Row& row : rows) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(row.name);
        if (row.vector) {
            const PolarDisplay display = to_polar_display(*row.vector);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f", static_cast<double>(display.amplitude));
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.2f", static_cast<double>(display.phase_deg));
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.3f", row.vector->x);
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%.3f", row.vector->y);
        } else {
            for (int col = 1; col <= 4; ++col) {
                ImGui::TableSetColumnIndex(col);
                ImGui::TextUnformatted("--");
            }
        }
    }
    ImGui::EndTable();
}

App::App() = default;

App::App(std::filesystem::path config_path) : config_path_(std::move(config_path)) {
    if (auto cfg = load_config(config_path_)) {
        a_.xy = cfg->a;
        b_.xy = cfg->b;
        show_sum_ = cfg->show_sum;
        show_difference_ = cfg->show_difference;
        show_difference_ba_ = cfg->show_difference_ba;
        show_product_ = cfg->show_product;
        show_quotient_ab_ = cfg->show_quotient_ab;
        show_quotient_ba_ = cfg->show_quotient_ba;
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
        .a = a_.xy,
        .b = b_.xy,
        .show_sum = show_sum_,
        .show_difference = show_difference_,
        .show_difference_ba = show_difference_ba_,
        .show_product = show_product_,
        .show_quotient_ab = show_quotient_ab_,
        .show_quotient_ba = show_quotient_ba_,
        .line_width = line_width_,
        .auto_scale = auto_scale_,
        .manual_ring_interval = manual_ring_interval_,
    };
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
    ImGui::TextUnformatted("Enter two vectors as Amplitude/Phase or Real/Imag.");
    ImGui::Spacing();

    ImGui::SeparatorText("Vector A");
    draw_vector_input("A", a_);
    ImGui::SeparatorText("Vector B");
    draw_vector_input("B", b_);

    const vecmath::Vec2 a = to_vec(a_.xy);
    const vecmath::Vec2 b = to_vec(b_.xy);

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
    ImGui::Checkbox("Show B - A", &show_difference_ba_);
    ImGui::SameLine();
    ImGui::Checkbox("Show A x B", &show_product_);

    const DerivedVectors derived = compute_derived_vectors(a, b);

    ImGui::BeginDisabled(!derived.quotient_ab.has_value());
    ImGui::Checkbox("Show A / B", &show_quotient_ab_);
    ImGui::EndDisabled();
    if (!derived.quotient_ab) {
        show_quotient_ab_ = false;
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!derived.quotient_ba.has_value());
    ImGui::Checkbox("Show B / A", &show_quotient_ba_);
    ImGui::EndDisabled();
    if (!derived.quotient_ba) {
        show_quotient_ba_ = false;
    }

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

    ImGui::SeparatorText("Derived vectors");
    draw_derived_vectors_table(derived);
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
        .show_difference_ba = show_difference_ba_,
        .show_product = show_product_,
        .show_quotient_ab = show_quotient_ab_,
        .show_quotient_ba = show_quotient_ba_,
        .marker_style = marker_style_,
        .zero_direction_deg = static_cast<double>(zero_direction_deg_),
        .rotation_direction = rotation_direction_,
        .measurement_convention = measurement_convention_,
        .zero_direction_input_focused = zero_direction_input_focused_,
        .show_zero_direction_arc_persistent = show_zero_direction_arc_persistent_,
        .auto_scale = auto_scale_,
        .manual_ring_interval = static_cast<double>(manual_ring_interval_),
    });

    // The grid's outer ring is drawn at exactly `plan.extent.extent()` -- the
    // same PlotFrame handed to begin_vector_plot and draw_rotation_indicator
    // -- so the scale ruler's tick positions always land exactly on the
    // rings they label (PlotFrame's constructor keeps the two in sync).
    // begin_vector_plot inflates its own axis view a bit beyond that extent
    // internally (rather than shrinking the grid inside an unchanged view,
    // which would move the rings off the ruler's ticks) so the grid's spoke
    // degree labels, drawn just outside the outer ring, have room without
    // getting clipped.
    const double extent = plan.extent.extent();

    if (!polarplot::begin_vector_plot("##polar", plan.extent)) {
        return;
    }
    polarplot::draw_polar_grid(plan.extent, plan.convention);

    // Hover-highlight cue for A/B (see #44/#47): whichever tip is under the
    // cursor gets its marker recolored; no drag capability yet.
    const polarplot::HoverTarget hovered = polarplot::hover_target(plan.a, plan.b, plan.convention);
    const polarplot::MarkerColor* const a_marker_color =
        (hovered == polarplot::HoverTarget::kA) ? &polarplot::kHoverMarkerColor : nullptr;
    const polarplot::MarkerColor* const b_marker_color =
        (hovered == polarplot::HoverTarget::kB) ? &polarplot::kHoverMarkerColor : nullptr;

    polarplot::draw_vector("A", plan.a, plan.convention, marker_style_, /*head_frac=*/0.12,
                           line_width_, a_marker_color);
    polarplot::draw_vector("B", plan.b, plan.convention, marker_style_, /*head_frac=*/0.12,
                           line_width_, b_marker_color);
    if (plan.difference) {
        polarplot::draw_vector("A - B", *plan.difference, plan.convention, marker_style_,
                               /*head_frac=*/0.12, line_width_);
    }
    if (plan.sum) {
        polarplot::draw_vector("A + B", *plan.sum, plan.convention, marker_style_,
                               /*head_frac=*/0.12, line_width_);
    }
    if (plan.difference_ba) {
        polarplot::draw_vector("B - A", *plan.difference_ba, plan.convention, marker_style_,
                               /*head_frac=*/0.12, line_width_);
    }
    if (plan.product) {
        polarplot::draw_vector("A x B", *plan.product, plan.convention, marker_style_,
                               /*head_frac=*/0.12, line_width_);
    }
    if (plan.quotient_ab) {
        polarplot::draw_vector("A / B", *plan.quotient_ab, plan.convention, marker_style_,
                               /*head_frac=*/0.12, line_width_);
    }
    if (plan.quotient_ba) {
        polarplot::draw_vector("B / A", *plan.quotient_ba, plan.convention, marker_style_,
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
