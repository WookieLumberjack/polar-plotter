#include <filesystem>
#include <fstream>
#include <optional>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "ui/config.hpp"
#include "ui/theme.hpp"
#include "waveform_plotting/waveform_buffer.hpp"

using ui::Config;
using ui::load_config;
using ui::save_config;
using ui::Theme;
using waveform_plotting::PhaseConvention;

namespace {

std::filesystem::path make_temp_path(const char* name) {
    return std::filesystem::temp_directory_path() / name;
}

// Fails the current test and returns a default-constructed Config if `opt`
// is empty, otherwise returns its value -- see the identical helper in
// plot_plan_tests.cpp for why this beats REQUIRE(has_value()) + dereference.
Config require_value(const std::optional<Config>& opt) {
    if (opt) {
        return *opt;
    }
    FAIL("expected optional value to be set");
    return Config{};
}

}  // namespace

TEST_CASE("line_width round-trips through save_config/load_config", "[config]") {
    const std::filesystem::path path = make_temp_path("polar_plotter_config_tests_line_width.cfg");

    Config cfg{};
    cfg.line_width = 4.5F;

    REQUIRE(save_config(path, cfg));
    const Config loaded = require_value(load_config(path));
    CHECK(loaded.line_width == 4.5F);

    std::filesystem::remove(path);
}

TEST_CASE("loading a config file that omits line_width falls back to the default", "[config]") {
    const std::filesystem::path path =
        make_temp_path("polar_plotter_config_tests_line_width_missing.cfg");

    {
        std::ofstream out(path, std::ios::trunc);
        out << "a.x=1.0\n";
        out << "a.y=2.0\n";
    }

    const Config loaded = require_value(load_config(path));
    CHECK(loaded.line_width == Config{}.line_width);

    std::filesystem::remove(path);
}

TEST_CASE("auto_scale and manual_ring_interval round-trip through save_config/load_config",
          "[config]") {
    const std::filesystem::path path =
        make_temp_path("polar_plotter_config_tests_manual_scale.cfg");

    Config cfg{};
    cfg.auto_scale = false;
    cfg.manual_ring_interval = 12.5F;

    REQUIRE(save_config(path, cfg));
    const Config loaded = require_value(load_config(path));
    CHECK(loaded.auto_scale == false);
    CHECK(loaded.manual_ring_interval == 12.5F);

    std::filesystem::remove(path);
}

TEST_CASE("loading a config file that omits auto_scale/manual_ring_interval falls back to defaults",
          "[config]") {
    const std::filesystem::path path =
        make_temp_path("polar_plotter_config_tests_manual_scale_missing.cfg");

    {
        std::ofstream out(path, std::ios::trunc);
        out << "a.x=1.0\n";
        out << "a.y=2.0\n";
    }

    const Config loaded = require_value(load_config(path));
    CHECK(loaded.auto_scale == Config{}.auto_scale);
    CHECK(loaded.manual_ring_interval == Config{}.manual_ring_interval);

    std::filesystem::remove(path);
}

TEST_CASE(
    "loading a config file with a non-positive manual_ring_interval= value falls back to the "
    "default",
    "[config]") {
    const std::filesystem::path path =
        make_temp_path("polar_plotter_config_tests_manual_ring_interval_nonpositive.cfg");

    // PlotFrame's constructor (see polarplot::PlotFrame) asserts a strictly
    // positive ring_interval; manual_ring_interval reaches it verbatim via
    // ui::plot_plan::manual_extent, so zero/negative on-disk values must
    // never survive load_config (see #37).
    const auto check_falls_back_to_default = [&](std::string_view value) {
        {
            std::ofstream out(path, std::ios::trunc);
            out << "manual_ring_interval=" << value << '\n';
        }
        const Config loaded = require_value(load_config(path));
        CHECK(loaded.manual_ring_interval == Config{}.manual_ring_interval);
    };

    check_falls_back_to_default("0");
    check_falls_back_to_default("-1.5");

    std::filesystem::remove(path);
}

TEST_CASE(
    "show_difference_ba/show_product/show_quotient_ab/show_quotient_ba round-trip through "
    "save_config/load_config",
    "[config]") {
    const std::filesystem::path path =
        make_temp_path("polar_plotter_config_tests_derived_toggles.cfg");

    Config cfg{};
    cfg.show_difference_ba = true;
    cfg.show_product = true;
    cfg.show_quotient_ab = true;
    cfg.show_quotient_ba = true;

    REQUIRE(save_config(path, cfg));
    const Config loaded = require_value(load_config(path));
    CHECK(loaded.show_difference_ba == true);
    CHECK(loaded.show_product == true);
    CHECK(loaded.show_quotient_ab == true);
    CHECK(loaded.show_quotient_ba == true);

    std::filesystem::remove(path);
}

TEST_CASE(
    "loading a config file that omits the derived-vector toggles falls back to their defaults",
    "[config]") {
    const std::filesystem::path path =
        make_temp_path("polar_plotter_config_tests_derived_toggles_missing.cfg");

    {
        std::ofstream out(path, std::ios::trunc);
        out << "a.x=1.0\n";
        out << "a.y=2.0\n";
    }

    const Config loaded = require_value(load_config(path));
    CHECK(loaded.show_difference_ba == Config{}.show_difference_ba);
    CHECK(loaded.show_product == Config{}.show_product);
    CHECK(loaded.show_quotient_ab == Config{}.show_quotient_ab);
    CHECK(loaded.show_quotient_ba == Config{}.show_quotient_ba);

    std::filesystem::remove(path);
}

TEST_CASE("theme round-trips through save_config/load_config", "[config]") {
    const std::filesystem::path path = make_temp_path("polar_plotter_config_tests_theme.cfg");

    Config cfg{};
    cfg.theme = Theme::kNordLight;

    REQUIRE(save_config(path, cfg));
    const Config loaded = require_value(load_config(path));
    CHECK(loaded.theme == Theme::kNordLight);

    std::filesystem::remove(path);
}

TEST_CASE("loading a config file that omits theme falls back to the default", "[config]") {
    const std::filesystem::path path =
        make_temp_path("polar_plotter_config_tests_theme_missing.cfg");

    {
        std::ofstream out(path, std::ios::trunc);
        out << "a.x=1.0\n";
        out << "a.y=2.0\n";
    }

    const Config loaded = require_value(load_config(path));
    CHECK(loaded.theme == Config{}.theme);

    std::filesystem::remove(path);
}

TEST_CASE("loading a config file with an unrecognized theme= value falls back to the default",
          "[config]") {
    const std::filesystem::path path =
        make_temp_path("polar_plotter_config_tests_theme_unrecognized.cfg");

    {
        std::ofstream out(path, std::ios::trunc);
        out << "theme=not_a_real_theme\n";
    }

    const Config loaded = require_value(load_config(path));
    CHECK(loaded.theme == Config{}.theme);

    std::filesystem::remove(path);
}

TEST_CASE(
    "waveform_frequency_hz/waveform_phase_convention round-trip through "
    "save_config/load_config",
    "[config]") {
    const std::filesystem::path path = make_temp_path("polar_plotter_config_tests_waveform.cfg");

    Config cfg{};
    cfg.waveform_frequency_hz = 2.5F;
    cfg.waveform_phase_convention = PhaseConvention::kLead;

    REQUIRE(save_config(path, cfg));
    const Config loaded = require_value(load_config(path));
    CHECK(loaded.waveform_frequency_hz == 2.5F);
    CHECK(loaded.waveform_phase_convention == PhaseConvention::kLead);

    std::filesystem::remove(path);
}

TEST_CASE(
    "loading a config file that omits waveform_frequency_hz/waveform_phase_convention falls back "
    "to their defaults",
    "[config]") {
    const std::filesystem::path path =
        make_temp_path("polar_plotter_config_tests_waveform_missing.cfg");

    {
        std::ofstream out(path, std::ios::trunc);
        out << "a.x=1.0\n";
        out << "a.y=2.0\n";
    }

    const Config loaded = require_value(load_config(path));
    CHECK(loaded.waveform_frequency_hz == Config{}.waveform_frequency_hz);
    CHECK(loaded.waveform_phase_convention == Config{}.waveform_phase_convention);

    std::filesystem::remove(path);
}

TEST_CASE(
    "loading a config file with an out-of-range waveform_frequency_hz= value clamps it into "
    "range",
    "[config]") {
    const std::filesystem::path path =
        make_temp_path("polar_plotter_config_tests_waveform_frequency_out_of_range.cfg");

    const auto check_clamped = [&](std::string_view value, float expected) {
        {
            std::ofstream out(path, std::ios::trunc);
            out << "waveform_frequency_hz=" << value << '\n';
        }
        const Config loaded = require_value(load_config(path));
        CHECK(loaded.waveform_frequency_hz == expected);
    };

    check_clamped("0.0", 0.1F);
    check_clamped("-3.0", 0.1F);
    check_clamped("50.0", 5.0F);

    std::filesystem::remove(path);
}

TEST_CASE(
    "loading a config file with an unrecognized waveform_phase_convention= value falls back to "
    "the default",
    "[config]") {
    const std::filesystem::path path =
        make_temp_path("polar_plotter_config_tests_waveform_phase_convention_unrecognized.cfg");

    {
        std::ofstream out(path, std::ios::trunc);
        out << "waveform_phase_convention=not_a_real_convention\n";
    }

    const Config loaded = require_value(load_config(path));
    CHECK(loaded.waveform_phase_convention == Config{}.waveform_phase_convention);

    std::filesystem::remove(path);
}

TEST_CASE(
    "waveform_phase_convention is unaffected by, and does not affect, angle-convention fields "
    "when saved/loaded together",
    "[config]") {
    const std::filesystem::path path =
        make_temp_path("polar_plotter_config_tests_waveform_phase_convention_independence.cfg");

    // #105/#108: Phase convention and Angle convention (rotation direction /
    // measurement convention -- not modeled in ui::Config at all, only in
    // ui::App/PlotInputs) must never interact. This round-trips Phase
    // convention alone and checks nothing else about the loaded config
    // shifted as a side effect.
    Config cfg{};
    cfg.waveform_phase_convention = PhaseConvention::kLead;

    REQUIRE(save_config(path, cfg));
    const Config loaded = require_value(load_config(path));
    CHECK(loaded.waveform_phase_convention == PhaseConvention::kLead);
    CHECK(loaded.a == Config{}.a);
    CHECK(loaded.b == Config{}.b);
    CHECK(loaded.theme == Config{}.theme);

    std::filesystem::remove(path);
}
