#include <filesystem>
#include <fstream>
#include <optional>

#include <catch2/catch_test_macros.hpp>

#include "ui/config.hpp"

using ui::Config;
using ui::load_config;
using ui::save_config;

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
