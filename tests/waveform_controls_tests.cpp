#include <catch2/catch_test_macros.hpp>

#include "ui/waveform_controls.hpp"

using ui::clamp_waveform_frequency_hz;
using ui::kMaxWaveformFrequencyHz;
using ui::kMinWaveformFrequencyHz;

TEST_CASE("clamp_waveform_frequency_hz leaves an in-range value unchanged",
          "[ui][waveform_controls]") {
    CHECK(clamp_waveform_frequency_hz(1.0F) == 1.0F);
    CHECK(clamp_waveform_frequency_hz(kMinWaveformFrequencyHz) == kMinWaveformFrequencyHz);
    CHECK(clamp_waveform_frequency_hz(kMaxWaveformFrequencyHz) == kMaxWaveformFrequencyHz);
}

TEST_CASE("clamp_waveform_frequency_hz clamps below the minimum up to it",
          "[ui][waveform_controls]") {
    CHECK(clamp_waveform_frequency_hz(0.0F) == kMinWaveformFrequencyHz);
    CHECK(clamp_waveform_frequency_hz(-5.0F) == kMinWaveformFrequencyHz);
}

TEST_CASE("clamp_waveform_frequency_hz clamps above the maximum down to it",
          "[ui][waveform_controls]") {
    CHECK(clamp_waveform_frequency_hz(5.1F) == kMaxWaveformFrequencyHz);
    CHECK(clamp_waveform_frequency_hz(1000.0F) == kMaxWaveformFrequencyHz);
}
