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

// Convert an ImPlot-resolved color (from ImPlot::GetLastItemColor()) to a
// polarplot::MarkerColor, for handing a vector's just-drawn on-plot color
// straight into polarplot::draw_length_tick (see #62) -- so a length tick's
// color is always read off the vector's actual drawn color, never
// independently assigned.
polarplot::MarkerColor to_marker_color(const ImVec4& color) {
    return {color.x, color.y, color.z, color.w};
}

// Draw a length tick for a just-drawn vector: magnitude is always
// std::hypot(x, y) of \p head's math-convention components -- never its
// plotted y-coordinate -- and color is read off whatever item ImPlot last
// drew (that vector's own shaft/arrowhead), reusing draw_vector's own
// shaft/head color-sync trick (see polar_plot.cpp's plot_arrow_shape) so tick
// color is guaranteed to match rather than independently assigned.
void draw_length_tick_for(polarplot::Point head) {
    const double magnitude = std::hypot(head.x, head.y);
    polarplot::draw_length_tick(magnitude, to_marker_color(ImPlot::GetLastItemColor()));
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

void App::draw_dockspace_host() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);

    // Standard invisible-dockspace-host window: fills the viewport, has no
    // chrome of its own, and never becomes a dockable node itself (only its
    // DockSpace() child area is). ImGuiWindowFlags_MenuBar is reserved here
    // (unused for now) so a later ticket adding the actual menu bar doesn't
    // need to touch this window's flags again.
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

    // The grid's outer ring is drawn at exactly `view_frame.extent()` -- the
    // same PlotFrame handed to begin_vector_plot and draw_rotation_indicator
    // -- so the scale ruler's tick positions always land exactly on the
    // rings they label (PlotFrame's constructor keeps the two in sync).
    // begin_vector_plot inflates its own axis view a bit beyond that extent
    // internally (rather than shrinking the grid inside an unchanged view,
    // which would move the rings off the ruler's ticks) so the grid's spoke
    // degree labels, drawn just outside the outer ring, have room without
    // getting clipped.
    const double extent = view_frame.extent();

    if (!polarplot::begin_vector_plot("##polar", view_frame)) {
        return;
    }
    polarplot::draw_polar_grid(view_frame, plan.convention);

    // Hover/click-drag for A/B (see #44/#47/#48): whichever tip is under the
    // cursor is this frame's hit-test target; draw_interactive_vector turns
    // that plus each vector's own carried-over dragging state into this
    // frame's marker color, head position, and interaction state.
    const polarplot::HoverTarget hovered = polarplot::hover_target(plan.a, plan.b, plan.convention);

    // Manual-scale drag clamp (#44/#49): with auto-scale off, `extent`
    // (this same PlotFrame's extent, already used above for the grid/axis
    // view) never grows with the dragged vector's magnitude, so passing it
    // through as the visible-extent clamp keeps a manual-scale drag from
    // moving the tip past what's currently visible. Ignored while
    // auto_scale_ is true.
    const polarplot::InteractiveVectorResult a_result = polarplot::draw_interactive_vector(
        "A", plan.a, plan.convention, marker_style_, hovered == polarplot::HoverTarget::kA,
        a_.dragging_, /*head_frac=*/0.12, line_width_, auto_scale_, extent);
    draw_length_tick_for(a_result.head);
    const polarplot::InteractiveVectorResult b_result = polarplot::draw_interactive_vector(
        "B", plan.b, plan.convention, marker_style_, hovered == polarplot::HoverTarget::kB,
        b_.dragging_, /*head_frac=*/0.12, line_width_, auto_scale_, extent);
    draw_length_tick_for(b_result.head);
    // Live drag tooltip (#44/#51): only while actively dragging, not during a
    // plain pre-drag hover -- disappears the instant the drag ends, since a
    // kReleased/kIdle/kHovered frame no longer matches kDragging here.
    if (a_result.state == polarplot::InteractionState::kDragging) {
        draw_drag_tooltip("A", a_result.head);
    }
    if (b_result.state == polarplot::InteractionState::kDragging) {
        draw_drag_tooltip("B", b_result.head);
    }
    apply_interactive_result(a_, a_result);
    apply_interactive_result(b_, b_result);
    if (plan.difference) {
        polarplot::draw_vector("A - B", *plan.difference, plan.convention, marker_style_,
                               /*head_frac=*/0.12, line_width_);
        draw_length_tick_for(*plan.difference);
    }
    if (plan.sum) {
        polarplot::draw_vector("A + B", *plan.sum, plan.convention, marker_style_,
                               /*head_frac=*/0.12, line_width_);
        draw_length_tick_for(*plan.sum);
    }
    if (plan.difference_ba) {
        polarplot::draw_vector("B - A", *plan.difference_ba, plan.convention, marker_style_,
                               /*head_frac=*/0.12, line_width_);
        draw_length_tick_for(*plan.difference_ba);
    }
    if (plan.product) {
        polarplot::draw_vector("A x B", *plan.product, plan.convention, marker_style_,
                               /*head_frac=*/0.12, line_width_);
        draw_length_tick_for(*plan.product);
    }
    if (plan.quotient_ab) {
        polarplot::draw_vector("A / B", *plan.quotient_ab, plan.convention, marker_style_,
                               /*head_frac=*/0.12, line_width_);
        draw_length_tick_for(*plan.quotient_ab);
    }
    if (plan.quotient_ba) {
        polarplot::draw_vector("B / A", *plan.quotient_ba, plan.convention, marker_style_,
                               /*head_frac=*/0.12, line_width_);
        draw_length_tick_for(*plan.quotient_ba);
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
    polarplot::draw_rotation_indicator(view_frame, plan.rotation_indicator_sweep_sign,
                                       rotation_indicator_style);
    polarplot::end_vector_plot();
}

}  // namespace ui
