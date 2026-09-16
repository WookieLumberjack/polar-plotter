#ifndef APP_VULKAN_BACKEND_HPP
#define APP_VULKAN_BACKEND_HPP

namespace app {

// Windows-only Vulkan validation spike (#99, part of #96): stands up a full
// Vulkan pipeline (instance, physical/logical device, swapchain, render
// pass, framebuffers, command buffers, sync objects) via GLFW's Vulkan
// window-surface support, and presents nothing but a solid clear color,
// recreating the swapchain correctly across resize/maximize. This exists to
// confirm or refute -- as cheaply as possible -- whether swapping the
// Windows rendering backend away from OpenGL eliminates the maximize-time
// stall described in #96, before the sibling ticket invests in the full
// ImGui-over-Vulkan integration on top of this same pipeline.
//
// This is a hard cutover with no OpenGL fallback: see
// docs/adr/0004-windows-vulkan-hard-cutover.md. If Vulkan initialization
// fails, or no suitable device is found, this prints an actionable message
// to stderr and returns a failure exit code rather than falling back.
//
// Only ever called on Windows (see the `#ifdef _WIN32` branch in
// app/main.cpp); Linux and macOS keep the existing GLFW + OpenGL3 + ImGui/
// ImPlot path in main.cpp, completely untouched by this file. This
// translation unit is nonetheless compiled on every platform (see
// app/CMakeLists.txt) purely so it's covered by this project's
// clang-format/clang-tidy checks and by a real -Wall -Wextra -Wpedantic
// -Wconversion -Werror Clang compile against the real Vulkan-Headers/volk
// types -- it is simply never invoked outside Windows.
//
// Returns an exit code suitable for returning directly from main().
int run_vulkan_clear_window();

}  // namespace app

#endif  // APP_VULKAN_BACKEND_HPP
