#include <catch2/catch_test_macros.hpp>

#include "ui/construction.hpp"
#include "vector_math/vec2.hpp"

using ui::ConstructionVector;
using ui::SumConstruction;
using vecmath::Vec2;

TEST_CASE("tip_to_tail_sum builds both parallelogram paths", "[construction]") {
    constexpr Vec2 a{3.0, 1.0};
    constexpr Vec2 b{-1.0, 2.0};

    constexpr SumConstruction construction = ui::tip_to_tail_sum(a, b);

    SECTION("b starts at a's tip") {
        STATIC_REQUIRE(construction.b_from_a_tip.start == a);
        STATIC_REQUIRE(construction.b_from_a_tip.vector == b);
    }

    SECTION("a starts at b's tip") {
        STATIC_REQUIRE(construction.a_from_b_tip.start == b);
        STATIC_REQUIRE(construction.a_from_b_tip.vector == a);
    }

    SECTION("both paths arrive at the same resultant a + b") {
        constexpr Vec2 sum = a + b;
        STATIC_REQUIRE(construction.b_from_a_tip.start + construction.b_from_a_tip.vector == sum);
        STATIC_REQUIRE(construction.a_from_b_tip.start + construction.a_from_b_tip.vector == sum);
    }
}

TEST_CASE("tip_to_tail_sum handles the degenerate a == b case", "[construction]") {
    constexpr Vec2 a{2.0, -4.0};

    constexpr SumConstruction construction = ui::tip_to_tail_sum(a, a);

    STATIC_REQUIRE(construction.b_from_a_tip == ConstructionVector{.start = a, .vector = a});
    STATIC_REQUIRE(construction.a_from_b_tip == ConstructionVector{.start = a, .vector = a});
}

TEST_CASE("tip_to_tail_difference builds -b from a's tip", "[construction]") {
    constexpr Vec2 a{5.0, 2.0};
    constexpr Vec2 b{1.0, 3.0};

    constexpr ConstructionVector construction = ui::tip_to_tail_difference(a, b);

    STATIC_REQUIRE(construction.start == a);
    STATIC_REQUIRE(construction.vector == -b);

    SECTION("arrives at the resultant a - b") {
        STATIC_REQUIRE(construction.start + construction.vector == a - b);
    }
}

TEST_CASE("tip_to_tail_difference handles the degenerate a == b case", "[construction]") {
    constexpr Vec2 a{7.0, -3.0};

    constexpr ConstructionVector construction = ui::tip_to_tail_difference(a, a);

    STATIC_REQUIRE(construction.start == a);
    STATIC_REQUIRE(construction.vector == -a);
    // a - b is the zero vector when a == b: the construction still arrives
    // there rather than producing a NaN/degenerate arrow.
    STATIC_REQUIRE(construction.start + construction.vector == Vec2{0.0, 0.0});
}

TEST_CASE("difference_segment connects b's tip to a's tip", "[construction]") {
    constexpr Vec2 a{5.0, 2.0};
    constexpr Vec2 b{1.0, 3.0};

    constexpr ConstructionVector segment = ui::difference_segment(a, b);

    STATIC_REQUIRE(segment.start == b);
    STATIC_REQUIRE(segment.vector == a - b);

    SECTION("arrives at a's tip") { STATIC_REQUIRE(segment.start + segment.vector == a); }

    SECTION("is congruent to a - b") { STATIC_REQUIRE(segment.vector == a - b); }
}

TEST_CASE("difference_segment handles the degenerate a == b case", "[construction]") {
    constexpr Vec2 a{7.0, -3.0};

    constexpr ConstructionVector segment = ui::difference_segment(a, a);

    STATIC_REQUIRE(segment.start == a);
    STATIC_REQUIRE(segment.vector == Vec2{0.0, 0.0});
    STATIC_REQUIRE(segment.start + segment.vector == a);
}

TEST_CASE("tip_to_tail_difference_ba builds -a from b's tip", "[construction]") {
    constexpr Vec2 a{5.0, 2.0};
    constexpr Vec2 b{1.0, 3.0};

    constexpr ConstructionVector construction = ui::tip_to_tail_difference_ba(a, b);

    STATIC_REQUIRE(construction.start == b);
    STATIC_REQUIRE(construction.vector == -a);

    SECTION("arrives at the resultant b - a") {
        STATIC_REQUIRE(construction.start + construction.vector == b - a);
    }
}

TEST_CASE("tip_to_tail_difference_ba handles the degenerate a == b case", "[construction]") {
    constexpr Vec2 a{7.0, -3.0};

    constexpr ConstructionVector construction = ui::tip_to_tail_difference_ba(a, a);

    STATIC_REQUIRE(construction.start == a);
    STATIC_REQUIRE(construction.vector == -a);
    // b - a is the zero vector when a == b: the construction still arrives
    // there rather than producing a NaN/degenerate arrow.
    STATIC_REQUIRE(construction.start + construction.vector == Vec2{0.0, 0.0});
}

TEST_CASE("difference_segment_ba connects a's tip to b's tip", "[construction]") {
    constexpr Vec2 a{5.0, 2.0};
    constexpr Vec2 b{1.0, 3.0};

    constexpr ConstructionVector segment = ui::difference_segment_ba(a, b);

    STATIC_REQUIRE(segment.start == a);
    STATIC_REQUIRE(segment.vector == b - a);

    SECTION("arrives at b's tip") { STATIC_REQUIRE(segment.start + segment.vector == b); }

    SECTION("is congruent to b - a") { STATIC_REQUIRE(segment.vector == b - a); }
}

TEST_CASE("difference_segment_ba handles the degenerate a == b case", "[construction]") {
    constexpr Vec2 a{7.0, -3.0};

    constexpr ConstructionVector segment = ui::difference_segment_ba(a, a);

    STATIC_REQUIRE(segment.start == a);
    STATIC_REQUIRE(segment.vector == Vec2{0.0, 0.0});
    STATIC_REQUIRE(segment.start + segment.vector == a);
}
