#ifndef APP_CONFIG_PATH_HPP
#define APP_CONFIG_PATH_HPP

#include <filesystem>

namespace app {

// Where ui::App persists last-used inputs (see ui::Config). Shared between
// the OpenGL host (Linux/macOS, app/main.cpp) and the Vulkan host (Windows,
// app/vulkan_backend.cpp) so both construct ui::App against the same path
// convention.
std::filesystem::path config_path();

}  // namespace app

#endif  // APP_CONFIG_PATH_HPP
