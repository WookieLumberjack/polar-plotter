#include <algorithm>
#include <array>

#include <catch2/catch_test_macros.hpp>

#include "ui/theme.hpp"

using ui::Theme;
using ui::theme_style;
using ui::ThemeStyle;

namespace {

constexpr std::array<Theme, 5> kAllThemes{Theme::kSlate, Theme::kMidnight, Theme::kPaper,
                                          Theme::kNordLight, Theme::kMint};

}  // namespace

TEST_CASE("theme_style returns a distinct style for every Theme value", "[theme]") {
    for (std::size_t i = 0; i < kAllThemes.size(); ++i) {
        for (std::size_t j = i + 1; j < kAllThemes.size(); ++j) {
            CHECK_FALSE(theme_style(kAllThemes[i]) == theme_style(kAllThemes[j]));
        }
    }
}

TEST_CASE("light themes use larger rounding than dark themes", "[theme]") {
    constexpr std::array<Theme, 2> kDarkThemes{Theme::kSlate, Theme::kMidnight};
    constexpr std::array<Theme, 3> kLightThemes{Theme::kPaper, Theme::kNordLight, Theme::kMint};

    float max_dark_rounding = 0.0F;
    for (const Theme dark : kDarkThemes) {
        const ThemeStyle style = theme_style(dark);
        max_dark_rounding = std::max(
            {max_dark_rounding, style.window_rounding, style.frame_rounding, style.grab_rounding});
    }

    for (const Theme light : kLightThemes) {
        const ThemeStyle style = theme_style(light);
        CHECK(style.window_rounding > max_dark_rounding);
        CHECK(style.frame_rounding > max_dark_rounding);
        CHECK(style.grab_rounding > max_dark_rounding);
    }
}

TEST_CASE("kSlate and kMidnight keep sharp/default corner rounding", "[theme]") {
    CHECK(theme_style(Theme::kSlate).window_rounding == 0.0F);
    CHECK(theme_style(Theme::kMidnight).window_rounding == 0.0F);
}

TEST_CASE("theme_style is a pure function (same input, same output)", "[theme]") {
    CHECK(theme_style(Theme::kMint) == theme_style(Theme::kMint));
}
