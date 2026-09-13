#include <filesystem>
#include <fstream>
#include <optional>

#include <catch2/catch_test_macros.hpp>

#include "ui/config.hpp"
#include "ui/theme.hpp"

using ui::Config;
using ui::load_config;
using ui::save_config;
using ui::Theme;

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
