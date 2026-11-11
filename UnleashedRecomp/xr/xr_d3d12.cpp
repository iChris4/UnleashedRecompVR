// Parts of this file are adapted from PrimedGun's openxr_d3d12.cpp
// (QueryGraphicsRequirements, SelectSwapchainFormat, swapchain image enumeration).
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xr_d3d12.h"

#include <plume_d3d12.h>

#define XR_USE_GRAPHICS_API_D3D12
#include <openxr/openxr_platform.h>

#include <vr/openxr_runtime.h>
#include <os/logger.h>

static XrGraphicsRequirementsD3D12KHR g_requirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR };
static bool g_requirementsQueried;
static XrGraphicsBindingD3D12KHR g_binding{ XR_TYPE_GRAPHICS_BINDING_D3D12_KHR };

static DXGI_FORMAT SrgbSibling(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

static std::string WideToUtf8(const wchar_t* text)
{
    int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1)
        return {};

    std::string result(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), size, nullptr, nullptr);
    return result;
}

bool XrD3D12::QueryGraphicsRequirements(PortVr::OpenXRRuntime& runtime)
{
    PFN_xrGetD3D12GraphicsRequirementsKHR getRequirements = nullptr;
    if (!runtime.LoadFunction("xrGetD3D12GraphicsRequirementsKHR", &getRequirements) || getRequirements == nullptr)
    {
        LOGN_ERROR("[VR] The OpenXR runtime does not expose xrGetD3D12GraphicsRequirementsKHR.");
        return false;
    }

    XrResult result = getRequirements(runtime.Instance(), runtime.SystemId(), &g_requirements);
    runtime.ObserveResult(result);
    if (XR_FAILED(result))
    {
        LOGFN_ERROR("[VR] xrGetD3D12GraphicsRequirementsKHR failed ({}).", int32_t(result));
        return false;
    }

    g_requirementsQueried = true;

    LOGFN("[VR] D3D12 adapter LUID {:08X}:{:08X}, minimum feature level 0x{:X}.",
        uint32_t(g_requirements.adapterLuid.HighPart), g_requirements.adapterLuid.LowPart, uint32_t(g_requirements.minFeatureLevel));

    return true;
}

std::string XrD3D12::GetAdapterName()
{
    if (!g_requirementsQueried)
        return {};

    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
        return {};

    std::string name;
    IDXGIAdapter1* adapter = nullptr;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++)
    {
        DXGI_ADAPTER_DESC1 desc;
        if (SUCCEEDED(adapter->GetDesc1(&desc)) &&
            desc.AdapterLuid.LowPart == g_requirements.adapterLuid.LowPart &&
            desc.AdapterLuid.HighPart == g_requirements.adapterLuid.HighPart)
        {
            name = WideToUtf8(desc.Description);
        }

        adapter->Release();

        if (!name.empty())
            break;
    }

    factory->Release();

    return name;
}

bool XrD3D12::IsDeviceOnRequiredAdapter(plume::RenderDevice* device)
{
    auto d3d12Device = static_cast<plume::D3D12Device*>(device);
    LUID luid = d3d12Device->d3d->GetAdapterLuid();
    return luid.LowPart == g_requirements.adapterLuid.LowPart && luid.HighPart == g_requirements.adapterLuid.HighPart;
}

const void* XrD3D12::GetGraphicsBinding(plume::RenderDevice* device, plume::RenderCommandQueue* queue)
{
    g_binding = { XR_TYPE_GRAPHICS_BINDING_D3D12_KHR };
    g_binding.device = static_cast<plume::D3D12Device*>(device)->d3d;
    g_binding.queue = static_cast<plume::D3D12CommandQueue*>(queue)->d3d;
    return &g_binding;
}

bool XrD3D12::SelectSwapchainFormat(const std::vector<int64_t>& formats, int64_t& swapchainFormat, plume::RenderFormat& renderFormat, bool& isSrgb)
{
    // The game's output is gamma-encoded UNORM. The swapchain must be its sRGB
    // sibling so the compositor decodes the bytes instead of treating them as
    // linear light (which looks washed out). The copy into the swapchain image
    // is legal because both formats share a typeless family.
    struct Candidate
    {
        DXGI_FORMAT format;
        plume::RenderFormat renderFormat;
    };

    const Candidate candidates[] =
    {
        { DXGI_FORMAT_B8G8R8A8_UNORM, plume::RenderFormat::B8G8R8A8_UNORM },
        { DXGI_FORMAT_R8G8B8A8_UNORM, plume::RenderFormat::R8G8B8A8_UNORM },
    };

    auto offered = [&](DXGI_FORMAT format)
    {
        return std::find(formats.begin(), formats.end(), int64_t(format)) != formats.end();
    };

    for (const Candidate& candidate : candidates)
    {
        DXGI_FORMAT srgb = SrgbSibling(candidate.format);
        if (offered(srgb))
        {
            swapchainFormat = int64_t(srgb);
            renderFormat = candidate.renderFormat;
            isSrgb = true;
            return true;
        }
    }

    for (const Candidate& candidate : candidates)
    {
        if (offered(candidate.format))
        {
            LOGN_WARNING("[VR] The runtime offers no 8-bit sRGB swapchain format; colours will look washed out.");
            swapchainFormat = int64_t(candidate.format);
            renderFormat = candidate.renderFormat;
            isSrgb = false;
            return true;
        }
    }

    LOGN_ERROR("[VR] The runtime offers no swapchain format the renderer can copy into.");
    return false;
}

XrResult XrD3D12::EnumerateSwapchainImages(XrSwapchain swapchain, uint32_t width, uint32_t height, plume::RenderFormat renderFormat,
    std::vector<std::unique_ptr<plume::RenderTexture>>& images)
{
    uint32_t count = 0;
    XrResult result = xrEnumerateSwapchainImages(swapchain, 0, &count, nullptr);
    if (XR_FAILED(result) || count == 0)
        return XR_FAILED(result) ? result : XR_ERROR_RUNTIME_FAILURE;

    std::vector<XrSwapchainImageD3D12KHR> d3d12Images(count, { XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR });
    result = xrEnumerateSwapchainImages(swapchain, count, &count, reinterpret_cast<XrSwapchainImageBaseHeader*>(d3d12Images.data()));
    if (XR_FAILED(result))
        return result;

    images.clear();

    for (uint32_t i = 0; i < count; i++)
    {
        // Same wrapping as plume's own swap chain textures: allocation stays null,
        // so the wrapper never releases the runtime's resource.
        auto texture = std::make_unique<plume::D3D12Texture>();
        texture->d3d = d3d12Images[i].texture;
        texture->desc.dimension = plume::RenderTextureDimension::TEXTURE_2D;
        texture->desc.width = width;
        texture->desc.height = height;
        texture->desc.depth = 1;
        texture->desc.mipLevels = 1;
        texture->desc.arraySize = 1;
        texture->desc.format = renderFormat;
        texture->desc.flags = plume::RenderTextureFlag::RENDER_TARGET;
        texture->resourceStates = D3D12_RESOURCE_STATE_RENDER_TARGET;
        texture->layout = plume::RenderTextureLayout::COLOR_WRITE;
        images.emplace_back(std::move(texture));
    }

    return XR_SUCCESS;
}
