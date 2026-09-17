#ifndef UI_WAVEFORM_CONTROLS_HPP
#define UI_WAVEFORM_CONTROLS_HPP

/// \file
/// The waveform panel's frequency input bound (#105/#108): a single pure
/// clamp, kept as its own tiny seam -- mirroring ui::zero_direction's
/// pure-helper shape -- so it's unit-tested without booting the app, and
/// shared verbatim by both the live frequency input field
/// (ui::App::draw_waveform_panel) and ui::Config's on-load clamp of a
/// hand-edited/out-of-range persisted value.

namespace ui {

/// Frequency input's valid range (Hz), inclusive at both ends.
inline constexpr float kMinWaveformFrequencyHz = 0.1F;
inline constexpr float kMaxWaveformFrequencyHz = 5.0F;

/// Clamp \p frequency_hz into [kMinWaveformFrequencyHz, kMaxWaveformFrequencyHz].
/// Pure function -- the test seam for this bound.
[[nodiscard]] float clamp_waveform_frequency_hz(float frequency_hz);

}  // namespace ui

#endif  // UI_WAVEFORM_CONTROLS_HPP
