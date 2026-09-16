#include "font_atlas.hpp"

#include <imgui.h>

#include "jetbrains_mono_medium.h"

namespace app {

namespace {

// Base (100%-scale) size for the app's one and only font, chosen for this
// app's widget density (compact input rows, plot labels, tables) at the
// default window size. No font-size UI: see #60. The font atlas is always
// actually built at kFontSizePixels * content_scale (see rebuild_font_atlas)
// so text stays crisp on a scaled display instead of being blurrily
// upscaled from this base size.
constexpr float kFontSizePixels = 18.0F;

}  // namespace

void rebuild_font_atlas(float content_scale) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    io.Fonts->AddFontFromMemoryCompressedTTF(JetBrainsMonoMedium_compressed_data,
                                             JetBrainsMonoMedium_compressed_size,
                                             kFontSizePixels * content_scale);
    // Deliberately no explicit io.Fonts->Build() call: with the "new" dynamic
    // font atlas backends this Dear ImGui version ships (both
    // imgui_impl_opengl3 and imgui_impl_vulkan set
    // ImGuiBackendFlags_RendererHasTextures -- see font_atlas.hpp's doc
    // comment), the atlas is built and its texture uploaded lazily/
    // automatically as needed. Calling Build() here ends up preloading every
    // glyph range eagerly, which trips an assertion the next time
    // ImGui::NewFrame() runs
    // (imgui_draw.cpp's ImFontAtlasUpdateNewFrame: "Called
    // ImFontAtlas::Build() before ImGuiBackendFlags_RendererHasTextures got
    // set! With new backends: you don't need to call Build().").
}

}  // namespace app
