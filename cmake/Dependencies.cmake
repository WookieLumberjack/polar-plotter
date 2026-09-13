# Third-party dependencies, fetched and pinned. Nothing here is subject to the
# project warning policy.

include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# ---------------------------------------------------------------------------
# GLFW - windowing / input for the desktop backend.
# ---------------------------------------------------------------------------
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# X11 only on Linux for now: the Wayland backend pulls in wayland-scanner and
# the wayland-protocols toolchain, which isn't worth the CI/setup surface yet.
# Flip this on (and add the libwayland-dev / wayland-protocols packages) when a
# native Wayland session actually needs it.
if(UNIX AND NOT APPLE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
    SYSTEM)

# ---------------------------------------------------------------------------
# Catch2 - test framework.
# ---------------------------------------------------------------------------
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    GIT_SHALLOW TRUE
    SYSTEM)

FetchContent_MakeAvailable(glfw Catch2)

# Catch2's CTest integration module (provides catch_discover_tests()).
list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")

# ---------------------------------------------------------------------------
# Dear ImGui - no upstream CMake, so we compile it ourselves.
#
# Pinned to the `docking` branch's v1.92.9b-docking tag (not master's plain
# v1.92.9b) so panels can dock into a real dockspace (#57/#58); see
# docs/adr/0003-pin-imgui-to-docking-branch.md for the rationale and
# trade-offs of moving off a stable release tag onto a moving branch.
# `v1.92.9b-docking` is a real tag (ocornut cuts a matching `-docking` tag for
# every release), so GIT_SHALLOW TRUE still works -- shallow fetches need an
# advertised ref, not a bare SHA.
# ---------------------------------------------------------------------------
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.9b-docking
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui SYSTEM PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw)
target_compile_features(imgui PUBLIC cxx_std_23)
add_library(imgui::imgui ALIAS imgui)

# Vendored code: not our warning policy.
if(NOT MSVC)
    target_compile_options(imgui PRIVATE -w)
endif()

if(APPLE)
    target_link_libraries(imgui PUBLIC "-framework OpenGL")
elseif(WIN32)
    target_link_libraries(imgui PUBLIC opengl32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(imgui PUBLIC OpenGL::GL)
endif()

# ---------------------------------------------------------------------------
# ImPlot - depends only on Dear ImGui.
# ---------------------------------------------------------------------------
FetchContent_Declare(implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG v1.0
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(implot)

add_library(implot STATIC
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
    ${implot_SOURCE_DIR}/implot_demo.cpp)
target_include_directories(implot SYSTEM PUBLIC ${implot_SOURCE_DIR})
target_link_libraries(implot PUBLIC imgui)
target_compile_features(implot PUBLIC cxx_std_23)
add_library(implot::implot ALIAS implot)

if(NOT MSVC)
    target_compile_options(implot PRIVATE -w)
endif()
