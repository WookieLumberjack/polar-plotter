#include "vulkan_backend.hpp"

// volk resolves every Vulkan entry point at runtime (no link-time import
// library, no Vulkan SDK), the same pairing Dear ImGui's own
// example_glfw_vulkan uses. It must be included before GLFW so GLFW's
// Vulkan-related declarations (guarded on `VK_VERSION_1_0`) become visible.
#include <volk.h>

// GLFW dynamically loads vulkan-1.dll itself; GLFW_INCLUDE_NONE keeps
// glfw3.h from pulling in its own OpenGL/Vulkan headers so volk's remain
// the single source of Vulkan declarations/dispatch.
#define GLFW_INCLUDE_NONE
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <optional>
#include <string_view>
#include <vector>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
// Built with IMGUI_IMPL_VULKAN_USE_VOLK defined PUBLICly on the `imgui`
// CMake target (see cmake/Dependencies.cmake), the same pairing Dear ImGui's
// own example_glfw_vulkan uses -- this header then includes <volk.h> itself
// (already included above) instead of <vulkan/vulkan.h>.
#include <imgui_impl_vulkan.h>
#include <implot.h>

#include "config_path.hpp"
#include "font_atlas.hpp"
#include "ppm_writer.hpp"
#include "ui/app.hpp"

namespace app {

namespace {

constexpr int kInitialWidth = 1280;
constexpr int kInitialHeight = 800;
constexpr std::uint32_t kFramesInFlight = 2;

// Matches the OpenGL path's clear color (see the non-Windows branch of
// main.cpp) so the two backends are visually equivalent -- this is the
// render pass's VK_ATTACHMENT_LOAD_OP_CLEAR color, painted under the ImGui
// UI every frame exactly like glClearColor+glClear does on the OpenGL path.
constexpr VkClearColorValue kClearColor = {{0.10F, 0.11F, 0.13F, 1.0F}};

void glfw_error_callback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

// Passed to ImGui_ImplVulkan_InitInfo::CheckVkResultFn: Dear ImGui's Vulkan
// backend calls this after Vulkan calls it makes internally (pipeline/
// descriptor/texture creation). A negative VkResult here indicates a real
// Vulkan error rather than e.g. VK_SUBOPTIMAL_KHR, so abort rather than
// continue with a backend left in an unknown state -- matching Dear ImGui's
// own example_glfw_vulkan's check_vk_result.
void check_vk_result(VkResult result) {
    if (result == VK_SUCCESS) {
        return;
    }
    std::fprintf(stderr, "[vulkan] Dear ImGui Vulkan backend error: VkResult %d\n",
                 static_cast<int>(result));
    if (result < 0) {
        std::abort();
    }
}

struct QueueFamilyIndices {
    std::optional<std::uint32_t> graphics;
    std::optional<std::uint32_t> present;

    [[nodiscard]] bool complete() const { return graphics.has_value() && present.has_value(); }
};

struct SwapchainSupport {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

QueueFamilyIndices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;

    std::uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    for (std::uint32_t i = 0; i < count; ++i) {
        if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
            indices.graphics = i;
        }
        VkBool32 present_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
        if (present_support == VK_TRUE) {
            indices.present = i;
        }
        if (indices.complete()) {
            break;
        }
    }
    return indices;
}

bool device_supports_swapchain_extension(VkPhysicalDevice device) {
    std::uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());

    return std::ranges::any_of(extensions, [](const VkExtensionProperties& ext) {
        return std::string_view(static_cast<const char*>(ext.extensionName)) ==
               VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    });
}

SwapchainSupport query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapchainSupport support;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &support.capabilities);

    std::uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
    support.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, support.formats.data());

    std::uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
    support.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count,
                                              support.present_modes.data());

    return support;
}

VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR>& formats) {
    // UNORM, not SRGB -- matching Dear ImGui's own reference example
    // (imgui/examples/example_glfw_vulkan/main.cpp's requestSurfaceImageFormat
    // list, all *_UNORM). ImGui's Vulkan backend writes already gamma-encoded
    // (sRGB) color bytes straight through with no shader-side linear->sRGB
    // conversion; an *_SRGB swapchain image format makes the GPU apply an
    // additional encode on write, double-gamma-correcting the whole UI --
    // every color washes out and blacks stop being fully black. The
    // VK_COLOR_SPACE_SRGB_NONLINEAR_KHR color space (near-universally the
    // only one offered) is still correct to request: that's the display's
    // output color space, orthogonal to the swapchain image's storage format.
    constexpr std::array<VkFormat, 4> kPreferredFormats{
        VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM,
        VK_FORMAT_R8G8B8_UNORM};
    for (const VkFormat preferred : kPreferredFormats) {
        for (const auto& format : formats) {
            if (format.format == preferred &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
    }
    return formats.front();
}

VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& modes) {
    // FIFO is the only mode every Vulkan implementation is required to
    // support; MAILBOX (low-latency triple buffering) is a nice-to-have.
    if (std::ranges::find(modes, VK_PRESENT_MODE_MAILBOX_KHR) != modes.end()) {
        return VK_PRESENT_MODE_MAILBOX_KHR;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_extent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D extent{static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width,
                              capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, capabilities.minImageExtent.height,
                               capabilities.maxImageExtent.height);
    return extent;
}

}  // namespace

// Owns the full Vulkan pipeline plus the Dear ImGui Vulkan backend on top of
// it: instance, surface, physical/logical device, swapchain, render pass,
// framebuffers, command buffers, sync objects, the descriptor pool
// imgui_impl_vulkan requires, and the ui::App instance itself. `init()`
// reports failure (with an actionable stderr message at the point of
// failure) instead of throwing -- there is no fallback path to unwind to, so
// a plain bool keeps the failure handling in `run_vulkan_app()` linear and
// obvious.
class VulkanApp {
public:
    // \p screenshot_path mirrors main.cpp's non-Windows POLAR_PLOTTER_SCREENSHOT
    // handling: null means normal interactive operation; non-null names the
    // PPM path to capture to after a few frames, then exit (see run()).
    VulkanApp(GLFWwindow* window, const char* screenshot_path)
        : window_(window), screenshot_path_(screenshot_path) {}

    VulkanApp(const VulkanApp&) = delete;
    VulkanApp& operator=(const VulkanApp&) = delete;
    VulkanApp(VulkanApp&&) = delete;
    VulkanApp& operator=(VulkanApp&&) = delete;

    ~VulkanApp() { cleanup(); }

    [[nodiscard]] bool init() {
        return create_instance() && create_surface() && pick_physical_device() &&
               create_logical_device() && create_swapchain() && create_image_views() &&
               create_render_pass() && create_framebuffers() && create_command_pool_and_buffers() &&
               create_sync_objects() && create_descriptor_pool() && init_imgui();
    }

    // Returns an exit code suitable for returning from run_vulkan_app():
    // EXIT_SUCCESS for normal interactive operation (window closed by the
    // user) or a completed screenshot capture, EXIT_FAILURE if
    // screenshot_path_ was set and the capture failed.
    [[nodiscard]] int run() {
        glfwSetWindowUserPointer(window_, this);
        glfwSetFramebufferSizeCallback(window_, [](GLFWwindow* win, int /*width*/, int /*height*/) {
            auto* self = static_cast<VulkanApp*>(glfwGetWindowUserPointer(win));
            self->framebuffer_resized_ = true;
        });
        // Same cross-platform DPI/content-scale handling as the non-Windows
        // path (see main.cpp's glfw_content_scale_callback and CONTEXT.md's
        // "content scale" entry): react to the window moving to a monitor
        // with a different content scale at runtime, not just at startup.
        glfwSetWindowContentScaleCallback(
            window_, [](GLFWwindow* win, float xscale, float /*yscale*/) {
                auto* self = static_cast<VulkanApp*>(glfwGetWindowUserPointer(win));
                self->on_content_scale_changed(xscale);
            });

        ui::App ui_app(config_path());
        // See ui::App::set_content_scale's doc comment: this both records
        // the scale read in init_imgui() and re-derives the theme's style
        // for it, since the constructor above applied the theme at the
        // default (unscaled) content_scale_ of 1.0.
        ui_app.set_content_scale(content_scale_);
        ui_app_ = &ui_app;

        // Same env-var-driven contract as the non-Windows path (see
        // main.cpp's screenshot_mode): render a few frames off-screen (the
        // window itself was created hidden -- see run_vulkan_app()), then
        // capture and exit, instead of running interactively forever.
        const bool screenshot_mode = screenshot_path_ != nullptr;
        int frame = 0;
        int exit_code = EXIT_SUCCESS;

        while (glfwWindowShouldClose(window_) == GLFW_FALSE) {
            glfwPollEvents();

            // Skip drawing while minimized: the framebuffer is 0x0, which
            // create_swapchain()/recreate_swapchain() already handle by
            // blocking on glfwWaitEvents() rather than accepting a 0x0
            // extent -- avoid re-entering that block on every iteration by
            // simply not drawing until the window is restored.
            if (glfwGetWindowAttrib(window_, GLFW_ICONIFIED) != 0) {
                continue;
            }

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ui_app.render();
            if (ui_app.want_exit()) {
                glfwSetWindowShouldClose(window_, GLFW_TRUE);
            }

            ImGui::Render();
            draw_frame(ImGui::GetDrawData());

            // Let ImGui settle (it lays out some widgets over two frames),
            // then grab the swapchain image and quit -- matching the
            // OpenGL path's frame-count-then-exit contract exactly (see
            // main.cpp's identical `++frame >= 8` check).
            if (screenshot_mode && ++frame >= 8) {
                if (!capture_screenshot()) {
                    exit_code = EXIT_FAILURE;
                }
                break;
            }
        }

        vkDeviceWaitIdle(device_);
        ui_app_ = nullptr;
        return exit_code;
    }  // ui_app.save() runs here

private:
    // Rebuilds the font atlas at the new effective pixel size (re-uploaded
    // to the GPU automatically -- see app::rebuild_font_atlas), then
    // re-derives ui::App's style from scratch for the new scale, exactly
    // like the non-Windows path's glfw_content_scale_callback.
    void on_content_scale_changed(float xscale) {
        content_scale_ = xscale;
        rebuild_font_atlas(xscale);
        if (ui_app_ != nullptr) {
            ui_app_->set_content_scale(xscale);
        }
    }

    void cleanup() {
        if (device_ != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device_);
        }

        if (imgui_vulkan_initialized_) {
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImPlot::DestroyContext();
            ImGui::DestroyContext();
        }

        if (descriptor_pool_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
        }

        for (auto& sem : image_available_semaphores_) {
            vkDestroySemaphore(device_, sem, nullptr);
        }
        for (auto& sem : render_finished_semaphores_) {
            vkDestroySemaphore(device_, sem, nullptr);
        }
        for (auto& fence : in_flight_fences_) {
            vkDestroyFence(device_, fence, nullptr);
        }
        if (command_pool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_, command_pool_, nullptr);
        }

        cleanup_swapchain();

        if (render_pass_ != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device_, render_pass_, nullptr);
        }
        if (device_ != VK_NULL_HANDLE) {
            vkDestroyDevice(device_, nullptr);
        }
        if (surface_ != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(instance_, surface_, nullptr);
        }
        if (instance_ != VK_NULL_HANDLE) {
            vkDestroyInstance(instance_, nullptr);
        }
    }

    void cleanup_swapchain() {
        for (auto& framebuffer : framebuffers_) {
            vkDestroyFramebuffer(device_, framebuffer, nullptr);
        }
        framebuffers_.clear();

        for (auto& view : swapchain_image_views_) {
            vkDestroyImageView(device_, view, nullptr);
        }
        swapchain_image_views_.clear();

        if (swapchain_ != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device_, swapchain_, nullptr);
            swapchain_ = VK_NULL_HANDLE;
        }
    }

    static void report_failure(const char* what, VkResult result) {
        std::fprintf(stderr, "Vulkan initialization failed: %s (VkResult %d).\n", what,
                     static_cast<int>(result));
    }

    [[nodiscard]] bool create_instance() {
        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "polar-plotter";
        app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        app_info.pEngineName = "no engine";
        app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;

        std::uint32_t glfw_extension_count = 0;
        const char* const* glfw_extensions =
            glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        if (glfw_extensions == nullptr) {
            std::fprintf(
                stderr,
                "Vulkan initialization failed: GLFW could not determine the required instance "
                "extensions. Ensure a Vulkan-capable GPU driver (providing vulkan-1.dll) is "
                "installed.\n");
            return false;
        }

        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledExtensionCount = glfw_extension_count;
        create_info.ppEnabledExtensionNames = glfw_extensions;
        create_info.enabledLayerCount = 0;

        const VkResult result = vkCreateInstance(&create_info, nullptr, &instance_);
        if (result != VK_SUCCESS) {
            report_failure("vkCreateInstance", result);
            return false;
        }

        volkLoadInstanceOnly(instance_);
        return true;
    }

    [[nodiscard]] bool create_surface() {
        const VkResult result = glfwCreateWindowSurface(instance_, window_, nullptr, &surface_);
        if (result != VK_SUCCESS) {
            report_failure("glfwCreateWindowSurface", result);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool pick_physical_device() {
        std::uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
        if (device_count == 0) {
            std::fprintf(
                stderr,
                "Vulkan initialization failed: no Vulkan-capable device was found. Install or "
                "update your GPU driver (it must provide vulkan-1.dll); the LunarG Vulkan SDK "
                "is not required.\n");
            return false;
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());

        VkPhysicalDevice best = VK_NULL_HANDLE;
        QueueFamilyIndices best_indices;
        bool best_is_discrete = false;

        for (VkPhysicalDevice device : devices) {
            if (!device_supports_swapchain_extension(device)) {
                continue;
            }
            const QueueFamilyIndices indices = find_queue_families(device, surface_);
            if (!indices.complete()) {
                continue;
            }
            const SwapchainSupport support = query_swapchain_support(device, surface_);
            if (support.formats.empty() || support.present_modes.empty()) {
                continue;
            }

            VkPhysicalDeviceProperties props{};
            vkGetPhysicalDeviceProperties(device, &props);
            const bool is_discrete = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

            // Prefer the first suitable device, but upgrade to a discrete
            // GPU if one shows up later in enumeration order.
            if (best == VK_NULL_HANDLE || (is_discrete && !best_is_discrete)) {
                best = device;
                best_indices = indices;
                best_is_discrete = is_discrete;
            }
        }

        if (best == VK_NULL_HANDLE) {
            std::fprintf(
                stderr,
                "Vulkan initialization failed: no Vulkan device with swapchain support and "
                "graphics+present queues was found for this window surface. Update your GPU "
                "driver, or check that the display adapter in use exposes Vulkan.\n");
            return false;
        }

        // The loop above only ever keeps a device whose QueueFamilyIndices
        // was `complete()`, so both optionals are guaranteed populated here;
        // unwrap them once into plain members so every later use is a
        // direct read instead of a repeated (and clang-tidy-flagged)
        // unchecked `.value()`.
        if (!best_indices.graphics.has_value() || !best_indices.present.has_value()) {
            return false;
        }

        physical_device_ = best;
        graphics_family_ = *best_indices.graphics;
        present_family_ = *best_indices.present;
        return true;
    }

    [[nodiscard]] bool create_logical_device() {
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        const std::array<std::uint32_t, 2> unique_families = {graphics_family_, present_family_};
        const float queue_priority = 1.0F;

        for (const std::uint32_t family : unique_families) {
            const bool already_added = std::ranges::any_of(
                queue_create_infos, [family](const VkDeviceQueueCreateInfo& info) {
                    return info.queueFamilyIndex == family;
                });
            if (already_added) {
                continue;
            }
            VkDeviceQueueCreateInfo queue_info{};
            queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info.queueFamilyIndex = family;
            queue_info.queueCount = 1;
            queue_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(queue_info);
        }

        const std::array<const char*, 1> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        const VkPhysicalDeviceFeatures features{};

        VkDeviceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = static_cast<std::uint32_t>(queue_create_infos.size());
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.pEnabledFeatures = &features;
        create_info.enabledExtensionCount = static_cast<std::uint32_t>(device_extensions.size());
        create_info.ppEnabledExtensionNames = device_extensions.data();

        const VkResult result = vkCreateDevice(physical_device_, &create_info, nullptr, &device_);
        if (result != VK_SUCCESS) {
            report_failure("vkCreateDevice", result);
            return false;
        }

        volkLoadDevice(device_);
        vkGetDeviceQueue(device_, graphics_family_, 0, &graphics_queue_);
        vkGetDeviceQueue(device_, present_family_, 0, &present_queue_);
        return true;
    }

    [[nodiscard]] bool create_swapchain() {
        // A minimized window reports a 0x0 framebuffer, which Vulkan
        // rejects as a swapchain extent; block here until it's restored
        // rather than failing the whole app over a transient minimize.
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window_, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window_, &width, &height);
            glfwWaitEvents();
        }

        const SwapchainSupport support = query_swapchain_support(physical_device_, surface_);
        const VkSurfaceFormatKHR surface_format = choose_surface_format(support.formats);
        const VkPresentModeKHR present_mode = choose_present_mode(support.present_modes);
        const VkExtent2D extent = choose_extent(window_, support.capabilities);

        std::uint32_t image_count = support.capabilities.minImageCount + 1;
        if (support.capabilities.maxImageCount > 0 &&
            image_count > support.capabilities.maxImageCount) {
            image_count = support.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = surface_;
        create_info.minImageCount = image_count;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = extent;
        create_info.imageArrayLayers = 1;
        // TRANSFER_SRC in addition to the color-attachment usage every frame
        // needs: POLAR_PLOTTER_SCREENSHOT's capture path (see
        // capture_screenshot()) copies the swapchain image straight to a
        // host-visible staging buffer via vkCmdCopyImageToBuffer, which
        // requires the image to have been created with this usage bit.
        create_info.imageUsage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        const std::array<std::uint32_t, 2> queue_indices = {graphics_family_, present_family_};
        if (graphics_family_ != present_family_) {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = static_cast<std::uint32_t>(queue_indices.size());
            create_info.pQueueFamilyIndices = queue_indices.data();
        } else {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        create_info.preTransform = support.capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = present_mode;
        create_info.clipped = VK_TRUE;
        create_info.oldSwapchain = VK_NULL_HANDLE;

        const VkResult result = vkCreateSwapchainKHR(device_, &create_info, nullptr, &swapchain_);
        if (result != VK_SUCCESS) {
            report_failure("vkCreateSwapchainKHR", result);
            return false;
        }

        std::uint32_t actual_image_count = 0;
        vkGetSwapchainImagesKHR(device_, swapchain_, &actual_image_count, nullptr);
        swapchain_images_.resize(actual_image_count);
        vkGetSwapchainImagesKHR(device_, swapchain_, &actual_image_count, swapchain_images_.data());

        swapchain_format_ = surface_format.format;
        swapchain_extent_ = extent;
        return true;
    }

    [[nodiscard]] bool create_image_views() {
        swapchain_image_views_.resize(swapchain_images_.size());
        for (std::size_t i = 0; i < swapchain_images_.size(); ++i) {
            VkImageViewCreateInfo view_info{};
            view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_info.image = swapchain_images_[i];
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format = swapchain_format_;
            view_info.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                                    VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
            view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view_info.subresourceRange.baseMipLevel = 0;
            view_info.subresourceRange.levelCount = 1;
            view_info.subresourceRange.baseArrayLayer = 0;
            view_info.subresourceRange.layerCount = 1;

            const VkResult result =
                vkCreateImageView(device_, &view_info, nullptr, &swapchain_image_views_[i]);
            if (result != VK_SUCCESS) {
                report_failure("vkCreateImageView", result);
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool create_render_pass() {
        VkAttachmentDescription color_attachment{};
        color_attachment.format = swapchain_format_;
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference color_ref{};
        color_ref.attachment = 0;
        color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_ref;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = 1;
        render_pass_info.pAttachments = &color_attachment;
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass;
        render_pass_info.dependencyCount = 1;
        render_pass_info.pDependencies = &dependency;

        const VkResult result =
            vkCreateRenderPass(device_, &render_pass_info, nullptr, &render_pass_);
        if (result != VK_SUCCESS) {
            report_failure("vkCreateRenderPass", result);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool create_framebuffers() {
        framebuffers_.resize(swapchain_image_views_.size());
        for (std::size_t i = 0; i < swapchain_image_views_.size(); ++i) {
            const std::array<VkImageView, 1> attachments = {swapchain_image_views_[i]};

            VkFramebufferCreateInfo framebuffer_info{};
            framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_info.renderPass = render_pass_;
            framebuffer_info.attachmentCount = static_cast<std::uint32_t>(attachments.size());
            framebuffer_info.pAttachments = attachments.data();
            framebuffer_info.width = swapchain_extent_.width;
            framebuffer_info.height = swapchain_extent_.height;
            framebuffer_info.layers = 1;

            const VkResult result =
                vkCreateFramebuffer(device_, &framebuffer_info, nullptr, &framebuffers_[i]);
            if (result != VK_SUCCESS) {
                report_failure("vkCreateFramebuffer", result);
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool create_command_pool_and_buffers() {
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = graphics_family_;

        VkResult result = vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_);
        if (result != VK_SUCCESS) {
            report_failure("vkCreateCommandPool", result);
            return false;
        }

        command_buffers_.resize(kFramesInFlight);
        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = command_pool_;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = static_cast<std::uint32_t>(command_buffers_.size());

        result = vkAllocateCommandBuffers(device_, &alloc_info, command_buffers_.data());
        if (result != VK_SUCCESS) {
            report_failure("vkAllocateCommandBuffers", result);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool create_sync_objects() {
        image_available_semaphores_.resize(kFramesInFlight);
        render_finished_semaphores_.resize(kFramesInFlight);
        in_flight_fences_.resize(kFramesInFlight);

        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (std::uint32_t i = 0; i < kFramesInFlight; ++i) {
            if (vkCreateSemaphore(device_, &semaphore_info, nullptr,
                                  &image_available_semaphores_[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device_, &semaphore_info, nullptr,
                                  &render_finished_semaphores_[i]) != VK_SUCCESS ||
                vkCreateFence(device_, &fence_info, nullptr, &in_flight_fences_[i]) != VK_SUCCESS) {
                std::fprintf(
                    stderr, "Vulkan initialization failed: could not create frame sync objects.\n");
                return false;
            }
        }
        return true;
    }

    // Dear ImGui's Vulkan backend needs its own VkDescriptorPool (for the
    // font atlas's combined image sampler, plus any future ImGui_ImplVulkan_
    // AddTexture() call) -- sized per imgui_impl_vulkan.h's documented
    // minimums, matching Dear ImGui's own example_glfw_vulkan. Owned and
    // destroyed by this class, not by the ImGui backend, since it's supplied
    // via InitInfo::DescriptorPool rather than the InitInfo::
    // DescriptorPoolSize convenience path (see imgui_impl_vulkan.h).
    [[nodiscard]] bool create_descriptor_pool() {
        const std::array<VkDescriptorPoolSize, 2> pool_sizes = {{
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE},
            {VK_DESCRIPTOR_TYPE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE},
        }};

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 0;
        for (const VkDescriptorPoolSize& size : pool_sizes) {
            pool_info.maxSets += size.descriptorCount;
        }
        pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();

        const VkResult result =
            vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_);
        if (result != VK_SUCCESS) {
            report_failure("vkCreateDescriptorPool", result);
            return false;
        }
        return true;
    }

    // Sets up Dear ImGui/ImPlot and the Vulkan renderer backend on top of
    // the pipeline created so far (render_pass_/device_/descriptor_pool_
    // etc. must already exist). Mirrors the non-Windows path's ImGui/ImPlot
    // context setup and config flags (see main.cpp) so behavior matches
    // across backends; only the renderer backend init (ImGui_ImplVulkan_Init
    // vs ImGui_ImplOpenGL3_Init) and platform backend init
    // (ImGui_ImplGlfw_InitForVulkan vs ...InitForOpenGL) differ.
    [[nodiscard]] bool init_imgui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // Docking branch (see docs/adr/0003-pin-imgui-to-docking-branch.md):
        // lets the "Vectors"/"Polar plot" windows dock into ui::App's
        // dockspace instead of floating freely.
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // See main.cpp's identical setting for why: the polar plot's
        // hand-rolled tip drag (#44/#48) needs window-move restricted to the
        // title bar so it doesn't fall through and move the window instead.
        ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;

        // Read the window's initial content scale so the very first font
        // atlas build (below) is already correct, without waiting for a
        // content-scale-changed callback that may never fire if the window
        // opens on its eventual monitor -- same as the non-Windows path.
        float xscale = 1.0F;
        float yscale_unused = 1.0F;
        glfwGetWindowContentScale(window_, &xscale, &yscale_unused);
        content_scale_ = xscale;
        rebuild_font_atlas(content_scale_);

        ImGui_ImplGlfw_InitForVulkan(window_, true);

        ImGui_ImplVulkan_InitInfo init_info{};
        init_info.ApiVersion = VK_API_VERSION_1_0;
        init_info.Instance = instance_;
        init_info.PhysicalDevice = physical_device_;
        init_info.Device = device_;
        init_info.QueueFamily = graphics_family_;
        init_info.Queue = graphics_queue_;
        init_info.DescriptorPool = descriptor_pool_;
        init_info.MinImageCount = kFramesInFlight;
        init_info.ImageCount = static_cast<std::uint32_t>(swapchain_images_.size());
        init_info.PipelineInfoMain.RenderPass = render_pass_;
        init_info.PipelineInfoMain.Subpass = 0;
        init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.CheckVkResultFn = check_vk_result;

        if (!ImGui_ImplVulkan_Init(&init_info)) {
            std::fprintf(stderr, "Vulkan initialization failed: ImGui_ImplVulkan_Init.\n");
            return false;
        }
        imgui_vulkan_initialized_ = true;
        return true;
    }

    [[nodiscard]] bool recreate_swapchain() {
        vkDeviceWaitIdle(device_);
        cleanup_swapchain();
        return create_swapchain() && create_image_views() && create_framebuffers();
    }

    // Records the render pass and, inside it, Dear ImGui's Vulkan draw data
    // -- the full ui::App UI, docked panels, polar plot and all -- via
    // ImGui_ImplVulkan_RenderDrawData(), the Vulkan analog of the OpenGL
    // path's ImGui_ImplOpenGL3_RenderDrawData() call in main.cpp.
    void record_command_buffer(VkCommandBuffer command_buffer, std::uint32_t image_index,
                               ImDrawData* draw_data) const {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command_buffer, &begin_info);

        VkClearValue clear_value{};
        clear_value.color = kClearColor;

        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = render_pass_;
        render_pass_info.framebuffer = framebuffers_[image_index];
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = swapchain_extent_;
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clear_value;

        vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);
        vkCmdEndRenderPass(command_buffer);
        vkEndCommandBuffer(command_buffer);
    }

    // Vulkan equivalent of main.cpp's non-Windows read_framebuffer()+write_ppm
    // capture: copies the most recently presented swapchain image
    // (swapchain_images_[last_image_index_], already in
    // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR thanks to the render pass's
    // finalLayout) to a host-visible/coherent staging buffer via
    // vkCmdCopyImageToBuffer, maps it, converts to the same top-down,
    // 3-byte-per-pixel RGB layout the OpenGL path produces (dropping alpha
    // and swapping channel order if the swapchain format is BGRA), and
    // writes it out with the same shared app::write_ppm the OpenGL path
    // uses -- so POLAR_PLOTTER_SCREENSHOT's output is byte-for-byte
    // indistinguishable across backends. Unlike the OpenGL framebuffer (which
    // is bottom-up and needs a vertical flip), a Vulkan swapchain image's row
    // 0 is already the top row, so no flip is needed here.
    [[nodiscard]] bool capture_screenshot() const {
        // Everything in flight must be finished and the presented image's
        // contents finalized before we read it back.
        vkDeviceWaitIdle(device_);

        const std::uint32_t width = swapchain_extent_.width;
        const std::uint32_t height = swapchain_extent_.height;
        const auto buffer_size =
            static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * 4;

        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = buffer_size;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer staging_buffer = VK_NULL_HANDLE;
        if (vkCreateBuffer(device_, &buffer_info, nullptr, &staging_buffer) != VK_SUCCESS) {
            std::fprintf(stderr, "failed to write screenshot to %s\n", screenshot_path_);
            return false;
        }

        VkMemoryRequirements mem_requirements{};
        vkGetBufferMemoryRequirements(device_, staging_buffer, &mem_requirements);

        VkPhysicalDeviceMemoryProperties mem_properties{};
        vkGetPhysicalDeviceMemoryProperties(physical_device_, &mem_properties);

        constexpr VkMemoryPropertyFlags kWanted =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        std::uint32_t memory_type_index = std::numeric_limits<std::uint32_t>::max();
        for (std::uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i) {
            const bool type_ok = (mem_requirements.memoryTypeBits & (1U << i)) != 0U;
            const bool props_ok =
                (mem_properties.memoryTypes[i].propertyFlags & kWanted) == kWanted;
            if (type_ok && props_ok) {
                memory_type_index = i;
                break;
            }
        }
        if (memory_type_index == std::numeric_limits<std::uint32_t>::max()) {
            vkDestroyBuffer(device_, staging_buffer, nullptr);
            std::fprintf(stderr, "failed to write screenshot to %s\n", screenshot_path_);
            return false;
        }

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = memory_type_index;

        VkDeviceMemory staging_memory = VK_NULL_HANDLE;
        if (vkAllocateMemory(device_, &alloc_info, nullptr, &staging_memory) != VK_SUCCESS) {
            vkDestroyBuffer(device_, staging_buffer, nullptr);
            std::fprintf(stderr, "failed to write screenshot to %s\n", screenshot_path_);
            return false;
        }
        vkBindBufferMemory(device_, staging_buffer, staging_memory, 0);

        VkCommandBufferAllocateInfo cmd_alloc_info{};
        cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_alloc_info.commandPool = command_pool_;
        cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_alloc_info.commandBufferCount = 1;

        VkCommandBuffer cmd = VK_NULL_HANDLE;
        if (vkAllocateCommandBuffers(device_, &cmd_alloc_info, &cmd) != VK_SUCCESS) {
            vkDestroyBuffer(device_, staging_buffer, nullptr);
            vkFreeMemory(device_, staging_memory, nullptr);
            std::fprintf(stderr, "failed to write screenshot to %s\n", screenshot_path_);
            return false;
        }

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &begin_info);

        // The image is already idle (we just waited above) and its layout
        // is PRESENT_SRC_KHR from the render pass's finalLayout; move it to
        // TRANSFER_SRC_OPTIMAL for the copy below. TOP_OF_PIPE->TRANSFER is
        // sufficient since vkDeviceWaitIdle above already established a full
        // execution barrier.
        VkImageMemoryBarrier to_transfer_src{};
        to_transfer_src.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        to_transfer_src.srcAccessMask = 0;
        to_transfer_src.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        to_transfer_src.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        to_transfer_src.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        to_transfer_src.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_transfer_src.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_transfer_src.image = swapchain_images_[last_image_index_];
        to_transfer_src.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &to_transfer_src);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        // 0/0 mean tightly packed (no row padding), matching the tightly
        // packed RGB buffer app::write_ppm expects.
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyImageToBuffer(cmd, swapchain_images_[last_image_index_],
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, staging_buffer, 1, &region);

        vkEndCommandBuffer(cmd);

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd;

        bool ok = true;
        if (vkQueueSubmit(graphics_queue_, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
            ok = false;
        } else {
            vkQueueWaitIdle(graphics_queue_);
        }
        vkFreeCommandBuffers(device_, command_pool_, 1, &cmd);

        if (!ok) {
            vkDestroyBuffer(device_, staging_buffer, nullptr);
            vkFreeMemory(device_, staging_memory, nullptr);
            std::fprintf(stderr, "failed to write screenshot to %s\n", screenshot_path_);
            return false;
        }

        // vkMapMemory's out-parameter must be a plain (non-const) void*; cast
        // to const immediately afterward since nothing here ever writes
        // through it, satisfying misc-const-correctness cleanly instead of
        // fighting the Vulkan API's own signature.
        void* mapped_raw = nullptr;  // NOLINT(misc-const-correctness) -- vkMapMemory's
                                     // void** out-param requires a non-const pointee.
        vkMapMemory(device_, staging_memory, 0, buffer_size, 0, &mapped_raw);
        const void* mapped = mapped_raw;

        // Match choose_surface_format()'s preference: VK_FORMAT_B8G8R8A8_UNORM
        // (its top pick in practice -- essentially every real swapchain
        // offers it) stores channels as B,G,R,A in memory. Any other format
        // this (non-exhaustive) fallback might pick is assumed R,G,B,A --
        // true of every other commonly-exposed 8-bit UNORM/SRGB surface
        // format. VK_FORMAT_B8G8R8A8_SRGB is included too even though
        // choose_surface_format() no longer prefers it, purely for
        // robustness against a future change to that preference.
        const bool bgr_order = swapchain_format_ == VK_FORMAT_B8G8R8A8_UNORM ||
                               swapchain_format_ == VK_FORMAT_B8G8R8A8_SRGB;

        const auto* pixels = static_cast<const std::uint8_t*>(mapped);
        const auto pixel_count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        std::vector<std::uint8_t> rgb(pixel_count * 3);
        for (std::size_t i = 0; i < pixel_count; ++i) {
            const std::uint8_t* src = pixels + (i * 4);
            std::uint8_t* dst = rgb.data() + (i * 3);
            if (bgr_order) {
                dst[0] = src[2];
                dst[1] = src[1];
                dst[2] = src[0];
            } else {
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
            }
        }

        vkUnmapMemory(device_, staging_memory);
        vkDestroyBuffer(device_, staging_buffer, nullptr);
        vkFreeMemory(device_, staging_memory, nullptr);

        if (!write_ppm(screenshot_path_, static_cast<int>(width), static_cast<int>(height), rgb)) {
            std::fprintf(stderr, "failed to write screenshot to %s\n", screenshot_path_);
            return false;
        }
        std::fprintf(stderr, "wrote %ux%u screenshot to %s\n", width, height, screenshot_path_);
        return true;
    }

    void draw_frame(ImDrawData* draw_data) {
        vkWaitForFences(device_, 1, &in_flight_fences_[current_frame_], VK_TRUE,
                        std::numeric_limits<std::uint64_t>::max());

        std::uint32_t image_index = 0;
        VkResult result = vkAcquireNextImageKHR(
            device_, swapchain_, std::numeric_limits<std::uint64_t>::max(),
            image_available_semaphores_[current_frame_], VK_NULL_HANDLE, &image_index);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            static_cast<void>(recreate_swapchain());
            return;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            report_failure("vkAcquireNextImageKHR", result);
            return;
        }

        last_image_index_ = image_index;
        vkResetFences(device_, 1, &in_flight_fences_[current_frame_]);

        VkCommandBuffer command_buffer = command_buffers_[current_frame_];
        vkResetCommandBuffer(command_buffer, 0);
        record_command_buffer(command_buffer, image_index, draw_data);

        const std::array<VkSemaphore, 1> wait_semaphores = {
            image_available_semaphores_[current_frame_]};
        const std::array<VkPipelineStageFlags, 1> wait_stages = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        const std::array<VkSemaphore, 1> signal_semaphores = {
            render_finished_semaphores_[current_frame_]};

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = static_cast<std::uint32_t>(wait_semaphores.size());
        submit_info.pWaitSemaphores = wait_semaphores.data();
        submit_info.pWaitDstStageMask = wait_stages.data();
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        submit_info.signalSemaphoreCount = static_cast<std::uint32_t>(signal_semaphores.size());
        submit_info.pSignalSemaphores = signal_semaphores.data();

        result = vkQueueSubmit(graphics_queue_, 1, &submit_info, in_flight_fences_[current_frame_]);
        if (result != VK_SUCCESS) {
            report_failure("vkQueueSubmit", result);
            return;
        }

        const std::array<VkSwapchainKHR, 1> swapchains = {swapchain_};
        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = static_cast<std::uint32_t>(signal_semaphores.size());
        present_info.pWaitSemaphores = signal_semaphores.data();
        present_info.swapchainCount = static_cast<std::uint32_t>(swapchains.size());
        present_info.pSwapchains = swapchains.data();
        present_info.pImageIndices = &image_index;

        result = vkQueuePresentKHR(present_queue_, &present_info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
            framebuffer_resized_) {
            framebuffer_resized_ = false;
            static_cast<void>(recreate_swapchain());
        } else if (result != VK_SUCCESS) {
            report_failure("vkQueuePresentKHR", result);
        }

        current_frame_ = (current_frame_ + 1) % kFramesInFlight;
    }

    GLFWwindow* window_;

    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphics_queue_ = VK_NULL_HANDLE;
    VkQueue present_queue_ = VK_NULL_HANDLE;
    std::uint32_t graphics_family_ = 0;
    std::uint32_t present_family_ = 0;

    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchain_images_;
    VkFormat swapchain_format_ = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchain_extent_{};
    std::vector<VkImageView> swapchain_image_views_;

    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers_;

    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> command_buffers_;

    std::vector<VkSemaphore> image_available_semaphores_;
    std::vector<VkSemaphore> render_finished_semaphores_;
    std::vector<VkFence> in_flight_fences_;
    std::size_t current_frame_ = 0;
    // Index into swapchain_images_ last returned by vkAcquireNextImageKHR,
    // used by capture_screenshot() to know which swapchain image to read
    // back from.
    std::uint32_t last_image_index_ = 0;
    bool framebuffer_resized_ = false;

    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    bool imgui_vulkan_initialized_ = false;
    float content_scale_ = 1.0F;
    // Non-owning; valid only during run()'s while loop (see
    // on_content_scale_changed's doc comment) -- ui:: never depends on GLFW
    // itself, so this is how the content-scale callback below reaches it.
    ui::App* ui_app_ = nullptr;
    // POLAR_PLOTTER_SCREENSHOT's target path, or nullptr for normal
    // interactive operation -- see the constructor and run()'s doc comment.
    const char* screenshot_path_ = nullptr;
};

int run_vulkan_app() {
    // Same env-var contract as the non-Windows path (see main.cpp): when
    // set, render a few frames off-screen, dump the current swapchain image
    // to this path as a PPM, and exit. Lets CI / a headless box produce a
    // screenshot without a display server or window manager, exactly like
    // the OpenGL path does on Linux/macOS.
    const char* shot_path = std::getenv("POLAR_PLOTTER_SCREENSHOT");
    const bool screenshot_mode = shot_path != nullptr;

    glfwSetErrorCallback(glfw_error_callback);
    if (glfwInit() == GLFW_FALSE) {
        std::fprintf(stderr, "Failed to initialize GLFW.\n");
        return EXIT_FAILURE;
    }

    if (glfwVulkanSupported() == GLFW_FALSE) {
        std::fprintf(
            stderr,
            "Vulkan initialization failed: GLFW could not find a Vulkan loader (vulkan-1.dll) on "
            "this system. Install or update your GPU driver -- it must provide the Vulkan runtime; "
            "no separate Vulkan SDK install is required.\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    if (volkInitialize() != VK_SUCCESS) {
        std::fprintf(
            stderr,
            "Vulkan initialization failed: volk could not load the Vulkan loader. Install or "
            "update your GPU driver -- it must provide vulkan-1.dll.\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Same cross-platform DPI/content-scale policy as the non-Windows path
    // (see main.cpp and CONTEXT.md's "content scale" entry): have GLFW
    // itself resize the window to match a monitor's content scale rather
    // than leaving it at its requested logical size -- must be set before
    // window creation.
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    if (screenshot_mode) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }
    GLFWwindow* window =
        glfwCreateWindow(kInitialWidth, kInitialHeight, "polar-plotter", nullptr, nullptr);
    if (window == nullptr) {
        std::fprintf(stderr, "Failed to create the application window.\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    int exit_code = EXIT_SUCCESS;
    {
        VulkanApp vulkan(window, shot_path);
        if (!vulkan.init()) {
            // vulkan.init() has already printed an actionable message for
            // whichever step failed; no silent fallback to OpenGL (hard
            // cutover, see docs/adr/0004-windows-vulkan-hard-cutover.md).
            exit_code = EXIT_FAILURE;
        } else {
            exit_code = vulkan.run();
        }
    }  // ~VulkanApp tears down whatever it managed to create.

    glfwDestroyWindow(window);
    glfwTerminate();
    return exit_code;
}

}  // namespace app
