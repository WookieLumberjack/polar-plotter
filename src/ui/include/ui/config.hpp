#ifndef UI_CONFIG_HPP
#define UI_CONFIG_HPP

#include <array>
#include <filesystem>
#include <optional>

namespace ui {

/// The handful of values worth remembering between runs. Serialised as a flat
/// `key=value` text file -- the only thing this application writes to disk.
struct Config {
    std::array<float, 2> a{3.0F, 1.0F};
    std::array<float, 2> b{-1.0F, 2.0F};
    bool show_sum{false};
    bool show_difference{true};

    friend bool operator==(const Config&, const Config&) = default;
};

/// Parse a config file. Returns std::nullopt when the file cannot be read;
/// unknown keys are ignored and missing keys keep their default.
[[nodiscard]] std::optional<Config> load_config(const std::filesystem::path& path);

/// Write \p config to \p path. Returns false on I/O failure.
[[nodiscard]] bool save_config(const std::filesystem::path& path, const Config& config);

}  // namespace ui

#endif  // UI_CONFIG_HPP
