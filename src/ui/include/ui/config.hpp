#ifndef UI_CONFIG_HPP
#define UI_CONFIG_HPP

#include <array>
#include <filesystem>
#include <optional>

#include "ui/theme.hpp"

namespace ui {

/// The handful of values worth remembering between runs. Serialised as a flat
/// `key=value` text file -- the only thing this application writes to disk.
struct Config {
    std::array<float, 2> a{3.0F, 1.0F};
    std::array<float, 2> b{-1.0F, 2.0F};
    bool show_sum{false};
    bool show_difference{true};
    // Show/plot toggles for the remaining derived vectors (see
    // ui::DerivedVectors): B - A, the complex product A x B, and the two
    // complex quotients A / B, B / A. Unlike show_sum/show_difference, none
    // of these has a tip-to-tail/construction sub-toggle.
    bool show_difference_ba{false};
    bool show_product{false};
    bool show_quotient_ab{false};
    bool show_quotient_ba{false};
    // Pixel thickness of vector shafts/heads and the zero-direction/annotation
    // arcs, shared by all of them -- there is no per-item styling.
    float line_width{2.0F};
    // When true, the polar plot's ring interval/extent is auto-fit to the
    // shown vectors' magnitudes; when false, manual_ring_interval is used
    // verbatim instead (see ui::PlotInputs::auto_scale).
    bool auto_scale{true};
    // Ring interval used verbatim when auto_scale is false.
    float manual_ring_interval{1.0F};
    // Selected built-in visual theme (see ui/theme.hpp). Serialized as its
    // enum name (e.g. "slate"); an unrecognized/missing value falls back to
    // the default, same rule as every other field here.
    Theme theme{Theme::kSlate};

    friend bool operator==(const Config&, const Config&) = default;
};

/// Parse a config file. Returns std::nullopt when the file cannot be read;
/// unknown keys are ignored and missing keys keep their default.
[[nodiscard]] std::optional<Config> load_config(const std::filesystem::path& path);

/// Write \p config to \p path. Returns false on I/O failure.
[[nodiscard]] bool save_config(const std::filesystem::path& path, const Config& config);

}  // namespace ui

#endif  // UI_CONFIG_HPP
