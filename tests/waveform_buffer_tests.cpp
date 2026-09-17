#include <cmath>
#include <numbers>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "waveform_plotting/waveform_buffer.hpp"

using waveform_plotting::advance;
using waveform_plotting::PhaseConvention;
using waveform_plotting::WaveformBuffer;

TEST_CASE("advance writes zero whenever not visible, regardless of amplitude/phase",
          "[waveform_plotting]") {
    WaveformBuffer buffer;
    advance(buffer, false, 5.0F, 45.0F, 2.0F, PhaseConvention::kLag, 1.25F);
    REQUIRE(buffer.samples.at(0) == Catch::Approx(0.0F));

    advance(buffer, false, -3.0F, 180.0F, 10.0F, PhaseConvention::kLead, 0.0F);
    REQUIRE(buffer.samples.at(1) == Catch::Approx(0.0F));
}

TEST_CASE("known-value case: elapsed_seconds=0, phase_deg=0 yields amplitude",
          "[waveform_plotting]") {
    WaveformBuffer buffer;
    advance(buffer, true, 3.0F, 0.0F, 5.0F, PhaseConvention::kLag, 0.0F);
    REQUIRE(buffer.samples.at(0) == Catch::Approx(3.0F));

    WaveformBuffer buffer2;
    advance(buffer2, true, 3.0F, 0.0F, 5.0F, PhaseConvention::kLead, 0.0F);
    REQUIRE(buffer2.samples.at(0) == Catch::Approx(3.0F));
}

TEST_CASE("advance writes amplitude*cos(wt) at t>0 with zero phase", "[waveform_plotting]") {
    WaveformBuffer buffer;
    constexpr float kAmplitude = 2.0F;
    constexpr float kOmega = 1.0F;
    constexpr float kElapsed = 1.0F;
    advance(buffer, true, kAmplitude, 0.0F, kOmega, PhaseConvention::kLag, kElapsed);
    REQUIRE(buffer.samples.at(0) == Catch::Approx(kAmplitude * std::cos(kOmega * kElapsed)));
}

TEST_CASE("Lag and Lead produce mirrored signs for the same nonzero phase", "[waveform_plotting]") {
    constexpr float kAmplitude = 1.0F;
    constexpr float kOmega = 3.0F;
    constexpr float kElapsed = 0.7F;
    constexpr float kPhaseDeg = 30.0F;
    constexpr float kPhaseRad = kPhaseDeg * std::numbers::pi_v<float> / 180.0F;

    WaveformBuffer lag_buffer;
    advance(lag_buffer, true, kAmplitude, kPhaseDeg, kOmega, PhaseConvention::kLag, kElapsed);

    WaveformBuffer lead_buffer;
    advance(lead_buffer, true, kAmplitude, kPhaseDeg, kOmega, PhaseConvention::kLead, kElapsed);

    const float expected_lag = kAmplitude * std::cos((kOmega * kElapsed) - kPhaseRad);
    const float expected_lead = kAmplitude * std::cos((kOmega * kElapsed) + kPhaseRad);

    REQUIRE(lag_buffer.samples.at(0) == Catch::Approx(expected_lag));
    REQUIRE(lead_buffer.samples.at(0) == Catch::Approx(expected_lead));
    // Not the trivial phase=0 case, so Lag/Lead must actually differ.
    REQUIRE(expected_lag != Catch::Approx(expected_lead));
}

TEST_CASE(
    "switching visible from false to true mid-sequence starts exactly at that sample, "
    "with no replay of prior true values",
    "[waveform_plotting]") {
    WaveformBuffer buffer;

    // A handful of hidden ticks -- all should be zero, cursor advancing normally.
    for (int i = 0; i < 5; ++i) {
        advance(buffer, false, 4.0F, 20.0F, 6.0F, PhaseConvention::kLag,
                static_cast<float>(i) * 0.1F);
    }
    REQUIRE(buffer.cursor == 5);
    for (std::size_t i = 0; i < 5; ++i) {
        REQUIRE(buffer.samples.at(i) == Catch::Approx(0.0F));
    }

    // Now become visible: the very next sample (index 5) should be the true
    // waveform value for that tick's elapsed_seconds, not a "replay" of what
    // earlier (still-hidden) ticks would have produced.
    constexpr float kAmplitude = 4.0F;
    constexpr float kOmega = 6.0F;
    constexpr float kElapsed = 0.5F;
    advance(buffer, true, kAmplitude, 20.0F, kOmega, PhaseConvention::kLag, kElapsed);

    constexpr float kPhaseRad = 20.0F * std::numbers::pi_v<float> / 180.0F;
    const float expected = kAmplitude * std::cos((kOmega * kElapsed) - kPhaseRad);
    REQUIRE(buffer.samples.at(5) == Catch::Approx(expected));
    REQUIRE(buffer.cursor == 6);

    // Prior samples (0-4) remain untouched zeros -- no replay.
    for (std::size_t i = 0; i < 5; ++i) {
        REQUIRE(buffer.samples.at(i) == Catch::Approx(0.0F));
    }
}

TEST_CASE("cursor wraps around correctly after a full buffer's worth of calls",
          "[waveform_plotting]") {
    WaveformBuffer buffer;
    REQUIRE(buffer.cursor == 0);

    for (std::size_t i = 0; i < WaveformBuffer::kSampleCount; ++i) {
        advance(buffer, true, 1.0F, 0.0F, 1.0F, PhaseConvention::kLag, static_cast<float>(i));
    }
    // A full buffer's worth of calls should have wrapped the cursor back to 0.
    REQUIRE(buffer.cursor == 0);

    // One more call writes at index 0 again, wrapping cursor to 1.
    advance(buffer, true, 9.0F, 0.0F, 1.0F, PhaseConvention::kLag, 0.0F);
    REQUIRE(buffer.samples.at(0) == Catch::Approx(9.0F));
    REQUIRE(buffer.cursor == 1);
}
