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
#include <imgui_internal.h>  // DockBuilder* -- no public API for the first-run default layout.
#include <implot.h>

#include "polar_plotting/polar_plot.hpp"
#include "ui/config.hpp"
#include "ui/derived_vectors.hpp"
#include "ui/named_vector_spec.hpp"
#include "ui/plot_plan.hpp"
#include "ui/polar_display.hpp"
#include "ui/theme.hpp"
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

// Live drag tooltip (#44/#51): shown only while \p label's vector is actively
// dragging, near the cursor, with the same Amplitude/Phase/Real/Imag
// labels/format used by the vector input fields and derived-vectors table.
// Amplitude/Phase conversion is a ui-layer concern (ui::to_polar_display), so
// this is drawn here rather than pushed down into polar_plotting, which knows
// only raw Point/Vec2 and has no notion of that display convention.
void draw_drag_tooltip(const char* label, polarplot::Point head) {
    const vecmath::Vec2 v{head.x, head.y};
    const PolarDisplay display = to_polar_display(v);
    ImGui::BeginTooltip();
    ImGui::Text("%s", label);
    ImGui::Text("Amplitude: %.3f", static_cast<double>(display.amplitude));
    ImGui::Text("Phase (deg): %.2f", static_cast<double>(display.phase_deg));
    ImGui::Text("Real: %.3f", v.x);
    ImGui::Text("Imag: %.3f", v.y);
    ImGui::EndTooltip();
}

// Convert a ThemeStyle's text color to a polarplot::MarkerColor, for use as
// the default (idle-state) tip marker/label color override -- see #68: this
// replaces polar_plotting's old hardcoded near-white default, so tip
// markers/labels stay visible against light themes too.
polarplot::MarkerColor to_marker_color(const ThemeColor& color) {
    return {color.r, color.g, color.b, color.a};
}

}  // namespace

void App::draw_vector_input(const char* label_prefix, VectorInput& input) {
    PolarDisplay display =
        input.polar_active_ ? input.pending_polar_ : to_polar_display(to_vec(input.xy));

    const std::string amplitude_label = std::string(label_prefix) + ": Amp";
    const std::string phase_label = std::string(label_prefix) + ": Phase";
    const std::string real_label = std::string(label_prefix) + ": Re";
    const std::string imag_label = std::string(label_prefix) + ": Im";

    const float box_width = (ImGui::CalcItemWidth() - ImGui::GetStyle().ItemSpacing.x) / 2.0F;
    // ImGui draws each field's label immediately after its box, so a row's
    // second box would otherwise start at a different x depending on how
    // wide the first row's label text is (e.g. "Phase" vs "Re"). Reserving a
    // fixed label column -- the widest of the four labels, which is the same
    // for Vector A and Vector B since only the single-character prefix
    // differs -- keeps both columns aligned across all four fields and
    // across both vectors.
    const float label_width = std::max(
        {ImGui::CalcTextSize(amplitude_label.c_str()).x, ImGui::CalcTextSize(phase_label.c_str()).x,
         ImGui::CalcTextSize(real_label.c_str()).x, ImGui::CalcTextSize(imag_label.c_str()).x});
    const float inner_spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    const float col2_x = box_width + inner_spacing + label_width + ImGui::GetStyle().ItemSpacing.x;

    ImGui::SetNextItemWidth(box_width);
    const bool amp_changed =
        ImGui::InputFloat(("##" + amplitude_label).c_str(), &display.amplitude, 0.0F, 0.0F, "%.3f");
    const bool amp_focused = ImGui::IsItemFocused();
    ImGui::SameLine(0.0F, inner_spacing);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(amplitude_label.c_str());

    ImGui::SameLine(col2_x);
    ImGui::SetNextItemWidth(box_width);
    const bool phase_changed =
        ImGui::InputFloat(("##" + phase_label).c_str(), &display.phase_deg, 0.0F, 0.0F, "%.2f");
    const bool phase_focused = ImGui::IsItemFocused();
    ImGui::SameLine(0.0F, inner_spacing);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(phase_label.c_str());

    ImGui::SetNextItemWidth(box_width);
    const bool real_changed =
        ImGui::InputFloat(("##" + real_label).c_str(), input.xy.data(), 0.0F, 0.0F, "%.3f");
    ImGui::SameLine(0.0F, inner_spacing);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(real_label.c_str());

    ImGui::SameLine(col2_x);
    ImGui::SetNextItemWidth(box_width);
    const bool imag_changed =
        ImGui::InputFloat(("##" + imag_label).c_str(), input.xy.data() + 1, 0.0F, 0.0F, "%.3f");
    ImGui::SameLine(0.0F, inner_spacing);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(imag_label.c_str());

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
    if (!ImGui::BeginTable("derived_vectors", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        return;
    }
    ImGui::TableSetupColumn("Vector");
    ImGui::TableSetupColumn("Amplitude");
    ImGui::TableSetupColumn("Phase (deg)");
    ImGui::TableSetupColumn("Real");
    ImGui::TableSetupColumn("Imag");
    ImGui::TableHeadersRow();

    // Iterates kNamedVectorSpecs directly (skipping A/B's accessor-less
    // entries) rather than a second hand-written label list, so this table
    // can't drift from plan_plot/auto_fit_extent's identity source -- see
    // ui/named_vector_spec.hpp.
    for (const NamedVectorSpec& spec : kNamedVectorSpecs) {
        if (spec.accessor == nullptr) {
            continue;
        }
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(spec.label);
        if (const std::optional<vecmath::Vec2> vector = derived.*spec.accessor) {
            const PolarDisplay display = to_polar_display(*vector);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f", static_cast<double>(display.amplitude));
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.2f", static_cast<double>(display.phase_deg));
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.3f", vector->x);
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%.3f", vector->y);
        } else {
            for (int col = 1; col <= 4; ++col) {
                ImGui::TableSetColumnIndex(col);
                ImGui::TextUnformatted("--");
            }
        }
    }
    ImGui::EndTable();
}

App::App() : derived_(compute_derived_vectors(to_vec(a_.xy), to_vec(b_.xy))) {
    apply_current_theme();
}

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
        theme_ = cfg->theme;
    }
    // a_/b_ may have just been overwritten from cfg above, so derived_ is
    // (re)computed here rather than relying on the member initializer used by
    // the default constructor -- see derived_'s doc comment.
    derived_ = compute_derived_vectors(to_vec(a_.xy), to_vec(b_.xy));
    apply_current_theme();
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
        .theme = theme_,
    };
    (void)save_config(config_path_, cfg);
}

void App::apply_current_theme() const { apply_theme(theme_style(theme_)); }

void App::draw_dockspace_host() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);

    // Standard invisible-dockspace-host window: fills the viewport, has no
    // chrome of its own, and never becomes a dockable node itself (only its
    // DockSpace() child area is). ImGuiWindowFlags_MenuBar is reserved here
    // for the File/Theme menu bar drawn below.
    constexpr ImGuiWindowFlags kHostFlags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));
    ImGui::Begin("##dockspace_host", nullptr, kHostFlags);
    ImGui::PopStyleVar(3);

    draw_menu_bar();

    const ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

    // First run only: ImGui::DockBuilderGetNode returns null until a node
    // with this ID has been built at least once (either by us, here, or by
    // ImGui restoring one from a prior imgui.ini). Once it exists, layout is
    // entirely user-driven and persisted via imgui.ini -- we never rebuild
    // it again after this.
    if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, vp->WorkSize);

        ImGuiID left_id = 0;
        ImGuiID right_id = 0;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 1.0F / 3.0F, &left_id, &right_id);
        ImGui::DockBuilderDockWindow("Vectors", left_id);
        ImGui::DockBuilderDockWindow("Polar plot", right_id);
        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::DockSpace(dockspace_id);
    ImGui::End();
}

void App::draw_menu_bar() {
    if (!ImGui::BeginMenuBar()) {
        return;
    }
    if (ImGui::BeginMenu("File")) {
        // See want_exit()'s doc comment for why this sets a flag rather than
        // calling glfwSetWindowShouldClose itself.
        if (ImGui::MenuItem("Exit")) {
            want_exit_ = true;
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Theme")) {
        for (const Theme candidate : kAllThemes) {
            const bool selected = candidate == theme_;
            if (ImGui::MenuItem(theme_label(candidate), nullptr, selected) && !selected) {
                theme_ = candidate;
                apply_current_theme();
            }
        }
        ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
}

void App::render() {
    draw_dockspace_host();

    // Plot drawn first: dragging a tip (see #44/#48) writes the updated
    // position straight back into a_/b_ inside draw_plot(), so drawing the
    // plot before the controls window lets that same frame's Amplitude/
    // Phase/Real/Imag fields and derived-vectors table read the fresh
    // position -- window Begin/End order doesn't otherwise matter to either
    // window's own widgets. Both windows dock into the host's DockSpace by
    // name (see draw_dockspace_host); no explicit position/size is set here
    // any more -- the dockspace and, on first run, DockBuilder own that.
    ImGui::Begin("Polar plot");
    draw_plot();
    ImGui::End();

    ImGui::Begin("Vectors");
    draw_controls();
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

    // Degree fields only ever hold a signed angle (raw zero-direction can go
    // negative; degrees-from-top is clamped to [0, 180]), so size their text
    // box to "-360.00" rather than the panel's default full-width -- and size
    // the Side combo to its longest option ("right") plus its arrow.
    const float degree_field_width =
        ImGui::CalcTextSize("-360.00").x + (ImGui::GetStyle().FramePadding.x * 2.0F);
    const float side_combo_width = ImGui::CalcTextSize("right").x +
                                   (ImGui::GetStyle().FramePadding.x * 2.0F) +
                                   ImGui::GetFrameHeight();

    ImGui::SeparatorText("Zero direction");
    ImGui::SetNextItemWidth(degree_field_width);
    ImGui::InputFloat("Zero direction (deg)", &zero_direction_deg_, 1.0F, 10.0F, "%.2f");
    bool zero_direction_focused = ImGui::IsItemFocused();

    PlainZeroDirection plain = raw_to_plain_zero_direction(zero_direction_deg_);
    int side_index = plain.side == ZeroDirectionSide::kLeft ? 0 : 1;
    bool plain_changed = false;

    ImGui::PushID("zero_direction_plain");
    ImGui::TextUnformatted("Zero direction, plain language:");
    ImGui::SetNextItemWidth(side_combo_width);
    plain_changed |= ImGui::Combo("Side", &side_index, "left\0right\0\0");
    zero_direction_focused |= ImGui::IsItemFocused();
    ImGui::SetNextItemWidth(degree_field_width);
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

    ImGui::SeparatorText("Angle convention");
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
    ImGui::Checkbox("Show B - A", &show_difference_ba_);
    ImGui::SameLine();
    ImGui::Checkbox("Show A x B", &show_product_);

    // Single call site for compute_derived_vectors (see derived_'s doc
    // comment): this same frame's table below and next frame's draw_plot()
    // (via PlotInputs::derived) both read this member instead of
    // recomputing.
    derived_ = compute_derived_vectors(a, b);

    ImGui::BeginDisabled(!derived_.quotient_ab.has_value());
    ImGui::Checkbox("Show A / B", &show_quotient_ab_);
    ImGui::EndDisabled();
    if (!derived_.quotient_ab) {
        show_quotient_ab_ = false;
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!derived_.quotient_ba.has_value());
    ImGui::Checkbox("Show B / A", &show_quotient_ba_);
    ImGui::EndDisabled();
    if (!derived_.quotient_ba) {
        show_quotient_ba_ = false;
    }

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

    ImGui::SeparatorText("Derived quantities");
    ImGui::Text("A . B = %.4f", vecmath::dot(a, b));
    ImGui::Text("angle(A, B) = %.2f deg", vecmath::angle_between(a, b) * kRadToDeg);

    ImGui::SeparatorText("Derived vectors");
    draw_derived_vectors_table(derived_);
}

void App::apply_interactive_result(VectorInput& input,
                                   const polarplot::InteractiveVectorResult& result) {
    input.dragging_ = (result.state == polarplot::InteractionState::kDragging);
    if (result.state == polarplot::InteractionState::kDragging ||
        result.state == polarplot::InteractionState::kReleased) {
        // Starting (and continuing) a drag overrides any in-progress
        // Amplitude/Phase text edit for this vector -- same precedence as
        // clicking into Real/Imag today (see draw_vector_input).
        input.xy = {static_cast<float>(result.head.x), static_cast<float>(result.head.y)};
        input.polar_active_ = false;
    }
}

void App::draw_plot() {
    const vecmath::Vec2 a = to_vec(a_.xy);
    const vecmath::Vec2 b = to_vec(b_.xy);

    const PlotPlan plan = plan_plot(PlotInputs{
        .a = a,
        .b = b,
        .derived = derived_,
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

    // Freeze the plot's view for a drag's whole duration (see
    // drag_frozen_extent_'s doc comment): only refresh it from this frame's
    // fresh auto-fit/manual plan.extent while neither vector is already
    // mid-drag (i.e. the value here hasn't itself been influenced by the
    // drag it would otherwise be used to interpret).
    if (!(a_.dragging_ || b_.dragging_)) {
        drag_frozen_extent_ = plan.extent;
    }
    const polarplot::PlotFrame& view_frame = drag_frozen_extent_;

    // Theme-aware default tip marker/label color (#68): applies to every
    // named vector's tip marker/label whenever no hover/drag override takes
    // precedence, so markers stay visible against light themes instead of
    // polar_plotting's old hardcoded near-white default.
    const polarplot::DrawStyle style{
        .tip_marker_color = to_marker_color(theme_style(theme_).text),
        .line_width = line_width_,
        .marker_style = marker_style_,
        .color_for = &named_vector_color,
    };

    const std::optional<polarplot::SceneResult> result =
        polarplot::draw_scene(view_frame, plan, style, a_.dragging_, b_.dragging_);
    if (!result) {
        return;
    }

    // Live drag tooltip (#44/#51): only while actively dragging, not during a
    // plain pre-drag hover -- disappears the instant the drag ends, since a
    // kReleased/kIdle/kHovered frame no longer matches kDragging here.
    if (result->a.state == polarplot::InteractionState::kDragging) {
        draw_drag_tooltip("A", result->a.head);
    }
    if (result->b.state == polarplot::InteractionState::kDragging) {
        draw_drag_tooltip("B", result->b.head);
    }
    apply_interactive_result(a_, result->a);
    apply_interactive_result(b_, result->b);
}

}  // namespace ui
