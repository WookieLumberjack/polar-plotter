#ifndef VECTOR_MATH_VEC2_HPP
#define VECTOR_MATH_VEC2_HPP

/// \file
/// 2D vector primitives. This module is deliberately free of any UI / ImGui /
/// ImPlot dependency so it can be unit-tested in isolation and reused.

namespace vecmath {

/// A 2D vector in Cartesian components.
struct Vec2 {
    double x{0.0};
    double y{0.0};

    friend constexpr bool operator==(const Vec2&, const Vec2&) = default;
};

/// A 2D vector expressed in polar form.
struct Polar {
    double radius{0.0};     ///< Non-negative length.
    double angle_rad{0.0};  ///< Angle from +x axis, radians, in (-pi, pi].

    friend constexpr bool operator==(const Polar&, const Polar&) = default;
};

// --- Component-wise arithmetic ------------------------------------------------

constexpr Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
constexpr Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
constexpr Vec2 operator-(Vec2 v) { return {-v.x, -v.y}; }
constexpr Vec2 operator*(Vec2 v, double s) { return {v.x * s, v.y * s}; }
constexpr Vec2 operator*(double s, Vec2 v) { return v * s; }
constexpr Vec2 operator/(Vec2 v, double s) { return {v.x / s, v.y / s}; }

constexpr Vec2& operator+=(Vec2& a, Vec2 b) { return a = a + b; }
constexpr Vec2& operator-=(Vec2& a, Vec2 b) { return a = a - b; }
constexpr Vec2& operator*=(Vec2& v, double s) { return v = v * s; }

// --- Products ----------------------------------------------------------------

/// Dot product.
constexpr double dot(Vec2 a, Vec2 b) { return (a.x * b.x) + (a.y * b.y); }

/// z-component of the 3D cross product of \p a and \p b (a scalar in 2D).
constexpr double cross(Vec2 a, Vec2 b) { return (a.x * b.y) - (a.y * b.x); }

/// Squared magnitude. Cheap; prefer this for comparisons.
constexpr double magnitude_squared(Vec2 v) { return dot(v, v); }

// --- Trigonometric helpers (defined in vec2.cpp) ----------------------------

/// Euclidean length of \p v.
[[nodiscard]] double magnitude(Vec2 v);

/// Angle of \p v measured from the +x axis, radians, in (-pi, pi].
/// Returns 0 for the zero vector.
[[nodiscard]] double angle(Vec2 v);

/// Construct a vector from a length and an angle (radians from +x axis).
[[nodiscard]] Vec2 from_polar(double radius, double angle_rad);

/// Construct a vector from a polar pair.
[[nodiscard]] Vec2 from_polar(Polar p);

/// Convert \p v to polar form. The zero vector maps to {0, 0}.
[[nodiscard]] Polar to_polar(Vec2 v);

/// True when \p v is within \p epsilon (in magnitude) of the zero vector.
[[nodiscard]] bool is_zero(Vec2 v, double epsilon = 1e-12);

/// Unit vector in the direction of \p v. Returns {0, 0} when \p v is the zero
/// vector (see \ref is_zero).
[[nodiscard]] Vec2 normalized(Vec2 v);

/// Smallest angle between \p a and \p b, radians, in [0, pi].
/// Returns 0 when either argument is the zero vector.
[[nodiscard]] double angle_between(Vec2 a, Vec2 b);

}  // namespace vecmath

#endif  // VECTOR_MATH_VEC2_HPP
