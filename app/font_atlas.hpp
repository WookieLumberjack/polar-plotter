#ifndef APP_FONT_ATLAS_HPP
#define APP_FONT_ATLAS_HPP

namespace app {

// Clear and rebuild Dear ImGui's font atlas with the embedded JetBrains Mono
// font at a fixed base pixel size times \p content_scale, so the rendered
// glyphs are natively that size rather than a linear stretch of some other
// fixed size (which would look blurry on a scaled display -- see
// CONTEXT.md's "content scale" entry).
//
// Shared between the OpenGL host (Linux/macOS, app/main.cpp's non-Windows
// branch) and the Vulkan host (Windows, app/vulkan_backend.cpp): the actual
// rebuild (ImFontAtlas::Clear/AddFontFromMemoryCompressedTTF/Build) is pure
// Dear ImGui core API with no backend-specific calls, so both hosts share
// this one implementation instead of keeping two copies in sync. Neither
// backend needs an explicit GPU-side re-upload call after this: in this
// pinned Dear ImGui version (v1.92.9b-docking), both imgui_impl_opengl3 and
// imgui_impl_vulkan removed their old explicit
// Create/DestroyFontsTexture() entry points in favor of automatically
// re-uploading any ImTextureData the atlas rebuild marks dirty
// (ImGuiBackendFlags_RendererHasTextures), read from ImDrawData::Textures
// the next time each backend's RenderDrawData runs. There is no Vulkan- or
// OpenGL-specific branch to take here.
void rebuild_font_atlas(float content_scale);

}  // namespace app

#endif  // APP_FONT_ATLAS_HPP
