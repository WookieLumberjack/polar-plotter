// Desktop host: creates a GLFW window + OpenGL3 context, wires up Dear ImGui and
// ImPlot, and runs ui::App once per frame. Nothing project-specific lives here.

#include <cstdlib>
#include <filesystem>
#include <print>

// Only core GL 1.1 entry points (glViewport/glClear/...) are used directly here;
// every desktop platform exports these from its system GL library, so no loader
// is needed. Dear ImGui's OpenGL3 backend carries its own loader internally.
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

#include "ui/app.hpp"

namespace {

void glfw_error_callback(int error, const char* description) {
    std::println(stderr, "GLFW error {}: {}", error, description);
}

std::filesystem::path config_path() {
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME")) {
        return std::filesystem::path(xdg) / "polar-plotter" / "inputs.conf";
    }
    if (const char* home = std::getenv("HOME")) {
        return std::filesystem::path(home) / ".config" / "polar-plotter" / "inputs.conf";
    }
    return {"polar-plotter.conf"};
}

}  // namespace

int main() {
    glfwSetErrorCallback(glfw_error_callback);
    if (glfwInit() == GLFW_FALSE) {
        return EXIT_FAILURE;
    }

    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 800, "polar-plotter", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    {
        ui::App app(config_path());

        while (glfwWindowShouldClose(window) == GLFW_FALSE) {
            glfwPollEvents();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            app.render();

            ImGui::Render();
            int display_w = 0;
            int display_h = 0;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.10F, 0.11F, 0.13F, 1.0F);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }
    }  // app.save() runs here

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
