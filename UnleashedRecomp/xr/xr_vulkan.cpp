// The Vulkan counterpart of xr_d3d12.cpp (XR_KHR_vulkan_enable).
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xr_vulkan.h"

#include <plume_vulkan.h>

#define XR_USE_GRAPHICS_API_VULKAN
#include <openxr/openxr_platform.h>

#include <vr/openxr_runtime.h>
#include <os/logger.h>

#include <algorithm>
#include <sstream>

static XrGraphicsRequirementsVulkanKHR g_requirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR };
static XrGraphicsBindingVulkanKHR g_binding{ XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR };
static plume::VulkanCommandQueue* g_queue;

// The runtime's extension lists: one space-separated string.
template<typename Function>
static bool GetExtensionList(PortVr::OpenXRRuntime& runtime, const char* functionName, std::vector<std::string>& extensions)
{
    Function getExtensions = nullptr;
    if (!runtime.LoadFunction(functionName, &getExtensions) || getExtensions == nullptr)
    {
        LOGFN_ERROR("[VR] The OpenXR runtime does not expose {}.", functionName);
        return false;
    }

    uint32_t size = 0;
    XrResult result = getExtensions(runtime.Instance(), runtime.SystemId(), 0, &size, nullptr);
    runtime.ObserveResult(result);
    if (XR_FAILED(result))
    {
        LOGFN_ERROR("[VR] {} failed ({}).", functionName, int32_t(result));
        return false;
    }

    std::string names(size, '\0');
    result = getExtensions(runtime.Instance(), runtime.SystemId(), size, &size, names.data());
    runtime.ObserveResult(result);
    if (XR_FAILED(result))
    {
        LOGFN_ERROR("[VR] {} failed ({}).", functionName, int32_t(result));
        return false;
    }

    std::istringstream stream(names.c_str());
    for (std::string name; stream >> name;)
        extensions.push_back(name);

    return true;
}

bool XrVulkan::Prepare(PortVr::OpenXRRuntime& runtime)
{
    PFN_xrGetVulkanGraphicsRequirementsKHR getRequirements = nullptr;
    if (!runtime.LoadFunction("xrGetVulkanGraphicsRequirementsKHR", &getRequirements) || getRequirements == nullptr)
    {
        LOGN_ERROR("[VR] The OpenXR runtime does not expose xrGetVulkanGraphicsRequirementsKHR.");
        return false;
    }

    XrResult result = getRequirements(runtime.Instance(), runtime.SystemId(), &g_requirements);
    runtime.ObserveResult(result);
    if (XR_FAILED(result))
    {
        LOGFN_ERROR("[VR] xrGetVulkanGraphicsRequirementsKHR failed ({}).", int32_t(result));
        return false;
    }

    // plume creates a Vulkan 1.2 instance.
    if (XR_VERSION_MAJOR(g_requirements.minApiVersionSupported) > 1 ||
        (XR_VERSION_MAJOR(g_requirements.minApiVersionSupported) == 1 && XR_VERSION_MINOR(g_requirements.minApiVersionSupported) > 2))
    {
        LOGN_ERROR("[VR] The OpenXR runtime requires a newer Vulkan version than the game uses.");
        return false;
    }

    std::vector<std::string> instanceExtensions, deviceExtensions;
    if (!GetExtensionList<PFN_xrGetVulkanInstanceExtensionsKHR>(runtime, "xrGetVulkanInstanceExtensionsKHR", instanceExtensions) ||
        !GetExtensionList<PFN_xrGetVulkanDeviceExtensionsKHR>(runtime, "xrGetVulkanDeviceExtensionsKHR", deviceExtensions))
    {
        return false;
    }

    std::string instanceList, deviceList;
    for (const std::string& name : instanceExtensions)
        instanceList += (instanceList.empty() ? "" : " ") + name;
    for (const std::string& name : deviceExtensions)
        deviceList += (deviceList.empty() ? "" : " ") + name;

    LOGFN("[VR] Vulkan instance extensions for the runtime: {}.", instanceList.empty() ? "none" : instanceList);
    LOGFN("[VR] Vulkan device extensions for the runtime: {}.", deviceList.empty() ? "none" : deviceList);

    plume::VulkanAdditionalInstanceExtensions = std::move(instanceExtensions);
    plume::VulkanAdditionalDeviceExtensions = std::move(deviceExtensions);
    return true;
}

static VkPhysicalDevice GetRequiredPhysicalDevice(PortVr::OpenXRRuntime& runtime, VkInstance instance)
{
    PFN_xrGetVulkanGraphicsDeviceKHR getDevice = nullptr;
    if (instance == VK_NULL_HANDLE || !runtime.LoadFunction("xrGetVulkanGraphicsDeviceKHR", &getDevice) || getDevice == nullptr)
        return VK_NULL_HANDLE;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    XrResult result = getDevice(runtime.Instance(), runtime.SystemId(), instance, &physicalDevice);
    runtime.ObserveResult(result);
    if (XR_FAILED(result))
    {
        LOGFN_ERROR("[VR] xrGetVulkanGraphicsDeviceKHR failed ({}).", int32_t(result));
        return VK_NULL_HANDLE;
    }

    return physicalDevice;
}

std::string XrVulkan::GetDeviceName(PortVr::OpenXRRuntime& runtime, plume::RenderInterface* renderInterface)
{
    auto vulkanInterface = static_cast<plume::VulkanInterface*>(renderInterface);
    VkPhysicalDevice physicalDevice = GetRequiredPhysicalDevice(runtime, vulkanInterface != nullptr ? vulkanInterface->instance : VK_NULL_HANDLE);
    if (physicalDevice == VK_NULL_HANDLE)
        return {};

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    LOGFN("[VR] The OpenXR runtime renders on '{}'.", properties.deviceName);
    return properties.deviceName;
}

bool XrVulkan::IsDeviceOnRequiredGpu(PortVr::OpenXRRuntime& runtime, plume::RenderDevice* device)
{
    auto vulkanDevice = static_cast<plume::VulkanDevice*>(device);
    return vulkanDevice->physicalDevice == GetRequiredPhysicalDevice(runtime, vulkanDevice->renderInterface->instance);
}

const void* XrVulkan::GetGraphicsBinding(plume::RenderDevice* device, plume::RenderCommandQueue* queue)
{
    auto vulkanDevice = static_cast<plume::VulkanDevice*>(device);
    g_queue = static_cast<plume::VulkanCommandQueue*>(queue);

    g_binding = { XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR };
    g_binding.instance = vulkanDevice->renderInterface->instance;
    g_binding.physicalDevice = vulkanDevice->physicalDevice;
    g_binding.device = vulkanDevice->vk;
    g_binding.queueFamilyIndex = g_queue->familyIndex;
    g_binding.queueIndex = g_queue->queueIndex;
    return &g_binding;
}

bool XrVulkan::SelectSwapchainFormat(const std::vector<int64_t>& formats, int64_t& swapchainFormat, plume::RenderFormat& renderFormat, bool& isSrgb)
{
    // As on D3D12: the game's output is gamma-encoded UNORM, copied into the
    // sRGB sibling so the compositor decodes it. Vulkan allows copies between
    // formats of the same texel size.
    struct Candidate
    {
        VkFormat format;
        VkFormat srgbFormat;
        plume::RenderFormat renderFormat;
    };

    const Candidate candidates[] =
    {
        { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SRGB, plume::RenderFormat::B8G8R8A8_UNORM },
        { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_SRGB, plume::RenderFormat::R8G8B8A8_UNORM },
    };

    auto offered = [&](VkFormat format)
    {
        return std::find(formats.begin(), formats.end(), int64_t(format)) != formats.end();
    };

    for (const Candidate& candidate : candidates)
    {
        if (offered(candidate.srgbFormat))
        {
            swapchainFormat = int64_t(candidate.srgbFormat);
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

XrResult XrVulkan::EnumerateSwapchainImages(XrSwapchain swapchain, uint32_t width, uint32_t height, plume::RenderFormat renderFormat,
    plume::RenderDevice* device, std::vector<std::unique_ptr<plume::RenderTexture>>& images)
{
    uint32_t count = 0;
    XrResult result = xrEnumerateSwapchainImages(swapchain, 0, &count, nullptr);
    if (XR_FAILED(result) || count == 0)
        return XR_FAILED(result) ? result : XR_ERROR_RUNTIME_FAILURE;

    std::vector<XrSwapchainImageVulkanKHR> vulkanImages(count, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR });
    result = xrEnumerateSwapchainImages(swapchain, count, &count, reinterpret_cast<XrSwapchainImageBaseHeader*>(vulkanImages.data()));
    if (XR_FAILED(result))
        return result;

    images.clear();

    for (uint32_t i = 0; i < count; i++)
    {
        // Same wrapping as plume's own swap chain textures: the wrapper does not
        // own the image. Only copies and barriers use it, so it needs no view.
        auto texture = std::make_unique<plume::VulkanTexture>(static_cast<plume::VulkanDevice*>(device), vulkanImages[i].image);
        texture->desc.dimension = plume::RenderTextureDimension::TEXTURE_2D;
        texture->desc.width = width;
        texture->desc.height = height;
        texture->desc.depth = 1;
        texture->desc.mipLevels = 1;
        texture->desc.arraySize = 1;
        texture->desc.format = renderFormat;
        texture->desc.flags = plume::RenderTextureFlag::RENDER_TARGET;
        texture->fillSubresourceRange();
        texture->textureLayout = plume::RenderTextureLayout::COLOR_WRITE;
        texture->barrierStages = plume::RenderBarrierStage::GRAPHICS;
        images.emplace_back(std::move(texture));
    }

    return XR_SUCCESS;
}

std::unique_lock<std::mutex> XrVulkan::LockQueue()
{
    if (g_queue == nullptr || g_queue->queue == nullptr || g_queue->queue->mutex == nullptr)
        return {};

    return std::unique_lock<std::mutex>(*g_queue->queue->mutex);
}
