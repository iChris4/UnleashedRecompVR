#include "xr.h"
#include "xr_d3d12.h"
#include "xr_input.h"

#include <vr/openxr_runtime.h>
#include <os/logger.h>
#include <user/config.h>

#include <atomic>
#include <chrono>

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
static plume::RenderFormat g_imageFormat = plume::RenderFormat::B8G8R8A8_UNORM;
static bool g_swapchainSrgb;

static uint32_t g_renderWidth = 1920;
static uint32_t g_renderHeight = 1080;
static XrSwapchainState g_screenSwapchain;
static XrSwapchainState g_eyeSwapchains[2];

static PortVr::OpenXRFrame g_frame;
static bool g_frameBegun;
static std::atomic<bool> g_inputFocused;

// The head pose the game camera stands at (its position and heading), set on
// the first tracked frame and on every recenter.
static bool g_baseValid;
static XrVector3f g_basePosition;
static float g_baseYaw;

static std::mutex g_viewsMutex;
static VRViews g_views;

// The flat screen is placed once in the application space, in front of the
// head with the head's heading, and stays there while the player looks around.
static bool g_screenPlaced;
static XrPosef g_screenPose{ { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } };

static VRFrameMode g_lastFrameMode = VRFrameMode::None;

// Both thumbsticks held in for this long recentre the view, once per hold.
static constexpr auto RECENTER_HOLD_TIME = std::chrono::seconds(1);
static std::mutex g_recenterMutex;
static bool g_recenterHeld;
static bool g_recenterDone;
static std::chrono::steady_clock::time_point g_recenterHeldSince;
static std::atomic<bool> g_recenterRequested;

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

    result = XrD3D12::EnumerateSwapchainImages(swapchain.handle, width, height, g_imageFormat, swapchain.images);
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

static plume::RenderTexture* AcquireSwapchain(XrSwapchainState& swapchain)
{
    if (swapchain.acquired)
        return swapchain.images[swapchain.acquiredIndex].get();

    XrSwapchainImageAcquireInfo acquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
    XrResult result = xrAcquireSwapchainImage(swapchain.handle, &acquireInfo, &swapchain.acquiredIndex);
    g_runtime->ObserveResult(result);
    if (XR_FAILED(result))
    {
        LOGFN_ERROR("[VR] xrAcquireSwapchainImage failed ({}).", int32_t(result));
        return nullptr;
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
        return nullptr;
    }

    return swapchain.images[swapchain.acquiredIndex].get();
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

// The heading of an orientation: the angle about +Y that turns -Z towards its
// forward axis flattened onto the floor.
static float GetHeading(const XrQuaternionf& q)
{
    float forwardX = -2.0f * (q.x * q.z + q.w * q.y);
    float forwardZ = -(1.0f - 2.0f * (q.x * q.x + q.y * q.y));
    if ((forwardX * forwardX + forwardZ * forwardZ) <= 1.0e-6f)
        return 0.0f;

    return atan2f(-forwardX, -forwardZ);
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

    float yaw = GetHeading(location.pose.orientation);

    float distance = std::max(0.25f, Config::VRScreenDistance.Value);

    g_screenPose.orientation = { 0.0f, sinf(0.5f * yaw), 0.0f, cosf(0.5f * yaw) };
    g_screenPose.position.x = location.pose.position.x - sinf(yaw) * distance;
    g_screenPose.position.y = location.pose.position.y;
    g_screenPose.position.z = location.pose.position.z - cosf(yaw) * distance;
    g_screenPlaced = true;
}

static void PublishViews()
{
    VRViews views;

    if (g_frame.should_render && g_frame.views_valid)
    {
        const XrView* xrViews = g_frame.views.data();

        constexpr XrViewStateFlags RequiredFlags = XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_VALID_BIT;
        if (!g_baseValid && (g_frame.view_state_flags & RequiredFlags) == RequiredFlags)
        {
            g_basePosition.x = 0.5f * (xrViews[0].pose.position.x + xrViews[1].pose.position.x);
            g_basePosition.y = 0.5f * (xrViews[0].pose.position.y + xrViews[1].pose.position.y);
            g_basePosition.z = 0.5f * (xrViews[0].pose.position.z + xrViews[1].pose.position.z);
            g_baseYaw = GetHeading(xrViews[0].pose.orientation);
            g_baseValid = true;
        }

        if (g_baseValid)
        {
            views.valid = true;
            views.basePosition[0] = g_basePosition.x;
            views.basePosition[1] = g_basePosition.y;
            views.basePosition[2] = g_basePosition.z;

            // Undo the base heading: a rotation of -yaw about +Y.
            float sinHalf = sinf(-0.5f * g_baseYaw);
            float cosHalf = cosf(-0.5f * g_baseYaw);
            float sinYaw = sinf(-g_baseYaw);
            float cosYaw = cosf(-g_baseYaw);

            for (uint32_t i = 0; i < 2; i++)
            {
                const XrView& xrView = xrViews[i];
                const XrQuaternionf& q = xrView.pose.orientation;
                VRViews::Eye& eye = views.eyes[i];

                // (0, sinHalf, 0, cosHalf) * q
                eye.orientation[0] = cosHalf * q.x + sinHalf * q.z;
                eye.orientation[1] = cosHalf * q.y + sinHalf * q.w;
                eye.orientation[2] = cosHalf * q.z - sinHalf * q.x;
                eye.orientation[3] = cosHalf * q.w - sinHalf * q.y;

                float x = xrView.pose.position.x - g_basePosition.x;
                float y = xrView.pose.position.y - g_basePosition.y;
                float z = xrView.pose.position.z - g_basePosition.z;
                eye.position[0] = x * cosYaw + z * sinYaw;
                eye.position[1] = y;
                eye.position[2] = -x * sinYaw + z * cosYaw;

                eye.spaceOrientation[0] = q.x;
                eye.spaceOrientation[1] = q.y;
                eye.spaceOrientation[2] = q.z;
                eye.spaceOrientation[3] = q.w;
                eye.spacePosition[0] = xrView.pose.position.x;
                eye.spacePosition[1] = xrView.pose.position.y;
                eye.spacePosition[2] = xrView.pose.position.z;
                eye.angleLeft = xrView.fov.angleLeft;
                eye.angleRight = xrView.fov.angleRight;
                eye.angleUp = xrView.fov.angleUp;
                eye.angleDown = xrView.fov.angleDown;
            }
        }
    }

    std::lock_guard lock(g_viewsMutex);
    g_views = views;
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
    config.resolution_scale = std::clamp(Config::VRRenderScale.Value, 0.25f, 2.0f);

    g_runtime = std::make_unique<PortVr::OpenXRRuntime>(LogOpenXR);

    if (!g_runtime->Initialize(config) || !XrD3D12::QueryGraphicsRequirements(*g_runtime))
    {
        LOGN_WARNING("[VR] OpenXR is unavailable; the game will run without VR.");
        g_runtime.reset();
        return false;
    }

    // One resolution for both eyes: they share the game's render targets.
    const auto& view = g_runtime->ViewConfiguration()[0];
    g_renderWidth = std::max(64u, view.render_width & ~1u);
    g_renderHeight = std::max(64u, view.render_height & ~1u);

    LOGFN("[VR] Recommended eye resolution {}x{}; rendering at {}x{}.",
        view.properties.recommendedImageRectWidth, view.properties.recommendedImageRectHeight, g_renderWidth, g_renderHeight);

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

    if (!XrD3D12::SelectSwapchainFormat(g_runtime->SwapchainFormats(), g_swapchainFormat, g_imageFormat, g_swapchainSrgb))
        return false;

    if (!CreateSwapchain(g_screenSwapchain, g_renderWidth, g_renderHeight, "screen") ||
        !CreateSwapchain(g_eyeSwapchains[0], g_renderWidth, g_renderHeight, "left eye") ||
        !CreateSwapchain(g_eyeSwapchains[1], g_renderWidth, g_renderHeight, "right eye"))
    {
        return false;
    }

    if (!XrInput::Create(*g_runtime))
        LOGN_WARNING("[VR] Motion controllers are unavailable; use a gamepad or the keyboard.");

    LOGFN("[VR] Session created on '{}' ({}).", g_runtime->RuntimeInfo().system_name, g_runtime->RuntimeInfo().runtime_name);

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

        for (XrSwapchainState* swapchain : { &g_screenSwapchain, &g_eyeSwapchains[0], &g_eyeSwapchains[1] })
        {
            ReleaseSwapchain(*swapchain);
            DestroySwapchain(*swapchain);
        }
    }

    g_frameBegun = false;
    g_inputFocused = false;
    g_runtime->Shutdown();
    g_runtime.reset();

    std::lock_guard viewsLock(g_viewsMutex);
    g_views = {};
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
    // game camera and the screen back in front of the head once that is in effect.
    if (g_runtime->ConsumeAppSpaceChangesThrough(g_frame.predicted_display_time))
    {
        g_baseValid = false;
        g_screenPlaced = false;
    }

    // Both thumbsticks held in (see UpdateRecenterButtons).
    if (g_recenterRequested.exchange(false))
    {
        LOGN("[VR] Recentred.");
        g_baseValid = false;
        g_screenPlaced = false;
    }

    if (g_frame.should_render && !g_screenPlaced)
        PlaceScreen();

    PublishViews();

    // Input only belongs to the game while the headset has focus (not in the dashboard).
    g_inputFocused = g_runtime->IsSessionFocused();
    if (g_inputFocused)
        XrInput::Sync();
    else
        XrInput::Idle();
}

void VR::AcquireImages(VRImages& images)
{
    std::lock_guard lock(g_mutex);

    images = {};

    if (!g_frameBegun || !g_frame.should_render)
        return;

    // Whether this frame shows the world or the screen is only known once the
    // render thread has seen its draws, so all three images are acquired.
    images.screen = AcquireSwapchain(g_screenSwapchain);
    images.eyes[0] = AcquireSwapchain(g_eyeSwapchains[0]);
    images.eyes[1] = AcquireSwapchain(g_eyeSwapchains[1]);
}

bool VR::EndFrame(const VRFrameResult& result)
{
    std::lock_guard lock(g_mutex);

    if (!g_frameBegun)
        return false;

    g_frameBegun = false;

    // The render thread's copies into the images were submitted to the queue the
    // session was created with, so the runtime orders its reads after them.
    bool screenReleased = ReleaseSwapchain(g_screenSwapchain);
    bool leftReleased = ReleaseSwapchain(g_eyeSwapchains[0]);
    bool rightReleased = ReleaseSwapchain(g_eyeSwapchains[1]);

    VRFrameMode mode = result.mode;
    if (mode == VRFrameMode::Immersive && !(leftReleased && rightReleased && result.views.valid))
        mode = VRFrameMode::None;
    else if (mode == VRFrameMode::Flat && !screenReleased)
        mode = VRFrameMode::None;

    if (mode != g_lastFrameMode)
    {
        static const char* const ModeNames[] = { "none", "flat screen", "immersive" };
        LOGFN("[VR] Headset view: {}.", ModeNames[size_t(mode)]);
        g_lastFrameMode = mode;
    }

    if (mode == VRFrameMode::None || !g_frame.should_render)
    {
        g_runtime->EndFrameWithoutLayers(g_frame);
        return true;
    }

    if (mode == VRFrameMode::Immersive)
    {
        // Submit the poses the eyes were rendered with, never fresher ones: the
        // compositor reprojects from these to the display time.
        XrCompositionLayerProjectionView projectionViews[2]{};

        for (uint32_t i = 0; i < 2; i++)
        {
            const VRViews::Eye& eye = result.views.eyes[i];
            XrCompositionLayerProjectionView& view = projectionViews[i];
            view = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
            view.pose.orientation = { eye.spaceOrientation[0], eye.spaceOrientation[1], eye.spaceOrientation[2], eye.spaceOrientation[3] };
            view.pose.position = { eye.spacePosition[0], eye.spacePosition[1], eye.spacePosition[2] };
            view.fov = { eye.angleLeft, eye.angleRight, eye.angleUp, eye.angleDown };
            view.subImage.swapchain = g_eyeSwapchains[i].handle;
            view.subImage.imageRect = { { 0, 0 }, { int32_t(g_eyeSwapchains[i].width), int32_t(g_eyeSwapchains[i].height) } };
            view.subImage.imageArrayIndex = 0;
        }

        XrCompositionLayerProjection projection{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
        projection.space = g_runtime->AppSpace();
        projection.viewCount = 2;
        projection.views = projectionViews;

        const XrCompositionLayerBaseHeader* layers[] = { reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projection) };
        g_runtime->EndFrame(g_frame, layers, uint32_t(std::size(layers)));
        return true;
    }

    XrCompositionLayerQuad quad{ XR_TYPE_COMPOSITION_LAYER_QUAD };
    quad.layerFlags = 0;
    quad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
    quad.subImage.swapchain = g_screenSwapchain.handle;
    quad.subImage.imageRect = { { 0, 0 }, { int32_t(g_screenSwapchain.width), int32_t(g_screenSwapchain.height) } };
    quad.subImage.imageArrayIndex = 0;

    // A cutscene is drawn for the eyes' shape with more above and below than the
    // game frames (vertical field of view widened): its 16:9 middle is the shot.
    if (result.cutscene)
    {
        int32_t width = int32_t(g_screenSwapchain.width);
        int32_t height = int32_t(g_screenSwapchain.height);
        int32_t shotWidth = std::min(width, height * 16 / 9);
        int32_t shotHeight = std::min(height, width * 9 / 16);
        quad.subImage.imageRect = { { (width - shotWidth) / 2, (height - shotHeight) / 2 }, { shotWidth, shotHeight } };
    }

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

    // VRScreenWidth is the width of a 16:9 screen; other shapes keep its height.
    quad.size.height = std::max(0.25f, Config::VRScreenWidth.Value) * 9.0f / 16.0f;
    quad.size.width = quad.size.height * float(quad.subImage.imageRect.extent.width) / float(quad.subImage.imageRect.extent.height);

    const XrCompositionLayerBaseHeader* layers[] = { reinterpret_cast<const XrCompositionLayerBaseHeader*>(&quad) };
    g_runtime->EndFrame(g_frame, layers, uint32_t(std::size(layers)));

    return true;
}

void VR::GetViews(VRViews& views)
{
    std::lock_guard lock(g_viewsMutex);
    views = g_views;
}

void VR::Recenter()
{
    std::lock_guard lock(g_mutex);
    g_baseValid = false;
    g_screenPlaced = false;
}

void VR::UpdateRecenterButtons(bool bothThumbsticksHeld)
{
    if (!IsActive())
        return;

    std::lock_guard lock(g_recenterMutex);
    auto now = std::chrono::steady_clock::now();

    if (!bothThumbsticksHeld)
    {
        g_recenterHeld = false;
        g_recenterDone = false;
    }
    else if (!g_recenterHeld)
    {
        g_recenterHeld = true;
        g_recenterHeldSince = now;
    }
    else if (!g_recenterDone && now - g_recenterHeldSince >= RECENTER_HOLD_TIME)
    {
        g_recenterDone = true;
        g_recenterRequested = true;
    }
}

plume::RenderFormat VR::GetImageFormat()
{
    return g_imageFormat;
}

uint32_t VR::GetRenderWidth()
{
    return g_renderWidth;
}

uint32_t VR::GetRenderHeight()
{
    return g_renderHeight;
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
