#ifndef UI_VECTOR_PALETTE_HPP
#define UI_VECTOR_PALETTE_HPP

#include "polar_plotting/polar_plot.hpp"

/// \file
/// A fixed, distinct color for each of the 8 named vectors (A, B, and the 6
/// derived vectors), keyed by the exact label string \c App::draw_plot passes
/// to \c polarplot::draw_vector / \c polarplot::draw_interactive_vector.
///
/// This exists so a vector's rendered color never depends on ImPlot's
/// auto-cycle assignment order, which reassigns colors once a toggled-off
/// derived vector's legend entry is garbage-collected and later recreated
/// (see #67/#70). \c polar_plotting has no notion of "A", "A - B", etc. --
/// this mapping lives in \c ui so that module boundary stays intact.

namespace ui {

/// The fixed color for the named vector labelled \p label, from the palette
/// specified in #67/#70. \p label must be one of the 8 exact strings used by
/// \c App::draw_plot ("A", "B", "A + B", "A - B", "B - A", "A x B", "A / B",
/// "B / A"); any other input is a caller bug (asserted, not silently
/// defaulted). A and B's colors are unchanged from today's ImPlot
/// auto-cycled values. Pure function -- the test seam for this mapping.
[[nodiscard]] polarplot::MarkerColor vector_color(const char* label);

}  // namespace ui

#endif  // UI_VECTOR_PALETTE_HPP
