// Desktop host: creates a GLFW window + OpenGL3 context, wires up Dear ImGui and
// ImPlot, and runs ui::App once per frame. Nothing project-specific lives here.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

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

#include "jetbrains_mono_medium.h"
#include "ui/app.hpp"

namespace {

// Fixed size for the app's one and only font, chosen for this app's widget
// density (compact input rows, plot labels, tables) at the default window
// size. No font-size UI: see #60.
constexpr float kFontSizePixels = 18.0F;

void glfw_error_callback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

// Write an RGB framebuffer (top-down) as a binary PPM (P6). Chosen for zero
// dependencies; convert to PNG with e.g. `magick shot.ppm shot.png`.
bool write_ppm(const std::filesystem::path& path, int width, int height,
               const std::vector<std::uint8_t>& rgb) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }
    out << "P6\n" << width << ' ' << height << "\n255\n";
    out.write(reinterpret_cast<const char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
    return out.good();
}

// Read the GL back buffer into a top-down RGB buffer.
std::vector<std::uint8_t> read_framebuffer(int width, int height) {
    const auto w = static_cast<std::size_t>(width);
    const auto h = static_cast<std::size_t>(height);

    std::vector<std::uint8_t> rgba(w * h * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

    std::vector<std::uint8_t> rgb(w * h * 3);
    for (std::size_t y = 0; y < h; ++y) {
        const std::uint8_t* src = rgba.data() + (w * 4 * (h - 1 - y));  // vertical flip
        std::uint8_t* dst = rgb.data() + (w * 3 * y);
        for (std::size_t x = 0; x < w; ++x) {
            dst[(x * 3) + 0] = src[(x * 4) + 0];
            dst[(x * 3) + 1] = src[(x * 4) + 1];
            dst[(x * 3) + 2] = src[(x * 4) + 2];
        }
    }
    return rgb;
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
    // When set, render a few frames off-screen, dump the framebuffer to this
    // path as a PPM, and exit. Lets CI / a headless box produce a screenshot
    // without a display server or window manager.
    const char* shot_path = std::getenv("POLAR_PLOTTER_SCREENSHOT");
    const bool screenshot_mode = shot_path != nullptr;

    glfwSetErrorCallback(glfw_error_callback);
    if (glfwInit() == GLFW_FALSE) {
        return EXIT_FAILURE;
    }

    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    if (screenshot_mode) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }

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
    // Docking branch (see docs/adr/0003-pin-imgui-to-docking-branch.md):
    // lets the "Vectors"/"Polar plot" windows dock into ui::App's dockspace
    // instead of floating freely.
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // Restrict window-move to the title bar: the polar plot's tip drag (see
    // #44/#48) is hand-rolled directly over the plot body with no widget of
    // its own claiming the mouse, so with the default (drag-anywhere-in-body
    // moves the window) a tip drag would fall through and move the "Polar
    // plot" window instead of the vector. Still holds with docking enabled:
    // a docked tab's own drag-to-move/drag-to-dock affordance is its tab,
    // which is part of the title-bar area this flag already restricts
    // window-move to, not the plot body itself (re-verified manually, see
    // #58).
    ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;

    // JetBrains Mono Medium, embedded at build time (binary_to_compressed_c
    // over the fetched TTF, see cmake/Dependencies.cmake) -- no
    // AddFontFromFileTTF / runtime file path loading. This is the app's
    // default and only font (#60).
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(
        JetBrainsMonoMedium_compressed_data, JetBrainsMonoMedium_compressed_size, kFontSizePixels);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    {
        ui::App app(config_path());
        int frame = 0;

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

            // Let ImGui settle (it lays out some widgets over two frames), then
            // grab the framebuffer and quit.
            if (screenshot_mode && ++frame >= 8) {
                glFinish();
                if (!write_ppm(shot_path, display_w, display_h,
                               read_framebuffer(display_w, display_h))) {
                    std::fprintf(stderr, "failed to write screenshot to %s\n", shot_path);
                    return EXIT_FAILURE;
                }
                std::fprintf(stderr, "wrote %dx%d screenshot to %s\n", display_w, display_h,
                             shot_path);
                break;
            }

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
