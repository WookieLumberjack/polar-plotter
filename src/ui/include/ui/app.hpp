#ifndef UI_APP_HPP
#define UI_APP_HPP

#include <array>
#include <filesystem>

#include "polar_plotting/polar_plot.hpp"
#include "ui/angle_convention.hpp"
#include "ui/polar_display.hpp"

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
    };

    std::filesystem::path config_path_;
    VectorInput a_{{3.0F, 1.0F}};
    VectorInput b_{{-1.0F, 2.0F}};
    bool show_sum_{false};
    bool show_difference_{true};
    bool show_tip_to_tail_{false};
    bool show_difference_segment_{false};
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

    void draw_controls();
    void draw_plot() const;
    // Draws the 4 live-synced Amplitude/Phase/Real/Imag fields for one named
    // vector (\p label_prefix is "A" or "B"), applying the focus-tracked sync
    // rule: whichever field has focus this frame is the source of truth, the
    // rest are recomputed from it.
    static void draw_vector_input(const char* label_prefix, VectorInput& input);
};

}  // namespace ui

#endif  // UI_APP_HPP
