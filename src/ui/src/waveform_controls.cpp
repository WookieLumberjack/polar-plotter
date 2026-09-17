#include "ui/waveform_controls.hpp"

#include <algorithm>

namespace ui {

float clamp_waveform_frequency_hz(float frequency_hz) {
    return std::clamp(frequency_hz, kMinWaveformFrequencyHz, kMaxWaveformFrequencyHz);
}

}  // namespace ui
