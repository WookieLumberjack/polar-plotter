#include "ui/derived_vectors.hpp"

namespace ui {

DerivedVectors compute_derived_vectors(vecmath::Vec2 a, vecmath::Vec2 b) {
    return {
        .sum = a + b,
        .difference_ab = a - b,
        .difference_ba = b - a,
        .product = vecmath::complex_multiply(a, b),
        .quotient_ab = vecmath::complex_divide(a, b),
        .quotient_ba = vecmath::complex_divide(b, a),
    };
}

}  // namespace ui
