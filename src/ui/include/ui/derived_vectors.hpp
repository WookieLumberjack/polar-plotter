#ifndef UI_DERIVED_VECTORS_HPP
#define UI_DERIVED_VECTORS_HPP

#include <optional>

#include "vector_math/vec2.hpp"

/// \file
/// Pure computation of the 6 derived named vectors shown in the derived-vector
/// table. No ImGui/ImPlot dependency -- this is the independently-testable
/// seam for that table.

namespace ui {

/// The 6 derived named vectors computed from two named vectors \c a and \c b.
/// The two complex quotients are std::optional: nullopt when their divisor
/// has zero magnitude (see vecmath::complex_divide).
struct DerivedVectors {
    vecmath::Vec2 sum;                         ///< A + B
    vecmath::Vec2 difference_ab;               ///< A - B
    vecmath::Vec2 difference_ba;               ///< B - A
    vecmath::Vec2 product;                     ///< A x B (complex product)
    std::optional<vecmath::Vec2> quotient_ab;  ///< A / B (complex quotient)
    std::optional<vecmath::Vec2> quotient_ba;  ///< B / A (complex quotient)
};

/// Compute all 6 derived vectors from \p a and \p b.
[[nodiscard]] DerivedVectors compute_derived_vectors(vecmath::Vec2 a, vecmath::Vec2 b);

}  // namespace ui

#endif  // UI_DERIVED_VECTORS_HPP
