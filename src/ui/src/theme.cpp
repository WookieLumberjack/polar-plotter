#include "ui/theme.hpp"

#include <imgui.h>

namespace ui {
namespace {

ThemeColor to_theme_color(const ImVec4& c) { return {c.x, c.y, c.z, c.w}; }

ImVec4 lerp(const ImVec4& a, const ImVec4& b, float t) {
    return {a.x + ((b.x - a.x) * t), a.y + ((b.y - a.y) * t), a.z + ((b.z - a.z) * t),
            a.w + ((b.w - a.w) * t)};
}

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
        .title_bg = to_theme_color(defaults.Colors[ImGuiCol_TitleBg]),
        .title_bg_active = to_theme_color(defaults.Colors[ImGuiCol_TitleBgActive]),
        .menu_bar_bg = to_theme_color(defaults.Colors[ImGuiCol_MenuBarBg]),
        .popup_bg = to_theme_color(defaults.Colors[ImGuiCol_PopupBg]),
        .header = to_theme_color(defaults.Colors[ImGuiCol_Header]),
        .header_hovered = to_theme_color(defaults.Colors[ImGuiCol_HeaderHovered]),
        .button = to_theme_color(defaults.Colors[ImGuiCol_Button]),
        .button_hovered = to_theme_color(defaults.Colors[ImGuiCol_ButtonHovered]),
        .button_active = to_theme_color(defaults.Colors[ImGuiCol_ButtonActive]),
        .tab_selected = to_theme_color(defaults.Colors[ImGuiCol_TabSelected]),
        .table_header_bg = to_theme_color(defaults.Colors[ImGuiCol_TableHeaderBg]),
        .table_border = to_theme_color(defaults.Colors[ImGuiCol_TableBorderStrong]),
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
        .title_bg = {0.04F, 0.06F, 0.10F, 0.94F},
        .title_bg_active = {0.06F, 0.09F, 0.16F, 1.00F},
        .menu_bar_bg = {0.06F, 0.09F, 0.16F, 1.00F},
        .popup_bg = {0.05F, 0.08F, 0.13F, 0.96F},
        .header = {0.14F, 0.20F, 0.34F, 1.00F},
        .header_hovered = {0.18F, 0.26F, 0.42F, 1.00F},
        .button = {0.14F, 0.20F, 0.34F, 1.00F},
        .button_hovered = {0.18F, 0.26F, 0.42F, 1.00F},
        .button_active = {0.22F, 0.32F, 0.50F, 1.00F},
        .tab_selected = {0.14F, 0.20F, 0.34F, 1.00F},
        .table_header_bg = {0.10F, 0.14F, 0.22F, 1.00F},
        .table_border = {0.20F, 0.26F, 0.38F, 1.00F},
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
        .title_bg = {0.96F, 0.95F, 0.92F, 1.00F},
        .title_bg_active = {0.90F, 0.87F, 0.80F, 1.00F},
        .menu_bar_bg = {0.90F, 0.87F, 0.80F, 1.00F},
        .popup_bg = {0.98F, 0.97F, 0.94F, 0.98F},
        .header = {0.80F, 0.76F, 0.66F, 1.00F},
        .header_hovered = {0.85F, 0.81F, 0.70F, 1.00F},
        .button = {0.82F, 0.78F, 0.68F, 1.00F},
        .button_hovered = {0.87F, 0.83F, 0.72F, 1.00F},
        .button_active = {0.90F, 0.86F, 0.74F, 1.00F},
        .tab_selected = {0.85F, 0.81F, 0.70F, 1.00F},
        .table_header_bg = {0.88F, 0.86F, 0.80F, 1.00F},
        .table_border = {0.70F, 0.66F, 0.56F, 1.00F},
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
        .title_bg = {0.925F, 0.937F, 0.957F, 1.00F},
        .title_bg_active = {0.80F, 0.86F, 0.94F, 1.00F},
        .menu_bar_bg = {0.80F, 0.86F, 0.94F, 1.00F},
        .popup_bg = {0.96F, 0.97F, 0.99F, 0.98F},
        .header = {0.53F, 0.75F, 0.82F, 1.00F},
        .header_hovered = {0.60F, 0.80F, 0.86F, 1.00F},
        .button = {0.53F, 0.75F, 0.82F, 1.00F},
        .button_hovered = {0.60F, 0.80F, 0.86F, 1.00F},
        .button_active = {0.37F, 0.51F, 0.67F, 1.00F},
        .tab_selected = {0.53F, 0.75F, 0.82F, 1.00F},
        .table_header_bg = {0.80F, 0.86F, 0.94F, 1.00F},
        .table_border = {0.60F, 0.68F, 0.78F, 1.00F},
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
        .title_bg = {0.93F, 0.98F, 0.95F, 1.00F},
        .title_bg_active = {0.75F, 0.90F, 0.80F, 1.00F},
        .menu_bar_bg = {0.75F, 0.90F, 0.80F, 1.00F},
        .popup_bg = {0.96F, 0.99F, 0.97F, 0.98F},
        .header = {0.40F, 0.78F, 0.60F, 1.00F},
        .header_hovered = {0.46F, 0.84F, 0.66F, 1.00F},
        .button = {0.40F, 0.78F, 0.60F, 1.00F},
        .button_hovered = {0.46F, 0.84F, 0.66F, 1.00F},
        .button_active = {0.30F, 0.68F, 0.50F, 1.00F},
        .tab_selected = {0.40F, 0.78F, 0.60F, 1.00F},
        .table_header_bg = {0.75F, 0.90F, 0.80F, 1.00F},
        .table_border = {0.45F, 0.72F, 0.58F, 1.00F},
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

ThemeStyle scale_theme_style(const ThemeStyle& style, float content_scale) {
    ThemeStyle scaled = style;
    scaled.window_rounding *= content_scale;
    scaled.frame_rounding *= content_scale;
    scaled.grab_rounding *= content_scale;
    return scaled;
}

void apply_theme(const ThemeStyle& style) {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = style.window_rounding;
    s.FrameRounding = style.frame_rounding;
    s.GrabRounding = style.grab_rounding;

    const auto to_imvec4 = [](const ThemeColor& c) { return ImVec4(c.r, c.g, c.b, c.a); };
    const ImVec4 header = to_imvec4(style.header);
    const ImVec4 header_hovered = to_imvec4(style.header_hovered);
    const ImVec4 title_bg = to_imvec4(style.title_bg);
    const ImVec4 title_bg_active = to_imvec4(style.title_bg_active);
    const ImVec4 tab_selected = to_imvec4(style.tab_selected);

    s.Colors[ImGuiCol_Text] = to_imvec4(style.text);
    s.Colors[ImGuiCol_WindowBg] = to_imvec4(style.window_bg);
    s.Colors[ImGuiCol_FrameBg] = to_imvec4(style.frame_bg);
    s.Colors[ImGuiCol_TitleBg] = title_bg;
    s.Colors[ImGuiCol_TitleBgActive] = title_bg_active;
    s.Colors[ImGuiCol_MenuBarBg] = to_imvec4(style.menu_bar_bg);
    s.Colors[ImGuiCol_PopupBg] = to_imvec4(style.popup_bg);
    s.Colors[ImGuiCol_Header] = header;
    s.Colors[ImGuiCol_HeaderHovered] = header_hovered;
    s.Colors[ImGuiCol_Button] = to_imvec4(style.button);
    s.Colors[ImGuiCol_ButtonHovered] = to_imvec4(style.button_hovered);
    s.Colors[ImGuiCol_ButtonActive] = to_imvec4(style.button_active);
    s.Colors[ImGuiCol_TabSelected] = tab_selected;
    s.Colors[ImGuiCol_TableHeaderBg] = to_imvec4(style.table_header_bg);
    const ImVec4 table_border = to_imvec4(style.table_border);
    s.Colors[ImGuiCol_TableBorderStrong] = table_border;
    s.Colors[ImGuiCol_TableBorderLight] = table_border;

    // A docked panel with a single tab renders that tab as its title
    // bar/tab strip, colored by this family -- Dear ImGui's own theme
    // constructors derive it from Header/TitleBg*/TabSelected once at
    // startup and never recompute it, so re-derive it here the same way
    // every time the theme changes (see ThemeStyle::title_bg's doc comment).
    s.Colors[ImGuiCol_TabHovered] = header_hovered;
    const ImVec4 tab = lerp(header, title_bg_active, 0.80F);
    s.Colors[ImGuiCol_Tab] = tab;
    s.Colors[ImGuiCol_TabDimmed] = lerp(tab, title_bg, 0.80F);
    s.Colors[ImGuiCol_TabDimmedSelected] = lerp(tab_selected, title_bg, 0.40F);
    s.Colors[ImGuiCol_TabSelectedOverline] = header_hovered;
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
