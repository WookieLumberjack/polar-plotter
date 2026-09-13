#include "ui/vector_palette.hpp"

#include <cassert>
#include <cstring>

namespace ui {
namespace {

// Normalize a 0-255 channel value to ImPlot/ImGui's [0, 1] color range.
constexpr float channel(int value_0_255) { return static_cast<float>(value_0_255) / 255.0F; }

}  // namespace

polarplot::MarkerColor vector_color(const char* label) {
    assert(label != nullptr);

    if (std::strcmp(label, "A") == 0) {
        return {channel(76), channel(114), channel(176), 1.0F};
    }
    if (std::strcmp(label, "B") == 0) {
        return {channel(221), channel(132), channel(82), 1.0F};
    }
    if (std::strcmp(label, "A + B") == 0) {
        return {channel(85), channel(168), channel(104), 1.0F};
    }
    if (std::strcmp(label, "A - B") == 0) {
        return {channel(196), channel(78), channel(82), 1.0F};
    }
    if (std::strcmp(label, "B - A") == 0) {
        return {channel(129), channel(114), channel(179), 1.0F};
    }
    if (std::strcmp(label, "A x B") == 0) {
        return {channel(218), channel(139), channel(195), 1.0F};
    }
    if (std::strcmp(label, "A / B") == 0) {
        return {channel(100), channel(181), channel(205), 1.0F};
    }
    if (std::strcmp(label, "B / A") == 0) {
        return {channel(204), channel(185), channel(116), 1.0F};
    }

    assert(false && "vector_color: label is not one of the 8 named vectors");
    return {};
}

}  // namespace ui
