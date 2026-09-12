#ifndef UI_POLAR_DISPLAY_HPP
#define UI_POLAR_DISPLAY_HPP

#include "vector_math/vec2.hpp"

/// \file
/// Pure conversion between a named vector's Cartesian (Real, Imag) form and
/// an Amplitude/Phase-in-degrees display form. No ImGui/ImPlot dependency --
/// this is the independently-testable seam for the bidirectional
/// Amplitude/Phase/Real/Imag input fields.

namespace ui {

/// A vector expressed as Amplitude/Phase for display. \p phase_deg is
/// normalized to [0, 360) by \ref to_polar_display and
/// \ref canonicalize_polar_display, but \ref from_polar_display accepts any
/// finite amplitude/phase (including a transient negative amplitude while the
/// Amplitude field has focus).
struct PolarDisplay {
    float amplitude{0.0F};
    float phase_deg{0.0F};
};

/// Convert \p v to its canonical (non-negative amplitude, phase in [0, 360))
/// Amplitude/Phase display form.
[[nodiscard]] PolarDisplay to_polar_display(vecmath::Vec2 v);

/// Convert \p display back to Cartesian components. A negative amplitude is
/// accepted and behaves as a 180 deg direction flip (the same vector as the
/// canonical non-negative form), consistent with \ref
/// canonicalize_polar_display.
[[nodiscard]] vecmath::Vec2 from_polar_display(PolarDisplay display);

/// Snap a possibly-negative-amplitude \p display to its canonical form:
/// non-negative amplitude, with the sign folded into a 180 deg phase shift,
/// and the phase re-normalized to [0, 360). Called when the Amplitude input
/// field loses focus.
[[nodiscard]] PolarDisplay canonicalize_polar_display(PolarDisplay display);

}  // namespace ui

#endif  // UI_POLAR_DISPLAY_HPP
