#pragma once

#include <plume_render_interface.h>
#include <openxr/openxr.h>

namespace PortVr
{
    class OpenXRRuntime;
}

// The D3D12 half of the OpenXR layer. Everything that needs d3d12.h, plume's
// D3D12 backend or openxr_platform.h stays in xr_d3d12.cpp, so a Vulkan
// backend can later implement the same functions.
namespace XrD3D12
{
    inline constexpr const char* EXTENSION_NAME = "XR_KHR_D3D12_enable";

    // Must run after the OpenXR instance exists and before the D3D12 device is created.
    bool QueryGraphicsRequirements(PortVr::OpenXRRuntime& runtime);

    // The description of the DXGI adapter the runtime requires, for plume's
    // preferred device name. Empty when no adapter matches its LUID.
    std::string GetAdapterName();

    // True when the created device is on the adapter the runtime requires.
    bool IsDeviceOnRequiredAdapter(plume::RenderDevice* device);

    // XrGraphicsBindingD3D12KHR for xrCreateSession. Valid until the next call.
    const void* GetGraphicsBinding(plume::RenderDevice* device, plume::RenderCommandQueue* queue);

    // Picks the swapchain format among the runtime's, preferring the sRGB sibling
    // of a UNORM format the renderer can write and copy from. renderFormat is the
    // UNORM format to render the copy source in.
    bool SelectSwapchainFormat(const std::vector<int64_t>& formats, int64_t& swapchainFormat, plume::RenderFormat& renderFormat, bool& isSrgb);

    // Wraps the swapchain's images as plume textures that do not own them.
    // Images stay in D3D12_RESOURCE_STATE_RENDER_TARGET between frames, as the
    // OpenXR D3D12 binding requires.
    XrResult EnumerateSwapchainImages(XrSwapchain swapchain, uint32_t width, uint32_t height, plume::RenderFormat renderFormat,
        std::vector<std::unique_ptr<plume::RenderTexture>>& images);
}
