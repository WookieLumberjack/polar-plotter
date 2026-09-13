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

// Derived from kAllThemes/theme_name rather than its own independent
// if-chain, so a theme added to kAllThemes without a matching entry here
// fails loudly (round-trip test breaks) instead of silently parsing as
// std::nullopt (falls back to default) forever.
std::optional<Theme> parse_theme(std::string_view value) {
    for (const Theme candidate : kAllThemes) {
        if (theme_name(candidate) == value) {
            return candidate;
        }
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

// a.x/a.y/b.x/b.y address into Config::a/b's elements rather than a whole
// field, so they need the array member plus an index alongside the key.
struct VecField {
    std::string_view key;
    std::array<float, 2> Config::* field;
    std::size_t index;
};

void apply(Config& cfg, std::string_view key, std::string_view value) {
    constexpr std::array<VecField, 4> kVecFields{{
        {"a.x", &Config::a, 0},
        {"a.y", &Config::a, 1},
        {"b.x", &Config::b, 0},
        {"b.y", &Config::b, 1},
    }};
    constexpr std::array<BoolField, 6> kBoolFields{{
        {"show_sum", &Config::show_sum},
        {"show_difference", &Config::show_difference},
        {"show_difference_ba", &Config::show_difference_ba},
        {"show_product", &Config::show_product},
        {"show_quotient_ab", &Config::show_quotient_ab},
        {"show_quotient_ba", &Config::show_quotient_ba},
    }};
    constexpr std::array<FloatField, 1> kFloatFields{{
        {"line_width", &Config::line_width},
    }};

    for (const VecField& f : kVecFields) {
        if (key == f.key) {
            if (const auto v = parse_float(value)) {
                (cfg.*f.field)[f.index] = *v;
            }
            return;
        }
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
    if (key == "manual_ring_interval") {
        // Reaches polarplot::PlotFrame's constructor verbatim (via
        // ui::plot_plan::manual_extent), which asserts a strictly positive
        // ring_interval -- a non-positive on-disk value is hand-edited,
        // external input, not a programmer error, so it's rejected here the
        // same way an unparseable/unrecognized value is: falls back to
        // Config's default instead of reaching that precondition (#37).
        if (const auto v = parse_float(value); v && *v > 0.0F) {
            cfg.manual_ring_interval = *v;
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
