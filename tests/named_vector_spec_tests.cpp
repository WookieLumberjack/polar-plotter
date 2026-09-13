#include <array>
#include <cstddef>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "polar_plotting/polar_plot.hpp"
#include "ui/named_vector_spec.hpp"

using polarplot::MarkerColor;
using ui::kNamedVectorSpecs;
using ui::named_vector_color;

namespace {

bool color_eq(MarkerColor lhs, MarkerColor rhs) {
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

}  // namespace

TEST_CASE("named_vector_color maps each named vector's exact label to its fixed palette color",
          "[ui][named_vector_spec]") {
    CHECK(color_eq(named_vector_color("A"),
                   MarkerColor{76.0F / 255.0F, 114.0F / 255.0F, 176.0F / 255.0F, 1.0F}));
    CHECK(color_eq(named_vector_color("B"),
                   MarkerColor{221.0F / 255.0F, 132.0F / 255.0F, 82.0F / 255.0F, 1.0F}));
    CHECK(color_eq(named_vector_color("A + B"),
                   MarkerColor{85.0F / 255.0F, 168.0F / 255.0F, 104.0F / 255.0F, 1.0F}));
    CHECK(color_eq(named_vector_color("A - B"),
                   MarkerColor{196.0F / 255.0F, 78.0F / 255.0F, 82.0F / 255.0F, 1.0F}));
    CHECK(color_eq(named_vector_color("B - A"),
                   MarkerColor{129.0F / 255.0F, 114.0F / 255.0F, 179.0F / 255.0F, 1.0F}));
    CHECK(color_eq(named_vector_color("A x B"),
                   MarkerColor{218.0F / 255.0F, 139.0F / 255.0F, 195.0F / 255.0F, 1.0F}));
    CHECK(color_eq(named_vector_color("A / B"),
                   MarkerColor{100.0F / 255.0F, 181.0F / 255.0F, 205.0F / 255.0F, 1.0F}));
    CHECK(color_eq(named_vector_color("B / A"),
                   MarkerColor{204.0F / 255.0F, 185.0F / 255.0F, 116.0F / 255.0F, 1.0F}));
}

TEST_CASE("named_vector_color leaves A and B unchanged from today's auto-assigned values",
          "[ui][named_vector_spec]") {
    // Today's ImPlot default colormap (Deep) auto-assigns the first two
    // items' colors to exactly these RGB values; this palette pins A and B
    // to that same look rather than changing it.
    const MarkerColor a = named_vector_color("A");
    const MarkerColor b = named_vector_color("B");

    CHECK(color_eq(a, MarkerColor{76.0F / 255.0F, 114.0F / 255.0F, 176.0F / 255.0F, 1.0F}));
    CHECK(color_eq(b, MarkerColor{221.0F / 255.0F, 132.0F / 255.0F, 82.0F / 255.0F, 1.0F}));
}

TEST_CASE("kNamedVectorSpecs names all 8 named vectors in the documented order",
          "[ui][named_vector_spec]") {
    REQUIRE(kNamedVectorSpecs.size() == 8);

    const std::array<std::string, 8> expected_labels{
        "A", "B", "A - B", "A + B", "B - A", "A x B", "A / B", "B / A",
    };
    for (std::size_t i = 0; i < expected_labels.size(); ++i) {
        CHECK(expected_labels[i] == kNamedVectorSpecs[i].label);
    }
}

TEST_CASE("kNamedVectorSpecs: A and B carry no accessor or toggle", "[ui][named_vector_spec]") {
    CHECK(kNamedVectorSpecs[0].accessor == nullptr);
    CHECK(kNamedVectorSpecs[0].toggle == nullptr);
    CHECK(kNamedVectorSpecs[1].accessor == nullptr);
    CHECK(kNamedVectorSpecs[1].toggle == nullptr);
}

TEST_CASE("kNamedVectorSpecs: every derived vector carries both an accessor and a toggle",
          "[ui][named_vector_spec]") {
    for (std::size_t i = 2; i < kNamedVectorSpecs.size(); ++i) {
        CHECK(kNamedVectorSpecs[i].accessor != nullptr);
        CHECK(kNamedVectorSpecs[i].toggle != nullptr);
    }
}
