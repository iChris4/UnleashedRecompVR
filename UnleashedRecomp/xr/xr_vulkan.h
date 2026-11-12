#pragma once

#include <plume_render_interface.h>
#include <openxr/openxr.h>

#include <mutex>

namespace PortVr
{
    class OpenXRRuntime;
}

// The Vulkan half of the OpenXR layer (XR_KHR_vulkan_enable), the counterpart
// of xr_d3d12. Everything that needs plume's Vulkan backend or the Vulkan
// parts of openxr_platform.h stays in xr_vulkan.cpp.
namespace XrVulkan
{
    inline constexpr const char* EXTENSION_NAME = "XR_KHR_vulkan_enable";

    // Must run after the OpenXR instance exists and before plume creates its
    // Vulkan instance: queries the graphics requirements and has plume enable the
    // instance and device extensions the runtime requires.
    bool Prepare(PortVr::OpenXRRuntime& runtime);

    // The name of the physical device the runtime requires, for plume's preferred
    // device name, once plume's Vulkan instance exists. Empty when unknown.
    std::string GetDeviceName(PortVr::OpenXRRuntime& runtime, plume::RenderInterface* renderInterface);

    // True when the created device is on the physical device the runtime requires.
    bool IsDeviceOnRequiredGpu(PortVr::OpenXRRuntime& runtime, plume::RenderDevice* device);

    // XrGraphicsBindingVulkanKHR for xrCreateSession. Valid until the next call.
    // Also remembers the queue for LockQueue.
    const void* GetGraphicsBinding(plume::RenderDevice* device, plume::RenderCommandQueue* queue);

    // Picks the swapchain format among the runtime's, preferring the sRGB sibling
    // of a UNORM format the renderer can write and copy from. renderFormat is the
    // UNORM format to render the copy source in.
    bool SelectSwapchainFormat(const std::vector<int64_t>& formats, int64_t& swapchainFormat, plume::RenderFormat& renderFormat, bool& isSrgb);

    // Wraps the swapchain's images as plume textures that do not own them. The
    // runtime hands them over in VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL and
    // takes them back in it.
    XrResult EnumerateSwapchainImages(XrSwapchain swapchain, uint32_t width, uint32_t height, plume::RenderFormat renderFormat,
        plume::RenderDevice* device, std::vector<std::unique_ptr<plume::RenderTexture>>& images);

    // The runtime may use the session's queue in frame and swapchain calls, which
    // must then not overlap plume's submissions to it: plume submits under this
    // lock. An empty lock before the session exists.
    std::unique_lock<std::mutex> LockQueue();
}
