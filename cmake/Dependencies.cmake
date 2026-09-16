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
# Vulkan-Headers + volk - Windows-only Vulkan/ImGui backend (#99/#100/#96).
#
# Fetched (and the Dear ImGui library below built against them) unconditionally
# -- not gated on WIN32 -- rather than following the GLFW X11/Wayland
# platform-conditional pattern above: app/vulkan_backend.cpp, and Dear ImGui's
# own imgui_impl_vulkan.cpp backend (added to the `imgui` target below), are
# both compiled on every platform (see app/CMakeLists.txt) purely so they're
# covered by clang-format/clang-tidy and by a real warnings-as-errors Clang
# compile against these headers on Linux/macOS too, even though
# app::run_vulkan_app() is only ever called from the `#ifdef _WIN32` branch in
# app/main.cpp. This costs a small extra fetch/compile on Linux/macOS but
# changes nothing about their runtime behavior (dead code; volk's dynamic
# loader is never invoked there) -- see
# docs/adr/0004-windows-vulkan-hard-cutover.md.
#
# Fetched ahead of the Dear ImGui section below (rather than after, as in
# #99) because the `imgui` target now links against `volk` and needs its
# headers visible while compiling imgui_impl_vulkan.cpp.
#
# No Vulkan SDK is required to build or run: GLFW dynamically loads the
# Vulkan loader (vulkan-1.dll on Windows) at runtime with no link-time import
# library, and volk (MIT) resolves the remaining instance/device function
# pointers the same way Dear ImGui's own example_glfw_vulkan does.
# Vulkan-Headers (Apache-2.0 OR MIT) supplies the header-only type/constant
# declarations volk and app code build against. Both are pinned to the same
# `vulkan-sdk-1.3.296.0` tag so their Vulkan API versions line up.
# ---------------------------------------------------------------------------
FetchContent_Declare(vulkan_headers
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG vulkan-sdk-1.3.296.0
    GIT_SHALLOW TRUE
    SYSTEM)

# volk's own CMakeLists tries to discover a system Vulkan SDK
# (find_package(Vulkan) / $VULKAN_SDK) to find headers when
# VOLK_PULL_IN_VULKAN is ON; this project deliberately has neither, so that
# is turned off and volk is pointed at the Vulkan-Headers target fetched
# above instead (below, after FetchContent_MakeAvailable).
set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "" FORCE)
set(VOLK_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG vulkan-sdk-1.3.296.0
    GIT_SHALLOW TRUE
    SYSTEM)

FetchContent_MakeAvailable(vulkan_headers volk)

target_link_libraries(volk PUBLIC Vulkan::Headers)
target_link_libraries(volk_headers INTERFACE Vulkan::Headers)

# Vendored code: not our warning policy.
if(NOT MSVC)
    target_compile_options(volk PRIVATE -w)
endif()

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
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    # Windows-only at runtime (see app/vulkan_backend.cpp); compiled on every
    # platform for the same type-check-as-dead-code reason as
    # vulkan_backend.cpp itself (see the Vulkan-Headers/volk section above).
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp)
target_include_directories(imgui SYSTEM PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw volk)
target_compile_features(imgui PUBLIC cxx_std_23)
add_library(imgui::imgui ALIAS imgui)

# Widen ImDrawIdx to 32-bit (default is 16-bit): ImPlot's docs warn that a
# draw list needing more than 65,536 indices can silently corrupt rendering
# with the default width. Set once here, PUBLIC so it propagates identically
# to every consumer (implot, ui, app) -- required for ABI consistency across
# every translation unit that touches ImDrawList. See #97.
target_compile_definitions(imgui PUBLIC
    IMGUI_USER_CONFIG="${CMAKE_SOURCE_DIR}/cmake/imgui_user_config.h"
    # Pairs imgui_impl_vulkan.cpp with volk for runtime Vulkan function
    # loading (no link-time import library, no Vulkan SDK) -- the same
    # pairing Dear ImGui's own example_glfw_vulkan uses. PUBLIC so every
    # consumer that includes imgui_impl_vulkan.h (app/vulkan_backend.cpp)
    # sees the same macro the backend itself was compiled with -- imgui_impl_
    # vulkan.h's declarations differ slightly based on this define. See #100.
    IMGUI_IMPL_VULKAN_USE_VOLK)

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

# ---------------------------------------------------------------------------
# JetBrains Mono - the app's default (and only) font, embedded at build time.
#
# Fetched as a release zip (font source has no upstream CMake, and there's
# nothing to build -- just an asset to unpack), pinned to release v2.304 by
# URL + SHA256, same "exact version, verifiable" spirit as the git-tag pins
# above. OFL-1.1 licensed. See #60.
# ---------------------------------------------------------------------------
FetchContent_Declare(jetbrains_mono
    URL https://github.com/JetBrains/JetBrainsMono/releases/download/v2.304/JetBrainsMono-2.304.zip
    URL_HASH SHA256=6f6376c6ed2960ea8a963cd7387ec9d76e3f629125bc33d1fdcd7eb7012f7bbf)
FetchContent_MakeAvailable(jetbrains_mono)

# `binary_to_compressed_c` is a small, dependency-free host tool bundled with
# Dear ImGui (misc/fonts/) that turns a TTF into a compressed C byte array,
# so the font ships embedded in the binary with no runtime file I/O. It's a
# build-host tool, not part of the app -- no warnings policy, no ImGui link.
add_executable(binary_to_compressed_c ${imgui_SOURCE_DIR}/misc/fonts/binary_to_compressed_c.cpp)
if(NOT MSVC)
    target_compile_options(binary_to_compressed_c PRIVATE -w)
endif()

# Deliberately *not* -base85: base85 emits the font as one giant adjacent-
# concatenated C string literal, which trips first-party code's
# -Wpedantic -Werror (-Woverlength-strings; ISO C++ only requires compilers
# to support 65536-char string literals, and this font's is far longer). The
# plain compressed byte array has no such limit, at the cost of a slightly
# larger generated source file. Paired with AddFontFromMemoryCompressedTTF
# (no "Base85") below.
set(jetbrains_mono_header "${CMAKE_BINARY_DIR}/generated/jetbrains_mono_medium.h")
add_custom_command(
    OUTPUT ${jetbrains_mono_header}
    COMMAND binary_to_compressed_c
            "${jetbrains_mono_SOURCE_DIR}/fonts/ttf/JetBrainsMono-Medium.ttf"
            JetBrainsMonoMedium > ${jetbrains_mono_header}
    DEPENDS binary_to_compressed_c "${jetbrains_mono_SOURCE_DIR}/fonts/ttf/JetBrainsMono-Medium.ttf"
    VERBATIM)
add_custom_target(generate_jetbrains_mono_header DEPENDS ${jetbrains_mono_header})
