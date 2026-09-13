#include <catch2/catch_test_macros.hpp>

#include "polar_plotting/polar_plot.hpp"
#include "ui/vector_palette.hpp"

using polarplot::MarkerColor;
using ui::vector_color;

namespace {

bool color_eq(MarkerColor lhs, MarkerColor rhs) {
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

}  // namespace

TEST_CASE("vector_color maps each named vector's exact label to its fixed palette color",
          "[ui][vector_palette]") {
    CHECK(color_eq(vector_color("A"),
                   MarkerColor{76.0F / 255.0F, 114.0F / 255.0F, 176.0F / 255.0F, 1.0F}));
    CHECK(color_eq(vector_color("B"),
                   MarkerColor{221.0F / 255.0F, 132.0F / 255.0F, 82.0F / 255.0F, 1.0F}));
    CHECK(color_eq(vector_color("A + B"),
                   MarkerColor{85.0F / 255.0F, 168.0F / 255.0F, 104.0F / 255.0F, 1.0F}));
    CHECK(color_eq(vector_color("A - B"),
                   MarkerColor{196.0F / 255.0F, 78.0F / 255.0F, 82.0F / 255.0F, 1.0F}));
    CHECK(color_eq(vector_color("B - A"),
                   MarkerColor{129.0F / 255.0F, 114.0F / 255.0F, 179.0F / 255.0F, 1.0F}));
    CHECK(color_eq(vector_color("A x B"),
                   MarkerColor{218.0F / 255.0F, 139.0F / 255.0F, 195.0F / 255.0F, 1.0F}));
    CHECK(color_eq(vector_color("A / B"),
                   MarkerColor{100.0F / 255.0F, 181.0F / 255.0F, 205.0F / 255.0F, 1.0F}));
    CHECK(color_eq(vector_color("B / A"),
                   MarkerColor{204.0F / 255.0F, 185.0F / 255.0F, 116.0F / 255.0F, 1.0F}));
}

TEST_CASE("vector_color leaves A and B unchanged from today's auto-assigned values",
          "[ui][vector_palette]") {
    // Today's ImPlot default colormap (Deep) auto-assigns the first two
    // items' colors to exactly these RGB values; this palette pins A and B
    // to that same look rather than changing it.
    const MarkerColor a = vector_color("A");
    const MarkerColor b = vector_color("B");

    CHECK(color_eq(a, MarkerColor{76.0F / 255.0F, 114.0F / 255.0F, 176.0F / 255.0F, 1.0F}));
    CHECK(color_eq(b, MarkerColor{221.0F / 255.0F, 132.0F / 255.0F, 82.0F / 255.0F, 1.0F}));
}
