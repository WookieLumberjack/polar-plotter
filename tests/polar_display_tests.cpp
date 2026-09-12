#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ui/polar_display.hpp"
#include "vector_math/vec2.hpp"

using Catch::Matchers::WithinAbs;
using ui::canonicalize_polar_display;
using ui::from_polar_display;
using ui::PolarDisplay;
using ui::to_polar_display;
using vecmath::Vec2;

TEST_CASE("to_polar_display reads amplitude and phase normalized to 0-360 deg",
          "[ui][polar_display]") {
    CHECK_THAT(to_polar_display(Vec2{1.0, 0.0}).amplitude, WithinAbs(1.0, 1e-4));
    CHECK_THAT(to_polar_display(Vec2{1.0, 0.0}).phase_deg, WithinAbs(0.0, 1e-4));

    CHECK_THAT(to_polar_display(Vec2{0.0, 5.0}).amplitude, WithinAbs(5.0, 1e-4));
    CHECK_THAT(to_polar_display(Vec2{0.0, 5.0}).phase_deg, WithinAbs(90.0, 1e-4));

    CHECK_THAT(to_polar_display(Vec2{-1.0, 0.0}).phase_deg, WithinAbs(180.0, 1e-4));

    SECTION("negative-y phase normalizes into 0-360 range, not negative") {
        CHECK_THAT(to_polar_display(Vec2{0.0, -5.0}).phase_deg, WithinAbs(270.0, 1e-4));
    }

    SECTION("zero vector is canonical zero amplitude/phase") {
        CHECK_THAT(to_polar_display(Vec2{0.0, 0.0}).amplitude, WithinAbs(0.0, 1e-4));
        CHECK_THAT(to_polar_display(Vec2{0.0, 0.0}).phase_deg, WithinAbs(0.0, 1e-4));
    }
}

TEST_CASE("from_polar_display recovers Cartesian components", "[ui][polar_display]") {
    const Vec2 v = from_polar_display(PolarDisplay{5.0F, 90.0F});
    CHECK_THAT(v.x, WithinAbs(0.0, 1e-4));
    CHECK_THAT(v.y, WithinAbs(5.0, 1e-4));
}

TEST_CASE("to_polar_display and from_polar_display round-trip", "[ui][polar_display]") {
    for (const Vec2 v : {Vec2{3.0, 4.0}, Vec2{-2.0, 1.0}, Vec2{-1.0, -1.0}, Vec2{0.0, -6.0}}) {
        const PolarDisplay display = to_polar_display(v);
        const Vec2 round_tripped = from_polar_display(display);
        CAPTURE(v.x, v.y);
        CHECK_THAT(round_tripped.x, WithinAbs(v.x, 1e-4));
        CHECK_THAT(round_tripped.y, WithinAbs(v.y, 1e-4));
    }
}

TEST_CASE("from_polar_display treats negative amplitude as a 180 deg direction flip",
          "[ui][polar_display]") {
    const Vec2 negated = from_polar_display(PolarDisplay{-5.0F, 90.0F});
    const Vec2 flipped = from_polar_display(PolarDisplay{5.0F, 270.0F});
    CHECK_THAT(negated.x, WithinAbs(flipped.x, 1e-4));
    CHECK_THAT(negated.y, WithinAbs(flipped.y, 1e-4));
}

TEST_CASE("canonicalize_polar_display snaps negative amplitude to non-negative + adjusted phase",
          "[ui][polar_display]") {
    const PolarDisplay canonical = canonicalize_polar_display(PolarDisplay{-5.0F, 90.0F});
    CHECK_THAT(canonical.amplitude, WithinAbs(5.0, 1e-4));
    CHECK_THAT(canonical.phase_deg, WithinAbs(270.0, 1e-4));

    SECTION("wraps the adjusted phase back into 0-360 range") {
        const PolarDisplay wrapped = canonicalize_polar_display(PolarDisplay{-5.0F, 350.0F});
        CHECK_THAT(wrapped.amplitude, WithinAbs(5.0, 1e-4));
        CHECK_THAT(wrapped.phase_deg, WithinAbs(170.0, 1e-4));
    }

    SECTION("non-negative amplitude is left unchanged aside from phase normalization") {
        const PolarDisplay unchanged = canonicalize_polar_display(PolarDisplay{5.0F, 400.0F});
        CHECK_THAT(unchanged.amplitude, WithinAbs(5.0, 1e-4));
        CHECK_THAT(unchanged.phase_deg, WithinAbs(40.0, 1e-4));
    }
}
