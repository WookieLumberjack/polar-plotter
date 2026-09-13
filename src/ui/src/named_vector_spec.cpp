#include "ui/named_vector_spec.hpp"

#include <cassert>
#include <cstring>

namespace ui {
namespace {

// Normalize a 0-255 channel value to ImPlot/ImGui's [0, 1] color range.
constexpr float channel(int value_0_255) { return static_cast<float>(value_0_255) / 255.0F; }

}  // namespace

const std::array<NamedVectorSpec, 8> kNamedVectorSpecs{{
    {"A", {channel(76), channel(114), channel(176), 1.0F}, nullptr, nullptr},
    {"B", {channel(221), channel(132), channel(82), 1.0F}, nullptr, nullptr},
    {"A - B",
     {channel(196), channel(78), channel(82), 1.0F},
     &DerivedVectors::difference_ab,
     &PlotInputs::show_difference},
    {"A + B",
     {channel(85), channel(168), channel(104), 1.0F},
     &DerivedVectors::sum,
     &PlotInputs::show_sum},
    {"B - A",
     {channel(129), channel(114), channel(179), 1.0F},
     &DerivedVectors::difference_ba,
     &PlotInputs::show_difference_ba},
    {"A x B",
     {channel(218), channel(139), channel(195), 1.0F},
     &DerivedVectors::product,
     &PlotInputs::show_product},
    {"A / B",
     {channel(100), channel(181), channel(205), 1.0F},
     &DerivedVectors::quotient_ab,
     &PlotInputs::show_quotient_ab},
    {"B / A",
     {channel(204), channel(185), channel(116), 1.0F},
     &DerivedVectors::quotient_ba,
     &PlotInputs::show_quotient_ba},
}};

polarplot::MarkerColor named_vector_color(const char* label) {
    assert(label != nullptr);

    for (const NamedVectorSpec& spec : kNamedVectorSpecs) {
        if (std::strcmp(spec.label, label) == 0) {
            return spec.color;
        }
    }

    assert(false && "named_vector_color: label is not one of the 8 named vectors");
    return {};
}

}  // namespace ui
