#ifndef APP_PPM_WRITER_HPP
#define APP_PPM_WRITER_HPP

#include <cstdint>
#include <filesystem>
#include <vector>

namespace app {

// Write an RGB image (top-down, row-major, 3 bytes per pixel) as a binary
// PPM (P6). Shared by the OpenGL screenshot path (Linux/macOS,
// app/main.cpp's glReadPixels capture) and the Vulkan screenshot path
// (Windows, app/vulkan_backend.cpp's swapchain-image-to-staging-buffer
// capture) so POLAR_PLOTTER_SCREENSHOT produces byte-for-byte identical PPM
// output regardless of platform/backend. Chosen for zero dependencies;
// convert to PNG with e.g. `magick shot.ppm shot.png`.
bool write_ppm(const std::filesystem::path& path, int width, int height,
               const std::vector<std::uint8_t>& rgb);

}  // namespace app

#endif  // APP_PPM_WRITER_HPP
