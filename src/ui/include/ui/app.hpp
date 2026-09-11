#ifndef UI_APP_HPP
#define UI_APP_HPP

#include <array>
#include <filesystem>

#include "ui/angle_convention.hpp"

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
    };

    std::filesystem::path config_path_;
    VectorInput a_{{3.0F, 1.0F}};
    VectorInput b_{{-1.0F, 2.0F}};
    bool show_sum_{false};
    bool show_difference_{true};
    // Raw zero-direction angle, in degrees, as entered by the user.
    float zero_direction_deg_{0.0F};
    // The two independent toggles that compose into polar_plotting's
    // angle_sign (see ui/angle_convention.hpp). Defaults reproduce the
    // previously-fixed +1 (counterclockwise, with rotation).
    RotationDirection rotation_direction_{RotationDirection::CounterClockwise};
    MeasurementConvention measurement_convention_{MeasurementConvention::WithRotation};

    void draw_controls();
    void draw_plot() const;
};

}  // namespace ui

#endif  // UI_APP_HPP
