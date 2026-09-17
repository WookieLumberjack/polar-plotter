#include "waveform_ticker.hpp"

namespace app {

namespace {

constexpr double kTickSeconds = 1.0 / static_cast<double>(ui::kWaveformTickHz);

}  // namespace

void WaveformTicker::tick(double now, ui::App& app) {
    accumulator_ += now - last_time_;
    last_time_ = now;
    while (accumulator_ >= kTickSeconds) {
        app.advance_waveforms(static_cast<float>(kTickSeconds));
        accumulator_ -= kTickSeconds;
    }
}

}  // namespace app
