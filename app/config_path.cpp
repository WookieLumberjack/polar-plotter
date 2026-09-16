#include "config_path.hpp"

#include <cstdlib>

namespace app {

std::filesystem::path config_path() {
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME")) {
        return std::filesystem::path(xdg) / "polar-plotter" / "inputs.conf";
    }
    if (const char* home = std::getenv("HOME")) {
        return std::filesystem::path(home) / ".config" / "polar-plotter" / "inputs.conf";
    }
    return {"polar-plotter.conf"};
}

}  // namespace app
