#include <algorithm>
#include <array>

#include <catch2/catch_test_macros.hpp>

#include "ui/theme.hpp"

using ui::kAllThemes;
using ui::scale_theme_style;
using ui::Theme;
using ui::theme_style;
using ui::ThemeStyle;

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

TEST_CASE("scale_theme_style scales only the rounding fields", "[theme]") {
    const ThemeStyle style = theme_style(Theme::kPaper);
    const ThemeStyle scaled = scale_theme_style(style, 1.5F);

    CHECK(scaled.window_rounding == style.window_rounding * 1.5F);
    CHECK(scaled.frame_rounding == style.frame_rounding * 1.5F);
    CHECK(scaled.grab_rounding == style.grab_rounding * 1.5F);

    // Everything else -- every color -- must pass through untouched.
    ThemeStyle expected_colors_only = style;
    expected_colors_only.window_rounding = scaled.window_rounding;
    expected_colors_only.frame_rounding = scaled.frame_rounding;
    expected_colors_only.grab_rounding = scaled.grab_rounding;
    CHECK(scaled == expected_colors_only);
}

TEST_CASE("scale_theme_style at 1.0 is the identity", "[theme]") {
    const ThemeStyle style = theme_style(Theme::kMidnight);
    CHECK(scale_theme_style(style, 1.0F) == style);
}
