#include <catch2/catch_test_macros.hpp>

#include "ui/derived_vectors.hpp"
#include "vector_math/vec2.hpp"

using ui::compute_derived_vectors;
using ui::DerivedVectors;
using vecmath::Vec2;

TEST_CASE("compute_derived_vectors computes the additive derived vectors",
          "[ui][derived_vectors]") {
    constexpr Vec2 a{3.0, 1.0};
    constexpr Vec2 b{-1.0, 2.0};

    const DerivedVectors derived = compute_derived_vectors(a, b);

    REQUIRE(derived.sum.has_value());
    REQUIRE(derived.difference_ab.has_value());
    REQUIRE(derived.difference_ba.has_value());
    CHECK(derived.sum == a + b);
    CHECK(derived.difference_ab == a - b);
    CHECK(derived.difference_ba == b - a);
}

TEST_CASE("compute_derived_vectors computes the complex product", "[ui][derived_vectors]") {
    constexpr Vec2 a{3.0, 1.0};
    constexpr Vec2 b{-1.0, 2.0};

    const DerivedVectors derived = compute_derived_vectors(a, b);

    REQUIRE(derived.product.has_value());
    CHECK(derived.product == vecmath::complex_multiply(a, b));
}

TEST_CASE("compute_derived_vectors computes both complex quotients", "[ui][derived_vectors]") {
    constexpr Vec2 a{3.0, 1.0};
    constexpr Vec2 b{-1.0, 2.0};

    const DerivedVectors derived = compute_derived_vectors(a, b);

    REQUIRE(derived.quotient_ab.has_value());
    REQUIRE(derived.quotient_ba.has_value());
    CHECK(derived.quotient_ab == vecmath::complex_divide(a, b));
    CHECK(derived.quotient_ba == vecmath::complex_divide(b, a));
}

TEST_CASE("compute_derived_vectors leaves a quotient undefined for a zero-magnitude divisor",
          "[ui][derived_vectors]") {
    constexpr Vec2 a{3.0, 1.0};
    constexpr Vec2 zero{0.0, 0.0};

    SECTION("A / B is undefined when B is zero") {
        const DerivedVectors derived = compute_derived_vectors(a, zero);
        CHECK_FALSE(derived.quotient_ab.has_value());
        REQUIRE(derived.quotient_ba.has_value());
    }

    SECTION("B / A is undefined when B is zero") {
        const DerivedVectors derived = compute_derived_vectors(zero, a);
        CHECK_FALSE(derived.quotient_ba.has_value());
        REQUIRE(derived.quotient_ab.has_value());
    }
}

TEST_CASE(
    "compute_derived_vectors always engages sum/difference_ab/difference_ba/product, even when "
    "both quotients are undefined",
    "[ui][derived_vectors]") {
    constexpr Vec2 zero{0.0, 0.0};

    const DerivedVectors derived = compute_derived_vectors(zero, zero);

    CHECK(derived.sum.has_value());
    CHECK(derived.difference_ab.has_value());
    CHECK(derived.difference_ba.has_value());
    CHECK(derived.product.has_value());
    CHECK_FALSE(derived.quotient_ab.has_value());
    CHECK_FALSE(derived.quotient_ba.has_value());
}
