// Desktop host. On Windows this runs the full ui::App UI through a Vulkan
// rendering pipeline (see vulkan_backend.hpp/.cpp and #99/#100/#96) instead
// of the OpenGL/ImGui path below -- the OpenGL backend stalls while the
// window is maximized on Windows 11 (see docs/adr/0004). Linux and macOS are
// untouched: they still create a GLFW window + OpenGL3 context, wire up Dear
// ImGui and ImPlot, and run ui::App once per frame, exactly as before.

#ifdef _WIN32

#include "vulkan_backend.hpp"

int main() { return app::run_vulkan_app(); }

#else

#include <cstdint>
#include <cstdio>
#include <cstdlib>
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

#include "config_path.hpp"
#include "dpi_scale.hpp"
#include "font_atlas.hpp"
#include "ppm_writer.hpp"
#include "ui/app.hpp"
#include "waveform_ticker.hpp"

namespace {

void glfw_error_callback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

// GLFWwindowcontentscalefun: fires on startup's first PollEvents and again
// whenever the window moves to a monitor with a different content scale
// (e.g. dragged from a 100% to a 150% display). Rebuilds the font atlas at
// the new effective pixel size (re-uploaded to the GPU automatically -- see
// app::rebuild_font_atlas), then re-derives ui::App's style from scratch for
// the new scale -- see ui::App::apply_current_theme's doc comment for why
// that composition can't just scale ImGuiStyle in place.
void glfw_content_scale_callback(GLFWwindow* window, float xscale, float /*yscale*/) {
    const float manual_scale = app::manual_ui_scale(window, xscale);
    app::rebuild_font_atlas(manual_scale);

    auto* ui_app = static_cast<ui::App*>(glfwGetWindowUserPointer(window));
    if (ui_app != nullptr) {
        ui_app->set_content_scale(manual_scale);
    }
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
    // Have GLFW itself resize the window to match a monitor's content scale
    // (e.g. a 150%-scale Windows display) rather than leaving it at its
    // requested logical size -- must be set before window creation. See
    // CONTEXT.md's "content scale" entry.
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
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

    // Read the window's initial content scale so the very first font atlas
    // build (below) and the very first App::set_content_scale call are
    // already correct, without waiting for a content-scale-changed callback
    // that may never fire if the window opens on its eventual monitor.
    // manual_ui_scale (see dpi_scale.hpp) divides out whatever Dear ImGui's
    // GLFW backend already auto-compensates for (the macOS Retina
    // backing-store case, #111) before it reaches the font atlas / ui::App.
    float content_scale = 1.0F;
    float content_scale_y_unused = 1.0F;
    glfwGetWindowContentScale(window, &content_scale, &content_scale_y_unused);
    const float manual_scale = app::manual_ui_scale(window, content_scale);

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
    // default and only font (#60), built at the window's actual content
    // scale from the start (see app::rebuild_font_atlas) rather than a fixed
    // size ImGui_ImplOpenGL3_Init would otherwise upload once and never
    // revisit.
    app::rebuild_font_atlas(manual_scale);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    {
        ui::App app(app::config_path());
        // See ui::App::set_content_scale's doc comment: this both records
        // the scale read above and re-derives the theme's style for it,
        // since the constructor above applied the theme at the default
        // (unscaled) content_scale_ of 1.0.
        app.set_content_scale(manual_scale);

        // Let the content-scale-changed callback below reach this App
        // instance without ui:: ever depending on GLFW itself.
        glfwSetWindowUserPointer(window, &app);
        glfwSetWindowContentScaleCallback(window, glfw_content_scale_callback);

        // POLAR_PLOTTER_SCREENSHOT determinism (#105/#108): fill every
        // waveform buffer's full 5 second window with real simulated values
        // before the first frame is even drawn, rather than letting the
        // capture race real wall-clock time (which would make the captured
        // waveform image non-deterministic run-to-run).
        if (screenshot_mode) {
            app.prime_waveforms_for_screenshot();
        }

        int frame = 0;
        // Wall-clock-to-fixed-tick accumulator (#105/#108/#109): advance_waveforms
        // must be called at a fixed ui::kWaveformTickHz cadence, decoupled from
        // render frame rate, so the waveform's shape never warps/jitters when
        // frame rate fluctuates. glfwGetTime() is read here (GLFW-specific host
        // glue) and reduced to a plain elapsed-seconds value before crossing
        // into ui::App -- app::WaveformTicker (waveform_ticker.hpp) does the
        // actual accumulation and is shared verbatim with the Vulkan host (see
        // vulkan_backend.cpp), mirroring set_content_scale's boundary crossing.
        // Skipped entirely in screenshot mode: the buffers are already fully
        // (and deterministically) primed above, and a headless capture
        // shouldn't also advance them by whatever wall-clock time elapses
        // while rendering the handful of screenshot frames.
        app::WaveformTicker waveform_ticker(glfwGetTime());

        while (glfwWindowShouldClose(window) == GLFW_FALSE) {
            glfwPollEvents();

            if (!screenshot_mode) {
                waveform_ticker.tick(glfwGetTime(), app);
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            app.render();
            if (app.want_exit()) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }

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
                if (!app::write_ppm(shot_path, display_w, display_h,
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

#endif  // _WIN32
