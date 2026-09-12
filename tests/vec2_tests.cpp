#include <cmath>
#include <numbers>
#include <optional>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "vector_math/vec2.hpp"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using vecmath::Vec2;

namespace {
constexpr double kPi = std::numbers::pi;
}

TEST_CASE("component arithmetic", "[vec2]") {
    constexpr Vec2 a{3.0, 1.0};
    constexpr Vec2 b{-1.0, 2.0};

    STATIC_REQUIRE(a + b == Vec2{2.0, 3.0});
    STATIC_REQUIRE(a - b == Vec2{4.0, -1.0});
    STATIC_REQUIRE(-a == Vec2{-3.0, -1.0});
    STATIC_REQUIRE(a * 2.0 == Vec2{6.0, 2.0});
    STATIC_REQUIRE(2.0 * a == a * 2.0);
}

TEST_CASE("dot and cross products", "[vec2]") {
    constexpr Vec2 a{3.0, 1.0};
    constexpr Vec2 b{-1.0, 2.0};

    STATIC_REQUIRE(vecmath::dot(a, b) == -1.0);
    STATIC_REQUIRE(vecmath::cross(a, b) == 7.0);
    STATIC_REQUIRE(vecmath::dot(a, b) == vecmath::dot(b, a));

    SECTION("perpendicular vectors have zero dot product") {
        REQUIRE(vecmath::dot(Vec2{1.0, 0.0}, Vec2{0.0, 1.0}) == 0.0);
    }
}

TEST_CASE("magnitude", "[vec2]") {
    REQUIRE_THAT(vecmath::magnitude(Vec2{3.0, 4.0}), WithinRel(5.0));
    REQUIRE(vecmath::magnitude(Vec2{0.0, 0.0}) == 0.0);
    STATIC_REQUIRE(vecmath::magnitude_squared(Vec2{3.0, 4.0}) == 25.0);
}

TEST_CASE("angle is measured from the +x axis", "[vec2]") {
    REQUIRE_THAT(vecmath::angle(Vec2{1.0, 0.0}), WithinAbs(0.0, 1e-12));
    REQUIRE_THAT(vecmath::angle(Vec2{0.0, 1.0}), WithinAbs(kPi / 2.0, 1e-12));
    REQUIRE_THAT(vecmath::angle(Vec2{-1.0, 0.0}), WithinAbs(kPi, 1e-12));

    SECTION("zero vector has angle zero by convention") {
        REQUIRE(vecmath::angle(Vec2{0.0, 0.0}) == 0.0);
    }
}

TEST_CASE("polar round-trips to Cartesian", "[vec2][polar]") {
    const Vec2 v{3.0, -2.0};
    const vecmath::Polar p = vecmath::to_polar(v);
    const Vec2 back = vecmath::from_polar(p);

    REQUIRE_THAT(back.x, WithinAbs(v.x, 1e-12));
    REQUIRE_THAT(back.y, WithinAbs(v.y, 1e-12));
}

TEST_CASE("from_polar builds the expected vector", "[vec2][polar]") {
    const Vec2 v = vecmath::from_polar(2.0, kPi / 2.0);
    REQUIRE_THAT(v.x, WithinAbs(0.0, 1e-12));
    REQUIRE_THAT(v.y, WithinAbs(2.0, 1e-12));
}

TEST_CASE("normalized yields a unit vector", "[vec2]") {
    const Vec2 n = vecmath::normalized(Vec2{0.0, 5.0});
    REQUIRE_THAT(vecmath::magnitude(n), WithinRel(1.0));
    REQUIRE(n == Vec2{0.0, 1.0});

    SECTION("normalizing the zero vector returns zero, not NaN") {
        REQUIRE(vecmath::normalized(Vec2{0.0, 0.0}) == Vec2{0.0, 0.0});
    }
}

TEST_CASE("angle_between is symmetric and bounded", "[vec2]") {
    const Vec2 a{1.0, 0.0};
    const Vec2 b{0.0, 1.0};

    REQUIRE_THAT(vecmath::angle_between(a, b), WithinAbs(kPi / 2.0, 1e-12));
    REQUIRE_THAT(vecmath::angle_between(a, b), WithinAbs(vecmath::angle_between(b, a), 1e-12));
    REQUIRE_THAT(vecmath::angle_between(a, a), WithinAbs(0.0, 1e-9));
    REQUIRE_THAT(vecmath::angle_between(a, -a), WithinAbs(kPi, 1e-9));

    SECTION("degenerate input returns zero") {
        REQUIRE(vecmath::angle_between(Vec2{0.0, 0.0}, a) == 0.0);
    }
}

TEST_CASE("complex_multiply treats each Vec2 as a complex number", "[vec2][complex]") {
    const Vec2 a{3.0, 1.0};
    const Vec2 b{-1.0, 2.0};

    // (3+1i)(-1+2i) = (3*-1 - 1*2) + (3*2 + 1*-1)i = -5 + 5i
    const Vec2 product = vecmath::complex_multiply(a, b);
    REQUIRE_THAT(product.x, WithinAbs(-5.0, 1e-12));
    REQUIRE_THAT(product.y, WithinAbs(5.0, 1e-12));

    SECTION("amplitude multiplies and phase adds") {
        REQUIRE_THAT(vecmath::magnitude(product),
                     WithinRel(vecmath::magnitude(a) * vecmath::magnitude(b)));

        const double expected_angle = vecmath::angle(a) + vecmath::angle(b);
        REQUIRE_THAT(std::cos(vecmath::angle(product)), WithinAbs(std::cos(expected_angle), 1e-12));
        REQUIRE_THAT(std::sin(vecmath::angle(product)), WithinAbs(std::sin(expected_angle), 1e-12));
    }
}

TEST_CASE("complex_divide treats each Vec2 as a complex number", "[vec2][complex]") {
    const Vec2 a{3.0, 1.0};
    const Vec2 b{-1.0, 2.0};

    // (3+1i)/(-1+2i) = -1/5 - 7/5 i
    const std::optional<Vec2> quotient = vecmath::complex_divide(a, b);
    REQUIRE(quotient.has_value());
    REQUIRE_THAT(quotient->x, WithinAbs(-0.2, 1e-12));
    REQUIRE_THAT(quotient->y, WithinAbs(-1.4, 1e-12));

    SECTION("amplitude divides and phase subtracts") {
        REQUIRE_THAT(vecmath::magnitude(*quotient),
                     WithinRel(vecmath::magnitude(a) / vecmath::magnitude(b)));

        const double expected_angle = vecmath::angle(a) - vecmath::angle(b);
        REQUIRE_THAT(std::cos(vecmath::angle(*quotient)),
                     WithinAbs(std::cos(expected_angle), 1e-12));
        REQUIRE_THAT(std::sin(vecmath::angle(*quotient)),
                     WithinAbs(std::sin(expected_angle), 1e-12));
    }

    SECTION("zero-magnitude divisor is undefined") {
        REQUIRE_FALSE(vecmath::complex_divide(a, Vec2{0.0, 0.0}).has_value());
    }
}

TEST_CASE("vector difference matches the parallelogram rule", "[vec2]") {
    const Vec2 a{5.0, 2.0};
    const Vec2 b{1.0, 3.0};
    const Vec2 diff = a - b;

    REQUIRE(diff + b == a);
    REQUIRE_THAT(vecmath::magnitude(diff), WithinRel(std::sqrt(17.0)));
}
