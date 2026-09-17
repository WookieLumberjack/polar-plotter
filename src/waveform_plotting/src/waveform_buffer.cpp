#include "waveform_plotting/waveform_buffer.hpp"

#include <cmath>
#include <numbers>

namespace waveform_plotting {

void advance(WaveformBuffer& buffer, bool visible, float amplitude, float phase_deg,
             float angular_frequency_rad_per_s, PhaseConvention convention, float elapsed_seconds) {
    float sample = 0.0F;
    if (visible) {
        const float phase_rad = phase_deg * std::numbers::pi_v<float> / 180.0F;
        const float signed_phase = (convention == PhaseConvention::kLag) ? -phase_rad : phase_rad;
        sample =
            amplitude * std::cos((angular_frequency_rad_per_s * elapsed_seconds) + signed_phase);
    }

    buffer.samples.at(buffer.cursor) = sample;
    buffer.cursor = (buffer.cursor + 1) % WaveformBuffer::kSampleCount;
}

}  // namespace waveform_plotting
