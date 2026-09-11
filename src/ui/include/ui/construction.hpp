#ifndef UI_CONSTRUCTION_HPP
#define UI_CONSTRUCTION_HPP

/// \file
/// Pure functions computing the tip-to-tail construction vectors for a
/// vector sum/difference. No ImGui/ImPlot dependency -- callable from
/// ui::App::draw_plot but independently unit-testable.

#include "vector_math/vec2.hpp"

namespace ui {

/// A free vector construction: where it starts and its displacement. Maps
/// directly onto polarplot::AnnotationVector once ui::App converts each
/// vecmath::Vec2 to a polarplot::Point.
struct ConstructionVector {
    vecmath::Vec2 start;
    vecmath::Vec2 vector;

    friend constexpr bool operator==(const ConstructionVector&,
                                     const ConstructionVector&) = default;
};

/// The two parallelogram paths for showing how `a + b` is built tip-to-tail:
/// a copy of `b` starting at `a`'s tip, and a copy of `a` starting at `b`'s
/// tip. Both arrive at the same resultant `a + b`.
struct SumConstruction {
    ConstructionVector b_from_a_tip;
    ConstructionVector a_from_b_tip;

    friend constexpr bool operator==(const SumConstruction&, const SumConstruction&) = default;
};

/// Compute \ref SumConstruction for `a + b`.
[[nodiscard]] constexpr SumConstruction tip_to_tail_sum(vecmath::Vec2 a, vecmath::Vec2 b) {
    return SumConstruction{
        .b_from_a_tip = ConstructionVector{.start = a, .vector = b},
        .a_from_b_tip = ConstructionVector{.start = b, .vector = a},
    };
}

/// The tip-to-tail construction for `a - b`: a copy of `-b` starting at `a`'s
/// tip, arriving at the resultant `a - b`.
[[nodiscard]] constexpr ConstructionVector tip_to_tail_difference(vecmath::Vec2 a,
                                                                  vecmath::Vec2 b) {
    return ConstructionVector{.start = a, .vector = -b};
}

/// The difference segment for `a - b`: the free vector from `b`'s tip to
/// `a`'s tip. Congruent to `a - b` itself, but drawn where the two source
/// vectors actually are rather than at the origin. Degenerates to a
/// zero-length segment at `a` when `a == b`.
[[nodiscard]] constexpr ConstructionVector difference_segment(vecmath::Vec2 a, vecmath::Vec2 b) {
    return ConstructionVector{.start = b, .vector = a - b};
}

}  // namespace ui

#endif  // UI_CONSTRUCTION_HPP
