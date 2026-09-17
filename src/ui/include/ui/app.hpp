#ifndef UI_APP_HPP
#define UI_APP_HPP

#include <array>
#include <filesystem>

#include "polar_plotting/polar_plot.hpp"
#include "ui/angle_convention.hpp"
#include "ui/derived_vectors.hpp"
#include "ui/plot_plan.hpp"
#include "ui/polar_display.hpp"
#include "ui/theme.hpp"

namespace ui {

/// The application's UI state and per-frame rendering. Owns no windowing or
/// graphics resources -- the host (app/main.cpp) creates the ImGui/ImPlot
/// context and calls \ref render once per frame between NewFrame and Render.
class App {
public:
    App();

    /// Load persisted inputs from \p config_path if it exists.
    explicit App(std::filesystem::path config_path);

    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;

    /// Emit this frame's ImGui/ImPlot draw commands.
    void render();

    /// Persist current inputs to the config path (no-op if none was given).
    void save() const;

    /// True once the File > Exit menu item has been selected. ui:: must not
    /// depend on GLFW/windowing (see CLAUDE.md's module table), so App can't
    /// call glfwSetWindowShouldClose itself -- the host (app/main.cpp) polls
    /// this after render() and closes the window when it's true.
    [[nodiscard]] bool want_exit() const { return want_exit_; }

    /// Record the window's current content scale (1.0 = 100%, 1.5 = 150%,
    /// ...) and immediately reapply the current theme's style scaled to it.
    /// App has no idea what a "content scale" is beyond this plain float --
    /// reading it from GLFW and deciding when it changed is entirely the
    /// host's job (app/main.cpp, via glfwGetWindowContentScale and
    /// glfwSetWindowContentScaleCallback); this just keeps ImGuiStyle's
    /// rounding consistent with whatever the host last reported. See
    /// apply_current_theme() for why this always re-derives style from
    /// scratch rather than scaling in place.
    void set_content_scale(float content_scale);

private:
    struct VectorInput {
        std::array<float, 2> xy{0.0F, 0.0F};
        // Transient Amplitude/Phase state while one of those two fields has
        // ImGui keyboard focus (see draw_vector_input) -- lets a negative
        // amplitude survive across frames while the Amplitude field is being
        // typed into, since deriving it fresh from xy every frame would
        // always canonicalize it back to non-negative.
        PolarDisplay pending_polar_{};
        bool polar_active_{false};
        // Whether this vector's tip is currently being click-dragged (see
        // #44/#48): cross-frame state polar_plotting itself keeps none of,
        // fed back in as draw_interactive_vector's was_dragging next frame.
        bool dragging_{false};
    };

    std::filesystem::path config_path_;
    VectorInput a_{{3.0F, 1.0F}};
    VectorInput b_{{-1.0F, 2.0F}};
    bool show_sum_{false};
    bool show_difference_{true};
    bool show_tip_to_tail_{false};
    bool show_difference_segment_{false};
    // Show/plot toggles for the remaining derived vectors -- plain arrows,
    // no construction sub-toggles (see ui::Config::show_difference_ba etc.).
    bool show_difference_ba_{false};
    bool show_product_{false};
    bool show_quotient_ab_{false};
    bool show_quotient_ba_{false};
    // Raw zero-direction angle, in degrees, as entered by the user.
    float zero_direction_deg_{0.0F};
    // The two independent toggles that compose into polar_plotting's
    // angle_sign (see ui/angle_convention.hpp). Defaults reproduce the
    // previously-fixed +1 (counterclockwise, with rotation).
    RotationDirection rotation_direction_{RotationDirection::CounterClockwise};
    MeasurementConvention measurement_convention_{MeasurementConvention::WithRotation};
    // Set fresh each frame in draw_controls() -- true while either the raw
    // zero-direction input or the plain-language ("N deg left/right of top")
    // input has ImGui focus; draw_plot() reads it to decide whether to draw
    // the transient zero-direction angle arc this frame.
    bool zero_direction_input_focused_{false};
    // Independent, persistent lifecycle for the zero-direction angle arc:
    // when true, draw_plot() draws the arc every frame regardless of input
    // focus (in addition to the transient, focus-driven display above).
    bool show_zero_direction_arc_persistent_{false};
    // Plot-wide tip marker style, shared by every named vector (A, B, A+B,
    // A-B) -- there is no per-vector styling.
    polarplot::TipMarkerStyle marker_style_{polarplot::TipMarkerStyle::kDot};
    // Pixel thickness of vector shafts/heads and the zero-direction/annotation
    // arcs, shared by all of them -- there is no per-item styling.
    float line_width_{2.0F};
    // When true (the default), the plot's ring interval/extent auto-fits the
    // shown vectors' magnitudes; when false, manual_ring_interval_ is used
    // verbatim via the "Auto-scale" checkbox and slider in draw_controls().
    bool auto_scale_{true};
    // Ring interval used verbatim when auto_scale_ is false.
    float manual_ring_interval_{1.0F};
    // Snapshot of the auto-fit extent taken just before a drag begins, and
    // held fixed for the drag's whole duration (see draw_plot). With
    // auto-scale on, the extent auto-fits the dragged vector's own live
    // magnitude; feeding that same, still-changing magnitude back into the
    // pixel<->plot conversion used to interpret the drag's mouse position
    // every frame is a positive feedback loop (each frame's wider view makes
    // the same screen position map to an even larger magnitude) that runs
    // away exponentially within seconds. Freezing the view for the drag's
    // duration breaks the loop; auto-fit resumes normally, in one clean
    // jump, once the drag ends.
    polarplot::PlotFrame drag_frozen_extent_{1.0, kAutoFitRings};

    // This frame's 6 derived named vectors, computed once in draw_controls()
    // (see its call to compute_derived_vectors) and consumed both there (the
    // results table, quotient-checkbox enabled state) and by the next call to
    // draw_plot() (via PlotInputs::derived) -- see draw_controls' doc comment
    // for why using a one-frame-old value here is still exactly correct.
    // Initialized from a_/b_'s (possibly config-loaded) starting values in
    // both constructors so the very first frame -- drawn before
    // draw_controls() has run -- is already consistent.
    DerivedVectors derived_;

    // Currently-selected built-in visual theme (see ui/theme.hpp), applied to
    // ImGui::GetStyle() once on construction and again whenever the Theme
    // menu changes it (see draw_menu_bar) -- never reapplied every frame.
    Theme theme_{Theme::kSlate};
    // Window content scale last reported by the host (1.0 until
    // set_content_scale() is called -- see its doc comment). Purely a
    // multiplier as far as App is concerned; App never talks to GLFW.
    float content_scale_{1.0F};
    // See want_exit().
    bool want_exit_{false};

    // Re-derives theme_'s style from scratch -- theme_style(theme_) ->
    // scale_theme_style(..., content_scale_) -> apply_theme(...) -- and
    // pushes it into ImGui::GetStyle(). Called from both constructors, from
    // draw_menu_bar whenever the Theme selection changes, and from
    // set_content_scale whenever the host reports a new content scale.
    //
    // Deliberately never caches or compounds: ThemeStyle's rounding fields
    // and content-scale scaling both mutate the same ImGuiStyle fields
    // (WindowRounding/FrameRounding/GrabRounding), so e.g. scaling
    // ImGui::GetStyle() in place and then switching themes would silently
    // clobber the scaled rounding back to the new theme's fixed, unscaled
    // values (or the reverse: applying a theme after scaling without
    // re-scaling would leave stale, wrong-DPI rounding). Recomputing the
    // full theme_style -> scale_theme_style -> apply_theme chain from the
    // two source-of-truth members (theme_, content_scale_) every single time
    // makes that ordering bug structurally impossible rather than something
    // that has to be remembered at every call site.
    void apply_current_theme() const;

    // Full-viewport invisible host window + ImGui::DockSpace(); builds the
    // first-run default layout (Polar plot right 2/3, Vectors left 1/3) via
    // DockBuilder the first time the dockspace node doesn't exist yet, then
    // leaves layout entirely to the user (persisted via imgui.ini). Also
    // draws the File/Theme menu bar (see draw_menu_bar) inside this same
    // host window, which reserves ImGuiWindowFlags_MenuBar for it.
    void draw_dockspace_host();
    // File (Exit) and Theme (one selectable per Theme enum value, current
    // selection checked) menus, drawn via ImGui::BeginMenuBar() inside the
    // dockspace host window's Begin/End -- must be called between them.
    void draw_menu_bar();
    // Also recomputes derived_ from this frame's (possibly just-edited) a_/b_
    // -- the single call site for compute_derived_vectors -- for use by this
    // same function's results table/quotient-checkbox gating and by the next
    // call to draw_plot(), which is called before draw_controls() each frame
    // (see render()'s comment) so it necessarily reads a one-frame-old
    // derived_. That's still exactly right: derived_ is always recomputed
    // from whatever a_/b_ draw_plot() itself will read at the top of its next
    // call (nothing else changes a_/b_ in between), so the pairing stays
    // consistent and the formulas (unchanged) produce identical results
    // either way.
    void draw_controls();
    // Non-const: click-dragging a vector's tip (see #44/#48) writes the
    // updated position (and discards any in-progress Amplitude/Phase text
    // edit) straight back into a_/b_.
    void draw_plot();
    // Apply one vector's this-frame draw_interactive_vector result back into
    // its VectorInput: while dragging or on the release frame, the head
    // position wins over whatever xy held before, and any pending polar text
    // edit for that vector is discarded (same precedence as clicking into
    // Real/Imag -- see draw_vector_input).
    static void apply_interactive_result(VectorInput& input,
                                         const polarplot::InteractiveVectorResult& result);
    // Draws the 4 live-synced Amplitude/Phase/Real/Imag fields for one named
    // vector (\p label_prefix is "A" or "B"), applying the focus-tracked sync
    // rule: whichever field has focus this frame is the source of truth, the
    // rest are recomputed from it.
    static void draw_vector_input(const char* label_prefix, VectorInput& input);
    // Read-only table of the 6 derived vectors (A+B, A-B, B-A, AxB, A/B,
    // B/A), each row showing Amplitude/Phase/Real/Imag.
    static void draw_derived_vectors_table(const DerivedVectors& derived);
};

}  // namespace ui

#endif  // UI_APP_HPP
