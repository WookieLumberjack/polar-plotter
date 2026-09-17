#ifndef APP_WAVEFORM_TICKER_HPP
#define APP_WAVEFORM_TICKER_HPP

#include "ui/app.hpp"

namespace app {

// Wall-clock-to-fixed-tick accumulator (#105/#108/#109): ui::App::
// advance_waveforms must be called at a fixed ui::kWaveformTickHz cadence,
// decoupled from render frame rate, so the waveform's shape never
// warps/jitters when frame rate fluctuates.
//
// Shared between the OpenGL host (Linux/macOS, app/main.cpp) and the Vulkan
// host (Windows, app/vulkan_backend.cpp) -- see font_atlas.hpp for the same
// sharing precedent. Each host reads its own wall clock via glfwGetTime()
// (GLFW still owns the window on every platform, including Windows -- see
// CLAUDE.md's portability notes) and passes the raw `double` in via tick();
// this class knows nothing about GLFW, only plain elapsed seconds, mirroring
// ui::App::advance_waveforms's own plain-value host->ui boundary crossing.
class WaveformTicker {
public:
    // \p now establishes the baseline future tick() calls measure elapsed
    // time from -- construct this once, immediately before entering the
    // render loop, with the same clock reading tick() will later be called
    // with (e.g. `WaveformTicker ticker(glfwGetTime());`).
    explicit WaveformTicker(double now) : last_time_(now) {}

    WaveformTicker(const WaveformTicker&) = delete;
    WaveformTicker& operator=(const WaveformTicker&) = delete;
    WaveformTicker(WaveformTicker&&) = delete;
    WaveformTicker& operator=(WaveformTicker&&) = delete;
    ~WaveformTicker() = default;

    // Accumulates (\p now - the time of the previous tick()/construction
    // call) and calls app.advance_waveforms() once per whole
    // 1/ui::kWaveformTickHz-second tick that fits, carrying over any
    // fractional remainder to the next call -- so the waveform's shape stays
    // independent of render frame rate regardless of how irregularly tick()
    // itself gets called.
    void tick(double now, ui::App& app);

private:
    double last_time_;
    double accumulator_ = 0.0;
};

}  // namespace app

#endif  // APP_WAVEFORM_TICKER_HPP
