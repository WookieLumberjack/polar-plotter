#ifndef UI_NAMED_VECTOR_SPEC_HPP
#define UI_NAMED_VECTOR_SPEC_HPP

#include <array>
#include <cstddef>
#include <optional>

#include "polar_plotting/polar_plot.hpp"
#include "ui/derived_vectors.hpp"
#include "ui/plot_plan.hpp"
#include "vector_math/vec2.hpp"

/// \file
/// The single declaration site for identity shared by all 8 named vectors (A,
/// B, and the 6 derived vectors): each entry's label, fixed on-plot color,
/// and -- for the 6 derived vectors only -- how to read its current value (a
/// member-pointer accessor into ui::DerivedVectors) and whether it's
/// currently toggled on (a member-pointer into ui::PlotInputs). Replaces the
/// identity that used to be scattered across ui::plan_plot's
/// collect_derived_vectors, ui::vector_color's strcmp chain, and
/// App::draw_derived_vectors_table's row array.

namespace ui {

/// One named vector's fixed identity. \c accessor and \c toggle are null for
/// A and B, which have no derived-vector formula or show_* toggle of their
/// own (they're always drawn, driven directly by PlotInputs::a/b instead);
/// every other (derived-vector) entry has both set.
struct NamedVectorSpec {
    const char* label{nullptr};
    polarplot::MarkerColor color;
    std::optional<vecmath::Vec2> DerivedVectors::* accessor{nullptr};
    bool PlotInputs::* toggle{nullptr};
};

/// Number of named vectors (see \ref kNamedVectorSpecs). Also sizes
/// ui::App's parallel array of waveform_plotting::WaveformBuffer, one per
/// slot (#108) -- named rather than repeating a bare `8` at each use site.
inline constexpr std::size_t kNamedVectorCount = 8;

/// The 8 named vectors' fixed identity, in a fixed order: A, B, then the 6
/// derived vectors in the same order documented on
/// \c PlotPlan::derived_vectors' push-order contract (A - B, A + B, B - A,
/// A x B, A / B, B / A). \c ui::plan_plot's derived-vector collection and
/// \c ui::auto_fit_extent's magnitude loop both iterate this directly instead
/// of each hand-writing six per-vector blocks.
extern const std::array<NamedVectorSpec, kNamedVectorCount> kNamedVectorSpecs;

/// The fixed color for the named vector labelled \p label, sourced from
/// \ref kNamedVectorSpecs. \p label must be one of the 8 exact strings used
/// by \c App::draw_plot ("A", "B", "A + B", "A - B", "B - A", "A x B",
/// "A / B", "B / A"); any other input is a caller bug (asserted, not
/// silently defaulted). A and B's colors are unchanged from today's ImPlot
/// auto-cycled values. Pure function -- the test seam for this mapping.
[[nodiscard]] polarplot::MarkerColor named_vector_color(const char* label);

/// Whether \p spec's named vector is currently shown, using the exact same
/// gate \c ui::plan_plot's collect_derived_vectors/auto_fit_extent already
/// use internally (toggle on AND, for the two quotients, a defined value) --
/// the single source of truth for "is this named vector currently shown",
/// reused by anything else (e.g. the waveform panel, #108) that needs to
/// agree with the polar plot about what's visible rather than keeping a
/// second, parallel notion of visibility. A and B (\c spec.accessor ==
/// nullptr) are always shown. Pure function -- the test seam for this gate.
[[nodiscard]] bool named_vector_visible(const NamedVectorSpec& spec, const PlotInputs& inputs);

}  // namespace ui

#endif  // UI_NAMED_VECTOR_SPEC_HPP
