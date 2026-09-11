#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ui/zero_direction.hpp"

using Catch::Matchers::WithinAbs;
using ui::plain_to_raw_zero_direction;
using ui::PlainZeroDirection;
using ui::raw_to_plain_zero_direction;
using ui::ZeroDirectionSide;

TEST_CASE("raw_to_plain_zero_direction: exactly at top is 0 deg, canonical right",
          "[ui][zero_direction]") {
    const PlainZeroDirection plain = raw_to_plain_zero_direction(90.0F);
    CHECK(plain.side == ZeroDirectionSide::kRight);
    CHECK_THAT(plain.degrees_from_top, WithinAbs(0.0, 1e-4));
}

TEST_CASE("raw_to_plain_zero_direction: raw angle above top reads as left",
          "[ui][zero_direction]") {
    const PlainZeroDirection plain = raw_to_plain_zero_direction(135.0F);
    CHECK(plain.side == ZeroDirectionSide::kLeft);
    CHECK_THAT(plain.degrees_from_top, WithinAbs(45.0, 1e-4));
}

TEST_CASE("raw_to_plain_zero_direction: raw angle below top reads as right",
          "[ui][zero_direction]") {
    const PlainZeroDirection plain = raw_to_plain_zero_direction(45.0F);
    CHECK(plain.side == ZeroDirectionSide::kRight);
    CHECK_THAT(plain.degrees_from_top, WithinAbs(45.0, 1e-4));
}

TEST_CASE("raw_to_plain_zero_direction: raw angle at +x axis (0 deg) is 90 right",
          "[ui][zero_direction]") {
    const PlainZeroDirection plain = raw_to_plain_zero_direction(0.0F);
    CHECK(plain.side == ZeroDirectionSide::kRight);
    CHECK_THAT(plain.degrees_from_top, WithinAbs(90.0, 1e-4));
}

TEST_CASE("raw_to_plain_zero_direction: wraps near 180 deg from top", "[ui][zero_direction]") {
    SECTION("directly opposite top via the left side (270 deg raw) is canonical left, 180") {
        const PlainZeroDirection plain = raw_to_plain_zero_direction(270.0F);
        CHECK(plain.side == ZeroDirectionSide::kLeft);
        CHECK_THAT(plain.degrees_from_top, WithinAbs(180.0, 1e-4));
    }

    SECTION(
        "directly opposite top via the right side (-90 deg raw) also normalizes to "
        "canonical left, 180") {
        const PlainZeroDirection plain = raw_to_plain_zero_direction(-90.0F);
        CHECK(plain.side == ZeroDirectionSide::kLeft);
        CHECK_THAT(plain.degrees_from_top, WithinAbs(180.0, 1e-4));
    }

    SECTION("just short of opposite-top from the left stays left, just under 180") {
        const PlainZeroDirection plain = raw_to_plain_zero_direction(269.0F);
        CHECK(plain.side == ZeroDirectionSide::kLeft);
        CHECK_THAT(plain.degrees_from_top, WithinAbs(179.0, 1e-4));
    }

    SECTION("just short of opposite-top from the right stays right, just under 180") {
        const PlainZeroDirection plain = raw_to_plain_zero_direction(-89.0F);
        CHECK(plain.side == ZeroDirectionSide::kRight);
        CHECK_THAT(plain.degrees_from_top, WithinAbs(179.0, 1e-4));
    }
}

TEST_CASE("raw_to_plain_zero_direction normalizes a raw angle beyond a full turn",
          "[ui][zero_direction]") {
    const PlainZeroDirection plain = raw_to_plain_zero_direction(135.0F + 360.0F);
    CHECK(plain.side == ZeroDirectionSide::kLeft);
    CHECK_THAT(plain.degrees_from_top, WithinAbs(45.0, 1e-4));
}

TEST_CASE("plain_to_raw_zero_direction is the inverse mapping", "[ui][zero_direction]") {
    CHECK_THAT(plain_to_raw_zero_direction({ZeroDirectionSide::kRight, 0.0F}),
               WithinAbs(90.0, 1e-4));
    CHECK_THAT(plain_to_raw_zero_direction({ZeroDirectionSide::kLeft, 45.0F}),
               WithinAbs(135.0, 1e-4));
    CHECK_THAT(plain_to_raw_zero_direction({ZeroDirectionSide::kRight, 45.0F}),
               WithinAbs(45.0, 1e-4));
    CHECK_THAT(plain_to_raw_zero_direction({ZeroDirectionSide::kLeft, 180.0F}),
               WithinAbs(270.0, 1e-4));
    CHECK_THAT(plain_to_raw_zero_direction({ZeroDirectionSide::kRight, 180.0F}),
               WithinAbs(-90.0, 1e-4));
}

TEST_CASE("raw -> plain -> raw round-trips for values already in principal range",
          "[ui][zero_direction]") {
    // raw_to_plain_zero_direction normalizes raw - 90 into (-180, 180], i.e. raw
    // into (-90, 270]; round-tripping recovers the exact raw only within that
    // principal range (outside it, the same angle mod 360 comes back instead).
    for (const float raw : {0.0F, 30.0F, 90.0F, 135.0F, 179.0F, 181.0F, 269.0F}) {
        const PlainZeroDirection plain = raw_to_plain_zero_direction(raw);
        const float round_tripped = plain_to_raw_zero_direction(plain);
        CAPTURE(raw);
        CHECK_THAT(static_cast<double>(round_tripped), WithinAbs(static_cast<double>(raw), 1e-3));
    }
}
