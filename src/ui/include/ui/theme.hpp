#ifndef UI_THEME_HPP
#define UI_THEME_HPP

#include <array>
#include <cstdint>

namespace ui {

/// The application's built-in visual themes. kSlate reproduces today's
/// default ImGui look exactly (baseline, not a regression); kMidnight is
/// dark with the same sharp/default corner rounding, while kPaper/kNordLight/
/// kMint are light with noticeably larger rounding for a more modern look.
enum class Theme : std::uint8_t { kSlate, kMidnight, kPaper, kNordLight, kMint };

/// Every Theme value, in declaration order. The single source of truth for
/// "all themes" -- callers that need to enumerate them (the Theme menu,
/// serialization round-trip tests, ...) iterate this instead of each keeping
/// their own copy of the value list, so adding a theme only means adding it
/// here plus the per-theme switches (theme_style/theme_label/theme_name).
inline constexpr std::array<Theme, 5> kAllThemes{Theme::kSlate, Theme::kMidnight, Theme::kPaper,
                                                 Theme::kNordLight, Theme::kMint};

/// One RGBA color, componentwise, in the [0, 1] range ImGui colors use.
/// Kept as plain floats (rather than ImGui's ImVec4) so ThemeStyle stays
/// trivially comparable for tests without depending on ImVec4's equality.
struct ThemeColor {
    float r{0.0F};
    float g{0.0F};
    float b{0.0F};
    float a{1.0F};

    friend bool operator==(const ThemeColor&, const ThemeColor&) = default;
};

/// The handful of ImGuiStyle colors/rounding fields this app actually
/// customizes for a theme -- not a full ImGuiStyle copy. See ui::apply_theme,
/// which pushes these into ImGui::GetStyle() once whenever the selection
/// changes.
struct ThemeStyle {
    float window_rounding{0.0F};
    float frame_rounding{0.0F};
    float grab_rounding{0.0F};
    ThemeColor text{};
    ThemeColor window_bg{};
    ThemeColor frame_bg{};
    // A docked panel's title bar/tab strip when it does NOT have input focus
    // (e.g. neither panel has been clicked into yet, or focus is on the
    // other one) -- distinct from title_bg_active below, which only covers
    // the focused case. See apply_theme: Dear ImGui's own dark-theme
    // constructor derives the Tab/TabDimmed/TabDimmedSelected family from
    // this plus title_bg_active/header once at startup and never
    // recomputes them, so apply_theme must re-derive them the same way
    // whenever the theme changes, or unfocused panels stay stuck at the
    // original (dark) derived colors under every theme.
    ThemeColor title_bg{};
    ThemeColor title_bg_active{};
    // Backdrop behind the main menu bar (see the File/Theme menu bar in the
    // dockspace host window) and behind popups/tooltips -- Dear ImGui and
    // ImPlot's own StyleColorsAuto() (legend background, e.g.) both key off
    // these rather than window_bg, so a theme that leaves them unset shows
    // the wrong-contrast default backdrop under text this struct does color.
    ThemeColor menu_bar_bg{};
    ThemeColor popup_bg{};
    ThemeColor header{};
    ThemeColor header_hovered{};
    ThemeColor button{};
    ThemeColor button_hovered{};
    ThemeColor button_active{};
    ThemeColor tab_selected{};

    friend bool operator==(const ThemeStyle&, const ThemeStyle&) = default;
};

/// Pure mapping from a Theme to the style values it applies. Does not touch
/// any live ImGui context -- see ui::apply_theme for that.
[[nodiscard]] ThemeStyle theme_style(Theme theme);

/// Push \p style's colors/rounding fields into ImGui::GetStyle(). Call once
/// when the selection changes, not every frame -- there is a live ImGui
/// context by the time this runs.
void apply_theme(const ThemeStyle& style);

/// The theme's display name, for menu items ("Slate", "Midnight", ...).
[[nodiscard]] const char* theme_label(Theme theme);

}  // namespace ui

#endif  // UI_THEME_HPP
