#include "dpi_scale.hpp"

#include <GLFW/glfw3.h>

namespace app {

float manual_ui_scale(GLFWwindow* window, float content_scale) {
    int window_width = 0;
    glfwGetWindowSize(window, &window_width, nullptr);
    int framebuffer_width = 0;
    glfwGetFramebufferSize(window, &framebuffer_width, nullptr);
    if (window_width <= 0 || framebuffer_width <= 0) {
        return content_scale;
    }

    const float backend_auto_scale =
        static_cast<float>(framebuffer_width) / static_cast<float>(window_width);
    if (backend_auto_scale <= 0.0F) {
        return content_scale;
    }
    return content_scale / backend_auto_scale;
}

}  // namespace app
