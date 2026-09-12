# Complex multiply/divide are named free functions, and division returns std::optional<Vec2>

The A×B/A÷B/B÷A derived vectors treat two `Vec2`s as complex numbers
(amplitude multiplies/divides, phase adds/subtracts) — a different operation
from the existing scalar `dot`/`cross` on the same type. We expose this as
named free functions, `complex_multiply(Vec2, Vec2)` and
`complex_divide(Vec2, Vec2) -> std::optional<Vec2>`, rather than overloading
`operator*`/`operator/` on `Vec2×Vec2`. `Vec2` already defines
`operator*(Vec2, double)` for scalar scaling, and `dot`/`cross` — which are
also non-obvious two-`Vec2` operations — are named functions, not operators;
a `Vec2×Vec2` operator would silently overload beyond that precedent and
suggest complex multiplication is a native vector operation rather than an
explicit reinterpretation. `complex_divide` returns `std::nullopt` on a
zero-magnitude divisor instead of a NaN-filled `Vec2` or an assert/throw,
since division by a live-edited, possibly-zero vector is an expected
transient state, not a caller bug — the optional makes the undefined case
explicit at the type level and maps directly to the UI's placeholder/disabled
row.
