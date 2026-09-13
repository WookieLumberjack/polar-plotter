#include "ui/config.hpp"

#include <array>
#include <cctype>
#include <fstream>
#include <string>
#include <string_view>

namespace ui {
namespace {

std::optional<float> parse_float(std::string_view text) {
    // from_chars for float is not universally available yet; fall back to strtod.
    const std::string owned{text};
    try {
        std::size_t consumed = 0;
        const float value = std::stof(owned, &consumed);
        if (consumed != owned.size()) {
            return std::nullopt;
        }
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

std::string_view trim(std::string_view s) {
    const auto not_space = [](unsigned char c) { return std::isspace(c) == 0; };
    while (!s.empty() && !not_space(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }
    while (!s.empty() && !not_space(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }
    return s;
}

bool as_bool(std::string_view v) { return v == "1" || v == "true"; }

// theme= serializes/parses as the enum's name (e.g. "slate"), not a number --
// numbers would silently shift meaning if Theme's declaration order ever
// changes. An unrecognized name (including from a future version's theme
// this build doesn't know) falls back to Config's default, same as any other
// unrecognized/missing key.
std::string_view theme_name(Theme theme) {
    switch (theme) {
        case Theme::kSlate:
            return "slate";
        case Theme::kMidnight:
            return "midnight";
        case Theme::kPaper:
            return "paper";
        case Theme::kNordLight:
            return "nord_light";
        case Theme::kMint:
            return "mint";
    }
    return "slate";
}

std::optional<Theme> parse_theme(std::string_view value) {
    if (value == "slate") {
        return Theme::kSlate;
    }
    if (value == "midnight") {
        return Theme::kMidnight;
    }
    if (value == "paper") {
        return Theme::kPaper;
    }
    if (value == "nord_light") {
        return Theme::kNordLight;
    }
    if (value == "mint") {
        return Theme::kMint;
    }
    return std::nullopt;
}

// key -> pointer to the field it sets, for the flat key=value fields that
// just need parsing (or bool conversion) and assignment. a.x/a.y/b.x/b.y are
// handled separately below since they address into Config::a/b's elements
// rather than a whole field.
struct FloatField {
    std::string_view key;
    float Config::* field;
};

struct BoolField {
    std::string_view key;
    bool Config::* field;
};

void apply(Config& cfg, std::string_view key, std::string_view value) {
    constexpr std::array<BoolField, 6> kBoolFields{{
        {"show_sum", &Config::show_sum},
        {"show_difference", &Config::show_difference},
        {"show_difference_ba", &Config::show_difference_ba},
        {"show_product", &Config::show_product},
        {"show_quotient_ab", &Config::show_quotient_ab},
        {"show_quotient_ba", &Config::show_quotient_ba},
    }};
    constexpr std::array<FloatField, 3> kFloatFields{{
        {"line_width", &Config::line_width},
        {"manual_ring_interval", &Config::manual_ring_interval},
    }};

    if (key == "a.x") {
        if (const auto v = parse_float(value)) {
            cfg.a[0] = *v;
        }
        return;
    }
    if (key == "a.y") {
        if (const auto v = parse_float(value)) {
            cfg.a[1] = *v;
        }
        return;
    }
    if (key == "b.x") {
        if (const auto v = parse_float(value)) {
            cfg.b[0] = *v;
        }
        return;
    }
    if (key == "b.y") {
        if (const auto v = parse_float(value)) {
            cfg.b[1] = *v;
        }
        return;
    }
    if (key == "auto_scale") {
        cfg.auto_scale = as_bool(value);
        return;
    }
    if (key == "theme") {
        if (const auto v = parse_theme(value)) {
            cfg.theme = *v;
        }
        return;
    }
    for (const BoolField& f : kBoolFields) {
        if (key == f.key) {
            cfg.*f.field = as_bool(value);
            return;
        }
    }
    for (const FloatField& f : kFloatFields) {
        if (key == f.key) {
            if (const auto v = parse_float(value)) {
                cfg.*f.field = *v;
            }
            return;
        }
    }
}

}  // namespace

std::optional<Config> load_config(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        return std::nullopt;
    }

    Config cfg{};
    std::string line;
    while (std::getline(in, line)) {
        const std::string_view view = trim(line);
        if (view.empty() || view.front() == '#') {
            continue;
        }
        const auto eq = view.find('=');
        if (eq == std::string_view::npos) {
            continue;
        }
        apply(cfg, trim(view.substr(0, eq)), trim(view.substr(eq + 1)));
    }
    return cfg;
}

bool save_config(const std::filesystem::path& path, const Config& config) {
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        return false;
    }
    out << "# polar-plotter inputs\n";
    out << "a.x=" << config.a[0] << '\n';
    out << "a.y=" << config.a[1] << '\n';
    out << "b.x=" << config.b[0] << '\n';
    out << "b.y=" << config.b[1] << '\n';
    out << "show_sum=" << (config.show_sum ? 1 : 0) << '\n';
    out << "show_difference=" << (config.show_difference ? 1 : 0) << '\n';
    out << "show_difference_ba=" << (config.show_difference_ba ? 1 : 0) << '\n';
    out << "show_product=" << (config.show_product ? 1 : 0) << '\n';
    out << "show_quotient_ab=" << (config.show_quotient_ab ? 1 : 0) << '\n';
    out << "show_quotient_ba=" << (config.show_quotient_ba ? 1 : 0) << '\n';
    out << "line_width=" << config.line_width << '\n';
    out << "auto_scale=" << (config.auto_scale ? 1 : 0) << '\n';
    out << "manual_ring_interval=" << config.manual_ring_interval << '\n';
    out << "theme=" << theme_name(config.theme) << '\n';
    return out.good();
}

}  // namespace ui
