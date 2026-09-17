#ifndef APP_DPI_SCALE_HPP
#define APP_DPI_SCALE_HPP

struct GLFWwindow;

namespace app {

/// The manual UI/font scale factor to feed into rebuild_font_atlas and
/// ui::App::set_content_scale, given GLFW's raw \p content_scale for
/// \p window (from glfwGetWindowContentScale).
///
/// content_scale's meaning differs across platforms. On Windows (with
/// GLFW_SCALE_TO_MONITOR set), it's the OS DPI multiplier, and the window's
/// framebuffer size ends up equal to its window size -- nothing else
/// compensates for the scale, so it must be applied here in full. On macOS
/// Retina, content_scale is instead the automatic backing-store factor;
/// Dear ImGui's GLFW backend already compensates for it every frame via
/// io.DisplayFramebufferScale (= framebuffer size / window size, computed in
/// ImGui_ImplGlfw_NewFrame), so applying content_scale again on top of that
/// would double-scale the UI (#111). This divides out whatever ratio the
/// backend already auto-compensates -- a no-op on Windows/Linux (that ratio
/// is 1.0 there), reducing to ~1.0 on macOS Retina.
[[nodiscard]] float manual_ui_scale(GLFWwindow* window, float content_scale);

}  // namespace app

#endif  // APP_DPI_SCALE_HPP
