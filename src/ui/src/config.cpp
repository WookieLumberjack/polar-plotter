#include "ui/config.hpp"

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

void apply(Config& cfg, std::string_view key, std::string_view value) {
    const auto as_bool = [](std::string_view v) { return v == "1" || v == "true"; };

    if (key == "a.x") {
        if (const auto v = parse_float(value)) { cfg.a[0] = *v; }
    } else if (key == "a.y") {
        if (const auto v = parse_float(value)) { cfg.a[1] = *v; }
    } else if (key == "b.x") {
        if (const auto v = parse_float(value)) { cfg.b[0] = *v; }
    } else if (key == "b.y") {
        if (const auto v = parse_float(value)) { cfg.b[1] = *v; }
    } else if (key == "show_sum") {
        cfg.show_sum = as_bool(value);
    } else if (key == "show_difference") {
        cfg.show_difference = as_bool(value);
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
    return out.good();
}

}  // namespace ui
