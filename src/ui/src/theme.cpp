#include "ui/theme.hpp"

#include <imgui.h>

namespace ui {
namespace {

ThemeColor to_theme_color(const ImVec4& c) { return {c.x, c.y, c.z, c.w}; }

// kSlate reproduces today's default ImGui look exactly: a fresh, default-
// constructed ImGuiStyle (no live context required -- it's a plain struct
// whose constructor just fills in ImGui's built-in dark theme and zero
// rounding) is the baseline every other theme is judged against.
ThemeStyle slate_style() {
    const ImGuiStyle defaults{};
    return ThemeStyle{
        .window_rounding = defaults.WindowRounding,
        .frame_rounding = defaults.FrameRounding,
        .grab_rounding = defaults.GrabRounding,
        .text = to_theme_color(defaults.Colors[ImGuiCol_Text]),
        .window_bg = to_theme_color(defaults.Colors[ImGuiCol_WindowBg]),
        .frame_bg = to_theme_color(defaults.Colors[ImGuiCol_FrameBg]),
        .title_bg_active = to_theme_color(defaults.Colors[ImGuiCol_TitleBgActive]),
        .header = to_theme_color(defaults.Colors[ImGuiCol_Header]),
        .header_hovered = to_theme_color(defaults.Colors[ImGuiCol_HeaderHovered]),
        .button = to_theme_color(defaults.Colors[ImGuiCol_Button]),
        .button_hovered = to_theme_color(defaults.Colors[ImGuiCol_ButtonHovered]),
        .button_active = to_theme_color(defaults.Colors[ImGuiCol_ButtonActive]),
        .tab_selected = to_theme_color(defaults.Colors[ImGuiCol_TabSelected]),
    };
}

// Dark, same sharp/default corner rounding as kSlate -- a deep navy blue
// palette rather than ImGui's default near-black/grey.
ThemeStyle midnight_style() {
    return ThemeStyle{
        .window_rounding = 0.0F,
        .frame_rounding = 0.0F,
        .grab_rounding = 0.0F,
        .text = {0.92F, 0.93F, 0.95F, 1.00F},
        .window_bg = {0.04F, 0.06F, 0.10F, 0.94F},
        .frame_bg = {0.10F, 0.14F, 0.22F, 0.60F},
        .title_bg_active = {0.06F, 0.09F, 0.16F, 1.00F},
        .header = {0.14F, 0.20F, 0.34F, 1.00F},
        .header_hovered = {0.18F, 0.26F, 0.42F, 1.00F},
        .button = {0.14F, 0.20F, 0.34F, 1.00F},
        .button_hovered = {0.18F, 0.26F, 0.42F, 1.00F},
        .button_active = {0.22F, 0.32F, 0.50F, 1.00F},
        .tab_selected = {0.14F, 0.20F, 0.34F, 1.00F},
    };
}

// Light, warm off-white "paper" palette with noticeably larger rounding.
ThemeStyle paper_style() {
    return ThemeStyle{
        .window_rounding = 8.0F,
        .frame_rounding = 6.0F,
        .grab_rounding = 6.0F,
        .text = {0.15F, 0.13F, 0.10F, 1.00F},
        .window_bg = {0.96F, 0.95F, 0.92F, 1.00F},
        .frame_bg = {0.88F, 0.86F, 0.80F, 1.00F},
        .title_bg_active = {0.90F, 0.87F, 0.80F, 1.00F},
        .header = {0.80F, 0.76F, 0.66F, 1.00F},
        .header_hovered = {0.85F, 0.81F, 0.70F, 1.00F},
        .button = {0.82F, 0.78F, 0.68F, 1.00F},
        .button_hovered = {0.87F, 0.83F, 0.72F, 1.00F},
        .button_active = {0.90F, 0.86F, 0.74F, 1.00F},
        .tab_selected = {0.85F, 0.81F, 0.70F, 1.00F},
    };
}

// Light, Nord-inspired frost/snow palette with larger rounding.
ThemeStyle nord_light_style() {
    return ThemeStyle{
        .window_rounding = 8.0F,
        .frame_rounding = 6.0F,
        .grab_rounding = 6.0F,
        .text = {0.18F, 0.20F, 0.25F, 1.00F},
        .window_bg = {0.925F, 0.937F, 0.957F, 1.00F},
        .frame_bg = {0.85F, 0.87F, 0.91F, 1.00F},
        .title_bg_active = {0.80F, 0.86F, 0.94F, 1.00F},
        .header = {0.53F, 0.75F, 0.82F, 1.00F},
        .header_hovered = {0.60F, 0.80F, 0.86F, 1.00F},
        .button = {0.53F, 0.75F, 0.82F, 1.00F},
        .button_hovered = {0.60F, 0.80F, 0.86F, 1.00F},
        .button_active = {0.37F, 0.51F, 0.67F, 1.00F},
        .tab_selected = {0.53F, 0.75F, 0.82F, 1.00F},
    };
}

// Light, minty-green palette with larger rounding.
ThemeStyle mint_style() {
    return ThemeStyle{
        .window_rounding = 8.0F,
        .frame_rounding = 6.0F,
        .grab_rounding = 6.0F,
        .text = {0.10F, 0.20F, 0.16F, 1.00F},
        .window_bg = {0.93F, 0.98F, 0.95F, 1.00F},
        .frame_bg = {0.80F, 0.93F, 0.85F, 1.00F},
        .title_bg_active = {0.75F, 0.90F, 0.80F, 1.00F},
        .header = {0.40F, 0.78F, 0.60F, 1.00F},
        .header_hovered = {0.46F, 0.84F, 0.66F, 1.00F},
        .button = {0.40F, 0.78F, 0.60F, 1.00F},
        .button_hovered = {0.46F, 0.84F, 0.66F, 1.00F},
        .button_active = {0.30F, 0.68F, 0.50F, 1.00F},
        .tab_selected = {0.40F, 0.78F, 0.60F, 1.00F},
    };
}

}  // namespace

ThemeStyle theme_style(Theme theme) {
    switch (theme) {
        case Theme::kSlate:
            return slate_style();
        case Theme::kMidnight:
            return midnight_style();
        case Theme::kPaper:
            return paper_style();
        case Theme::kNordLight:
            return nord_light_style();
        case Theme::kMint:
            return mint_style();
    }
    return slate_style();
}

void apply_theme(const ThemeStyle& style) {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = style.window_rounding;
    s.FrameRounding = style.frame_rounding;
    s.GrabRounding = style.grab_rounding;

    const auto to_imvec4 = [](const ThemeColor& c) { return ImVec4(c.r, c.g, c.b, c.a); };
    s.Colors[ImGuiCol_Text] = to_imvec4(style.text);
    s.Colors[ImGuiCol_WindowBg] = to_imvec4(style.window_bg);
    s.Colors[ImGuiCol_FrameBg] = to_imvec4(style.frame_bg);
    s.Colors[ImGuiCol_TitleBgActive] = to_imvec4(style.title_bg_active);
    s.Colors[ImGuiCol_Header] = to_imvec4(style.header);
    s.Colors[ImGuiCol_HeaderHovered] = to_imvec4(style.header_hovered);
    s.Colors[ImGuiCol_Button] = to_imvec4(style.button);
    s.Colors[ImGuiCol_ButtonHovered] = to_imvec4(style.button_hovered);
    s.Colors[ImGuiCol_ButtonActive] = to_imvec4(style.button_active);
    s.Colors[ImGuiCol_TabSelected] = to_imvec4(style.tab_selected);
}

const char* theme_label(Theme theme) {
    switch (theme) {
        case Theme::kSlate:
            return "Slate";
        case Theme::kMidnight:
            return "Midnight";
        case Theme::kPaper:
            return "Paper";
        case Theme::kNordLight:
            return "Nord Light";
        case Theme::kMint:
            return "Mint";
    }
    return "Slate";
}

}  // namespace ui
