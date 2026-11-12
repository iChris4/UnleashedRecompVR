#include "xr.h"
#include "xr_d3d12.h"
#include "xr_input.h"

#include <vr/openxr_runtime.h>
#include <os/logger.h>
#include <user/config.h>

struct XrSwapchainState
{
    XrSwapchain handle = XR_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<std::unique_ptr<plume::RenderTexture>> images;
    uint32_t acquiredIndex = 0;
    bool acquired = false;
};

static std::unique_ptr<PortVr::OpenXRRuntime> g_runtime;
static std::mutex g_mutex;

static int64_t g_swapchainFormat;
static plume::RenderFormat g_renderFormat = plume::RenderFormat::B8G8R8A8_UNORM;
static bool g_swapchainSrgb;

static uint32_t g_screenWidth = 1920;
static uint32_t g_screenHeight = 1080;
static XrSwapchainState g_screenSwapchain;

static PortVr::OpenXRFrame g_frame;
static bool g_frameBegun;
static std::atomic<bool> g_inputFocused;

// The screen is placed once in the application space, in front of the head
// with the head's heading, and stays there while the player looks around.
static bool g_screenPlaced;
static XrPosef g_screenPose{ { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } };

static void LogOpenXR(PortVr::OpenXRLogLevel level, std::string_view message)
{
    switch (level)
    {
    case PortVr::OpenXRLogLevel::Info:
        LOGFN("[VR] {}", message);
        break;
    case PortVr::OpenXRLogLevel::Warning:
        LOGFN_WARNING("[VR] {}", message);
        break;
    case PortVr::OpenXRLogLevel::Error:
        LOGFN_ERROR("[VR] {}", message);
        break;
    }
}

static void DestroySwapchain(XrSwapchainState& swapchain)
{
    swapchain.images.clear();

    if (swapchain.handle != XR_NULL_HANDLE)
        xrDestroySwapchain(swapchain.handle);

    swapchain = {};
}

static bool CreateSwapchain(XrSwapchainState& swapchain, uint32_t width, uint32_t height, const char* name)
{
    XrSwapchainCreateInfo createInfo{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
    createInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
    if (g_swapchainSrgb)
        createInfo.usageFlags |= XR_SWAPCHAIN_USAGE_MUTABLE_FORMAT_BIT;

    createInfo.format = g_swapchainFormat;
    createInfo.sampleCount = 1;
    createInfo.width = width;
    createInfo.height = height;
    createInfo.faceCount = 1;
    createInfo.arraySize = 1;
    createInfo.mipCount = 1;

    XrResult result = xrCreateSwapchain(g_runtime->Session(), &createInfo, &swapchain.handle);
    g_runtime->ObserveResult(result);
    if (XR_FAILED(result))
    {
        LOGFN_ERROR("[VR] xrCreateSwapchain failed for the {} swapchain ({}).", name, int32_t(result));
        swapchain.handle = XR_NULL_HANDLE;
        return false;
    }

    swapchain.width = width;
    swapchain.height = height;

    result = XrD3D12::EnumerateSwapchainImages(swapchain.handle, width, height, g_renderFormat, swapchain.images);
    g_runtime->ObserveResult(result);
    if (XR_FAILED(result))
    {
        LOGFN_ERROR("[VR] xrEnumerateSwapchainImages failed for the {} swapchain ({}).", name, int32_t(result));
        DestroySwapchain(swapchain);
        return false;
    }

    LOGFN("[VR] Created the {} swapchain: {}x{}, format {}, {} images.", name, width, height, g_swapchainFormat, swapchain.images.size());
    return true;
}

static bool AcquireSwapchain(XrSwapchainState& swapchain)
{
    if (swapchain.acquired)
        return true;

    XrSwapchainImageAcquireInfo acquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
    XrResult result = xrAcquireSwapchainImage(swapchain.handle, &acquireInfo, &swapchain.acquiredIndex);
    g_runtime->ObserveResult(result);
    if (XR_FAILED(result))
    {
        LOGFN_ERROR("[VR] xrAcquireSwapchainImage failed ({}).", int32_t(result));
        return false;
    }

    swapchain.acquired = true;

    XrSwapchainImageWaitInfo waitInfo{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
    waitInfo.timeout = XR_INFINITE_DURATION;
    result = xrWaitSwapchainImage(swapchain.handle, &waitInfo);
    g_runtime->ObserveResult(result);
    if (XR_FAILED(result) || result == XR_TIMEOUT_EXPIRED || swapchain.acquiredIndex >= swapchain.images.size())
    {
        // OpenXR forbids releasing an image whose wait did not succeed. Leave it
        // acquired; the session teardown destroys it.
        LOGFN_ERROR("[VR] xrWaitSwapchainImage failed ({}).", int32_t(result));
        return false;
    }

    return true;
}

static bool ReleaseSwapchain(XrSwapchainState& swapchain)
{
    if (!swapchain.acquired)
        return false;

    XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
    XrResult result = xrReleaseSwapchainImage(swapchain.handle, &releaseInfo);
    g_runtime->ObserveResult(result);
    swapchain.acquired = false;

    if (XR_FAILED(result))
    {
        LOGFN_ERROR("[VR] xrReleaseSwapchainImage failed ({}).", int32_t(result));
        return false;
    }

    return true;
}

static void PlaceScreen()
{
    XrSpaceLocation location{ XR_TYPE_SPACE_LOCATION };
    XrResult result = xrLocateSpace(g_runtime->ViewSpace(), g_runtime->AppSpace(), g_frame.predicted_display_time, &location);
    if (XR_FAILED(result))
        return;

    constexpr XrSpaceLocationFlags RequiredFlags = XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT;
    if ((location.locationFlags & RequiredFlags) != RequiredFlags)
        return;

    // The head's forward (-Z) in the application space, flattened to its heading.
    const XrQuaternionf& q = location.pose.orientation;
    float forwardX = -2.0f * (q.x * q.z + q.w * q.y);
    float forwardZ = -(1.0f - 2.0f * (q.x * q.x + q.y * q.y));
    float yaw = 0.0f;
    if ((forwardX * forwardX + forwardZ * forwardZ) > 1.0e-6f)
        yaw = atan2f(-forwardX, -forwardZ);

    float distance = std::max(0.25f, Config::VRScreenDistance.Value);

    g_screenPose.orientation = { 0.0f, sinf(0.5f * yaw), 0.0f, cosf(0.5f * yaw) };
    g_screenPose.position.x = location.pose.position.x - sinf(yaw) * distance;
    g_screenPose.position.y = location.pose.position.y;
    g_screenPose.position.z = location.pose.position.z - cosf(yaw) * distance;
    g_screenPlaced = true;
}

bool VR::Init()
{
    if (g_runtime != nullptr)
        return true;

    PortVr::OpenXRConfig config;
    config.application_name = "Unleashed Recompiled";
    config.engine_name = "UnleashedRecomp";
    config.reference_space = XR_REFERENCE_SPACE_TYPE_LOCAL;
    config.required_extensions = { XrD3D12::EXTENSION_NAME };
    config.optional_extensions = { "XR_KHR_win32_convert_performance_counter_time" };

    g_runtime = std::make_unique<PortVr::OpenXRRuntime>(LogOpenXR);

    if (!g_runtime->Initialize(config) || !XrD3D12::QueryGraphicsRequirements(*g_runtime))
    {
        LOGN_WARNING("[VR] OpenXR is unavailable; the game will run without VR.");
        g_runtime.reset();
        return false;
    }

    g_screenHeight = std::clamp(Config::VRScreenResolution.Value, 360, 4320);
    g_screenWidth = (g_screenHeight * 16 + 8) / 9;
    g_screenWidth = (g_screenWidth + 1) & ~1u;

    return true;
}

bool VR::IsInitialized()
{
    return g_runtime != nullptr;
}

std::string VR::GetAdapterName()
{
    return XrD3D12::GetAdapterName();
}

bool VR::CreateSession(plume::RenderDevice* device, plume::RenderCommandQueue* queue)
{
    if (g_runtime == nullptr)
        return false;

    if (!XrD3D12::IsDeviceOnRequiredAdapter(device))
    {
        LOGN_ERROR("[VR] The D3D12 device is not on the adapter the OpenXR runtime requires.");
        return false;
    }

    if (!g_runtime->CreateSession(XrD3D12::GetGraphicsBinding(device, queue)))
        return false;

    if (!XrD3D12::SelectSwapchainFormat(g_runtime->SwapchainFormats(), g_swapchainFormat, g_renderFormat, g_swapchainSrgb))
        return false;

    if (!CreateSwapchain(g_screenSwapchain, g_screenWidth, g_screenHeight, "screen"))
        return false;

    if (!XrInput::Create(*g_runtime))
        LOGN_WARNING("[VR] Motion controllers are unavailable; use a gamepad or the keyboard.");

    LOGFN("[VR] Session created on '{}' ({}). Screen {}x{}.",
        g_runtime->RuntimeInfo().system_name, g_runtime->RuntimeInfo().runtime_name, g_screenWidth, g_screenHeight);

    return true;
}

bool VR::IsActive()
{
    return g_runtime != nullptr && g_runtime->HasSession();
}

void VR::Shutdown()
{
    std::lock_guard lock(g_mutex);

    if (g_runtime == nullptr)
        return;

    if (g_runtime->HasSession())
    {
        XrInput::Destroy();
        ReleaseSwapchain(g_screenSwapchain);
        DestroySwapchain(g_screenSwapchain);
    }

    g_frameBegun = false;
    g_inputFocused = false;
    g_runtime->Shutdown();
    g_runtime.reset();
}

void VR::BeginFrame()
{
    std::lock_guard lock(g_mutex);

    if (g_runtime == nullptr || !g_runtime->HasSession() || g_frameBegun)
        return;

    PortVr::OpenXREventStatus eventStatus = g_runtime->PollEvents();
    if (eventStatus != PortVr::OpenXREventStatus::Continue || !g_runtime->IsSessionRunning())
    {
        g_inputFocused = false;
        XrInput::Idle();
        return;
    }

    PortVr::OpenXRFrame frame;
    if (g_runtime->WaitFrame(frame) != PortVr::OpenXRFrameStatus::Ready)
        return;

    if (!g_runtime->BeginFrame(frame))
        return;

    g_runtime->LocateViews(frame);

    g_frame = frame;
    g_frameBegun = true;

    // The runtime recentred (or otherwise moved) the application space: put the
    // screen back in front of the head once the change is in effect.
    if (g_runtime->ConsumeAppSpaceChangesThrough(g_frame.predicted_display_time))
        g_screenPlaced = false;

    if (g_frame.should_render && !g_screenPlaced)
        PlaceScreen();

    // Input only belongs to the game while the headset has focus (not in the dashboard).
    g_inputFocused = g_runtime->IsSessionFocused();
    if (g_inputFocused)
        XrInput::Sync();
    else
        XrInput::Idle();
}

plume::RenderTexture* VR::AcquireScreenImage()
{
    std::lock_guard lock(g_mutex);

    if (!g_frameBegun || !g_frame.should_render)
        return nullptr;

    if (!AcquireSwapchain(g_screenSwapchain))
        return nullptr;

    return g_screenSwapchain.images[g_screenSwapchain.acquiredIndex].get();
}

bool VR::EndFrame()
{
    std::lock_guard lock(g_mutex);

    if (!g_frameBegun)
        return false;

    g_frameBegun = false;

    // The render thread's commands into the image were submitted to the queue
    // the session was created with, so the runtime orders its reads after them.
    bool submitted = ReleaseSwapchain(g_screenSwapchain);

    if (!submitted || !g_frame.should_render)
    {
        g_runtime->EndFrameWithoutLayers(g_frame);
        return true;
    }

    XrCompositionLayerQuad quad{ XR_TYPE_COMPOSITION_LAYER_QUAD };
    quad.layerFlags = 0;
    quad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
    quad.subImage.swapchain = g_screenSwapchain.handle;
    quad.subImage.imageRect = { { 0, 0 }, { int32_t(g_screenSwapchain.width), int32_t(g_screenSwapchain.height) } };
    quad.subImage.imageArrayIndex = 0;

    if (g_screenPlaced)
    {
        quad.space = g_runtime->AppSpace();
        quad.pose = g_screenPose;
    }
    else
    {
        // No head pose yet: keep the screen in front of the head.
        quad.space = g_runtime->ViewSpace();
        quad.pose.orientation = { 0.0f, 0.0f, 0.0f, 1.0f };
        quad.pose.position = { 0.0f, 0.0f, -std::max(0.25f, Config::VRScreenDistance.Value) };
    }

    quad.size.width = std::max(0.25f, Config::VRScreenWidth.Value);
    quad.size.height = quad.size.width * float(g_screenSwapchain.height) / float(g_screenSwapchain.width);

    const XrCompositionLayerBaseHeader* layers[] = { reinterpret_cast<const XrCompositionLayerBaseHeader*>(&quad) };
    g_runtime->EndFrame(g_frame, layers, uint32_t(std::size(layers)));

    return true;
}

void VR::Recenter()
{
    std::lock_guard lock(g_mutex);
    g_screenPlaced = false;
}

plume::RenderFormat VR::GetScreenFormat()
{
    return g_renderFormat;
}

uint32_t VR::GetScreenWidth()
{
    return g_screenWidth;
}

uint32_t VR::GetScreenHeight()
{
    return g_screenHeight;
}

bool VR::IsInputFocused()
{
    return g_inputFocused;
}

bool VR::GetGamepad(XAMINPUT_GAMEPAD& pad)
{
    return g_inputFocused && XrInput::GetGamepad(pad);
}

void VR::SetRumble(uint16_t leftMotorSpeed, uint16_t rightMotorSpeed)
{
    XrInput::SetRumble(leftMotorSpeed, rightMotorSpeed);
}
