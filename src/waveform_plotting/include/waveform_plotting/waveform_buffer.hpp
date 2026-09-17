#ifndef WAVEFORM_PLOTTING_WAVEFORM_BUFFER_HPP
#define WAVEFORM_PLOTTING_WAVEFORM_BUFFER_HPP

/// \file
/// The pure, tested logic core of the time-domain waveform plot (#105/#106):
/// a fixed-size sample ring buffer plus a single per-tick step function.
/// Depends only on Dear ImGui and ImPlot -- deliberately no dependency on
/// vector_math or ui -- so this module can be lifted into another project,
/// mirroring polar_plotting's shape.

#include <array>
#include <cstddef>
#include <cstdint>

namespace waveform_plotting {

/// Global Lag/Lead toggle controlling the sign of phi inside a waveform's
/// `amplitude * cos(omega * t +/- phi)` equation: \c kLag uses
/// `cos(omega * t - phi)`, \c kLead uses `cos(omega * t + phi)`. A purely
/// temporal concern -- see CONTEXT.md's "Phase convention" entry -- entirely
/// independent of polar_plotting's spatial AngleConvention; never let the two
/// interact.
enum class PhaseConvention : std::uint8_t {
    kLag,
    kLead,
};

/// A fixed-size ring buffer of waveform samples plus its write cursor. Plain
/// data, constructible directly in test code with no ImGui/ImPlot/GLFW
/// context. \ref advance is the only way samples are written.
struct WaveformBuffer {
    /// Fixed sample count -- not configurable, matching the spec.
    static constexpr std::size_t kSampleCount = 1000;

    std::array<float, kSampleCount> samples{};
    /// Index of the next sample \ref advance will write, wrapping at
    /// \c kSampleCount.
    std::size_t cursor{0};
};

/// The single pure per-tick step function: writes exactly one new sample at
/// \p buffer's current cursor, then advances the cursor (wrapping at
/// \c WaveformBuffer::kSampleCount).
///
/// When \p visible is false, writes `0.0F` regardless of \p amplitude/
/// \p phase_deg/\p angular_frequency_rad_per_s/\p elapsed_seconds -- a
/// visible=false-to-true transition mid-sequence never "replays" what earlier
/// hidden ticks would have produced; the first true-valued sample lands
/// exactly at the tick where \p visible first becomes true.
///
/// When \p visible is true, writes
/// `amplitude * cos(angular_frequency_rad_per_s * elapsed_seconds +/- phase_rad)`,
/// where `phase_rad = phase_deg` converted to radians and the sign is chosen
/// by \p convention (\c kLag subtracts, \c kLead adds).
void advance(WaveformBuffer& buffer, bool visible, float amplitude, float phase_deg,
             float angular_frequency_rad_per_s, PhaseConvention convention, float elapsed_seconds);

}  // namespace waveform_plotting

#endif  // WAVEFORM_PLOTTING_WAVEFORM_BUFFER_HPP
