#include "xr.h"
#include "xr_d3d12.h"
#include "xr_vulkan.h"
#include "xr_input.h"

#include <vr/openxr_runtime.h>
#include <os/logger.h>
#include <user/config.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

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

// The graphics API of the session (XR_KHR_vulkan_enable or XR_KHR_D3D12_enable),
// and the device the swapchain images are wrapped for.
static bool g_vulkanBackend;
static plume::RenderDevice* g_renderDevice;

// With Vulkan the runtime may use the session's queue in frame and swapchain
// calls, which must not overlap the render thread's submissions.
static std::unique_lock<std::mutex> LockGraphicsQueue()
{
    return g_vulkanBackend ? XrVulkan::LockQueue() : std::unique_lock<std::mutex>();
}

static int64_t g_swapchainFormat;
static plume::RenderFormat g_imageFormat = plume::RenderFormat::B8G8R8A8_UNORM;
static bool g_swapchainSrgb;

static uint32_t g_renderWidth = 1920;
static uint32_t g_renderHeight = 1080;

// A render scale asked for and not applied yet (negative when none).
static std::atomic<float> g_pendingRenderScale = -1.0f;

// The keep-alive: while the game submits nothing for STALL_TIME (a load, a
// long hitch, a minimised window), a thread of its own keeps the headset fed
// with the layer the game submitted last, which the compositor keeps in place
// in the world. A frame the game has begun meanwhile is ended with that layer
// too; the game begins a new one when it presents. The swapchains' images are
// those the game released last.
static constexpr auto STALL_TIME = std::chrono::milliseconds(100);

struct RetainedLayer
{
    VRFrameMode mode = VRFrameMode::None;
    XrCompositionLayerProjectionView views[2]{};
    XrCompositionLayerQuad quad{ XR_TYPE_COMPOSITION_LAYER_QUAD };
};

static RetainedLayer g_retainedLayer;
static std::chrono::steady_clock::time_point g_lastGameFrameTime;
static bool g_keepingAlive;
static std::chrono::steady_clock::time_point g_keepAliveSince;
static std::thread g_keepAliveThread;
static std::atomic<bool> g_keepAliveRunning;

static void StartKeepAlive();
static void StopKeepAlive();

// The pacing thread ([VR] VRPacingThread, replacing the keep-alive): a thread of
// its own runs the headset's frame loop (xrWaitFrame, xrBeginFrame, xrEndFrame)
// at the headset's rate. The game hands it finished frames (the layer it
// retains, with the images it released), and each cycle ends with the newest
// one: a fresh frame when the game delivered in time, else the previous one
// again, which the compositor reprojects for the head's new pose. A game frame
// that overruns a headset cycle then costs no more than its own time, where
// waiting in xrWaitFrame for the next cycle costs the rest of a whole one.
//
// The game still draws at most one frame per headset cycle: it begins a frame
// only for a cycle it has not drawn for yet, and waits for the next one
// otherwise. Everything below is guarded by g_mutex; the thread only leaves it
// for xrWaitFrame and while it waits for the game's frame.
static bool g_pacingEnabled;
static std::thread g_pacingThread;
static std::atomic<bool> g_pacingRunning;
static std::condition_variable g_pacingCondition;

// The headset cycles begun so far, the last one's frame state and when its
// xrWaitFrame returned, and whether the session is running.
static uint64_t g_pacingCycle;
static XrFrameState g_pacingFrameState{ XR_TYPE_FRAME_STATE };
static std::chrono::steady_clock::time_point g_pacingCycleTime;
static bool g_pacingSessionRunning;

// How long xrEndFrame takes (smoothed), which the wait for the game's frame
// leaves room for.
static double g_pacingEndFrameMs = 0.5;

// The frames the game handed over, the cycle its last frame was begun for, and
// how long its frames take from their beginning to their handover (smoothed).
static uint64_t g_publishSerial;
static uint64_t g_gameCycle;
static std::chrono::steady_clock::time_point g_gameFrameBeginTime;
static double g_gameFrameMs = 8.0;

static void StartPacing();
static void StopPacing();

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
    {
        auto queueLock = LockGraphicsQueue();
        xrDestroySwapchain(swapchain.handle);
    }

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

    XrResult result;
    {
        auto queueLock = LockGraphicsQueue();
        result = xrCreateSwapchain(g_runtime->Session(), &createInfo, &swapchain.handle);
    }

    g_runtime->ObserveResult(result);
    if (XR_FAILED(result))
    {
        LOGFN_ERROR("[VR] xrCreateSwapchain failed for the {} swapchain ({}).", name, int32_t(result));
        swapchain.handle = XR_NULL_HANDLE;
        return false;
    }

    swapchain.width = width;
    swapchain.height = height;

    result = g_vulkanBackend ?
        XrVulkan::EnumerateSwapchainImages(swapchain.handle, width, height, g_imageFormat, g_renderDevice, swapchain.images) :
        XrD3D12::EnumerateSwapchainImages(swapchain.handle, width, height, g_imageFormat, swapchain.images);
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

    auto queueLock = LockGraphicsQueue();

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

    auto queueLock = LockGraphicsQueue();

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

// The per-eye render resolution for a scale of the runtime's recommendation.
static void GetRenderSize(float scale, uint32_t& width, uint32_t& height)
{
    PortVr::OpenXREyeSize size = PortVr::OpenXRScaledEyeSize(g_runtime->ViewConfiguration()[0].properties, std::clamp(scale, 0.25f, 2.0f));
    width = std::max(64u, size.width & ~1u);
    height = std::max(64u, size.height & ~1u);
}

bool VR::Init(bool vulkan)
{
    if (g_runtime != nullptr)
        return true;

    g_vulkanBackend = vulkan;

    PortVr::OpenXRConfig config;
    config.application_name = "Unleashed Recompiled";
    config.engine_name = "UnleashedRecomp";
    config.reference_space = XR_REFERENCE_SPACE_TYPE_LOCAL;
    config.required_extensions = { vulkan ? XrVulkan::EXTENSION_NAME : XrD3D12::EXTENSION_NAME };
    config.optional_extensions = { "XR_KHR_win32_convert_performance_counter_time" };
    config.resolution_scale = std::clamp(Config::VRRenderScale.Value, 0.25f, 2.0f);

    g_runtime = std::make_unique<PortVr::OpenXRRuntime>(LogOpenXR);

    bool prepared = g_runtime->Initialize(config) &&
        (vulkan ? XrVulkan::Prepare(*g_runtime) : XrD3D12::QueryGraphicsRequirements(*g_runtime));

    if (!prepared)
    {
        LOGN_WARNING("[VR] OpenXR is unavailable; the game will run without VR.");
        g_runtime.reset();
        return false;
    }

    // One resolution for both eyes: they share the game's render targets.
    const auto& view = g_runtime->ViewConfiguration()[0];
    GetRenderSize(config.resolution_scale, g_renderWidth, g_renderHeight);

    LOGFN("[VR] Recommended eye resolution {}x{}; rendering at {}x{}.",
        view.properties.recommendedImageRectWidth, view.properties.recommendedImageRectHeight, g_renderWidth, g_renderHeight);

    return true;
}

bool VR::IsInitialized()
{
    return g_runtime != nullptr;
}

std::string VR::GetAdapterName(plume::RenderInterface* renderInterface)
{
    if (g_runtime == nullptr)
        return {};

    return g_vulkanBackend ? XrVulkan::GetDeviceName(*g_runtime, renderInterface) : XrD3D12::GetAdapterName();
}

bool VR::CreateSession(plume::RenderDevice* device, plume::RenderCommandQueue* queue)
{
    if (g_runtime == nullptr)
        return false;

    bool onRequiredDevice = g_vulkanBackend ? XrVulkan::IsDeviceOnRequiredGpu(*g_runtime, device) : XrD3D12::IsDeviceOnRequiredAdapter(device);
    if (!onRequiredDevice)
    {
        LOGFN_ERROR("[VR] The {} device is not on the GPU the OpenXR runtime requires.", g_vulkanBackend ? "Vulkan" : "D3D12");
        return false;
    }

    g_renderDevice = device;

    const void* binding = g_vulkanBackend ? XrVulkan::GetGraphicsBinding(device, queue) : XrD3D12::GetGraphicsBinding(device, queue);
    bool sessionCreated;
    {
        auto queueLock = LockGraphicsQueue();
        sessionCreated = g_runtime->CreateSession(binding);
    }

    if (!sessionCreated)
        return false;

    bool formatSelected = g_vulkanBackend ?
        XrVulkan::SelectSwapchainFormat(g_runtime->SwapchainFormats(), g_swapchainFormat, g_imageFormat, g_swapchainSrgb) :
        XrD3D12::SelectSwapchainFormat(g_runtime->SwapchainFormats(), g_swapchainFormat, g_imageFormat, g_swapchainSrgb);

    if (!formatSelected)
        return false;

    if (!CreateSwapchain(g_screenSwapchain, g_renderWidth, g_renderHeight, "screen") ||
        !CreateSwapchain(g_eyeSwapchains[0], g_renderWidth, g_renderHeight, "left eye") ||
        !CreateSwapchain(g_eyeSwapchains[1], g_renderWidth, g_renderHeight, "right eye"))
    {
        return false;
    }

    if (!XrInput::Create(*g_runtime))
        LOGN_WARNING("[VR] Motion controllers are unavailable; use a gamepad or the keyboard.");

    LOGFN("[VR] Session created on '{}' ({}, {}).", g_runtime->RuntimeInfo().system_name, g_runtime->RuntimeInfo().runtime_name,
        g_vulkanBackend ? "Vulkan" : "D3D12");

    g_pacingEnabled = Config::VRPacingThread;

    if (g_pacingEnabled)
    {
        LOGN("[VR] The headset is paced by a thread of its own.");
        StartPacing();
    }
    else
    {
        StartKeepAlive();
    }

    return true;
}

bool VR::IsActive()
{
    return g_runtime != nullptr && g_runtime->HasSession();
}

void VR::Shutdown()
{
    StopKeepAlive();
    StopPacing();

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
    g_retainedLayer = {};

    {
        auto queueLock = LockGraphicsQueue();
        g_runtime->Shutdown();
    }

    g_runtime.reset();

    std::lock_guard viewsLock(g_viewsMutex);
    g_views = {};
}

// Begins the game's frame with the pacing thread: for a headset cycle the game
// has not drawn for yet, waiting for the next one if need be. g_mutex is held.
// False when the headset wants no frame.
static bool BeginPacedFrame(std::unique_lock<std::mutex>& lock, PortVr::OpenXRFrame& frame)
{
    if (!g_pacingSessionRunning)
        return false;

    bool cycleReady = g_pacingCondition.wait_for(lock, std::chrono::milliseconds(100), []()
        {
            return g_pacingCycle != g_gameCycle || !g_pacingSessionRunning || !g_pacingRunning;
        });

    if (!cycleReady || !g_pacingSessionRunning || !g_pacingRunning || g_runtime == nullptr || !g_runtime->HasSession())
        return false;

    g_gameCycle = g_pacingCycle;

    auto now = std::chrono::steady_clock::now();
    g_gameFrameBeginTime = now;

    // The cycle the frame will be shown in, from how long the game's frames
    // take: this one while it can be handed over before the cycle ends, else a
    // later one, whose display time the eyes are then located for.
    double periodMs = double(g_pacingFrameState.predictedDisplayPeriod) / 1.0e6;
    int64_t cyclesAhead = 0;

    if (periodMs > 1.0)
    {
        double sinceCycleMs = std::chrono::duration<double, std::milli>(now - g_pacingCycleTime).count();
        double handoverMs = sinceCycleMs + g_gameFrameMs;
        double deadlineMs = periodMs - g_pacingEndFrameMs - 1.0;

        if (handoverMs > deadlineMs)
            cyclesAhead = std::clamp(int64_t(ceil((handoverMs - deadlineMs) / periodMs)), int64_t(0), int64_t(3));
    }

    frame = {};
    frame.predicted_display_period = g_pacingFrameState.predictedDisplayPeriod;
    frame.should_render = g_pacingFrameState.shouldRender == XR_TRUE;
    frame.predicted_display_time = g_pacingFrameState.predictedDisplayTime + cyclesAhead * g_pacingFrameState.predictedDisplayPeriod;

    g_runtime->LocateViewsAt(frame.predicted_display_time, frame);
    return true;
}

void VR::BeginFrame()
{
    std::unique_lock lock(g_mutex);

    if (g_runtime == nullptr || !g_runtime->HasSession() || g_frameBegun)
        return;

    PortVr::OpenXRFrame frame;

    if (g_pacingEnabled)
    {
        if (!BeginPacedFrame(lock, frame))
            return;
    }
    else
    {
        PortVr::OpenXREventStatus eventStatus = g_runtime->PollEvents();
        if (eventStatus != PortVr::OpenXREventStatus::Continue || !g_runtime->IsSessionRunning())
        {
            g_inputFocused = false;
            XrInput::Idle();
            return;
        }

        if (g_runtime->WaitFrame(frame) != PortVr::OpenXRFrameStatus::Ready)
            return;

        bool begun;
        {
            auto queueLock = LockGraphicsQueue();
            begun = g_runtime->BeginFrame(frame);
        }

        if (!begun)
            return;

        g_runtime->LocateViews(frame);
    }

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

// Ends a frame with a retained layer (or none). g_mutex is held.
static void EndFrameWithLayer(const PortVr::OpenXRFrame& frame, const RetainedLayer& layer)
{
    auto queueLock = LockGraphicsQueue();

    if (layer.mode == VRFrameMode::None || !frame.should_render)
    {
        g_runtime->EndFrameWithoutLayers(frame);
        return;
    }

    if (layer.mode == VRFrameMode::Immersive)
    {
        XrCompositionLayerProjection projection{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
        projection.space = g_runtime->AppSpace();
        projection.viewCount = 2;
        projection.views = layer.views;

        const XrCompositionLayerBaseHeader* layers[] = { reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projection) };
        g_runtime->EndFrame(frame, layers, uint32_t(std::size(layers)));
        return;
    }

    const XrCompositionLayerBaseHeader* layers[] = { reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layer.quad) };
    g_runtime->EndFrame(frame, layers, uint32_t(std::size(layers)));
}

static bool AnySwapchainAcquired()
{
    return g_screenSwapchain.acquired || g_eyeSwapchains[0].acquired || g_eyeSwapchains[1].acquired;
}

// One step of the keep-alive thread (see STALL_TIME).
static void KeepAlive()
{
    std::unique_lock lock(g_mutex);

    auto now = std::chrono::steady_clock::now();
    bool stalled = g_runtime != nullptr && g_runtime->HasSession() && g_retainedLayer.mode != VRFrameMode::None &&
        !AnySwapchainAcquired() && now - g_lastGameFrameTime >= STALL_TIME;

    if (!stalled)
    {
        if (g_keepingAlive)
        {
            g_keepingAlive = false;
            LOGFN("[VR] The game is presenting again after {} ms.",
                std::chrono::duration_cast<std::chrono::milliseconds>(now - g_keepAliveSince).count());
        }

        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return;
    }

    if (!g_keepingAlive)
    {
        g_keepingAlive = true;
        g_keepAliveSince = now;
        LOGN("[VR] The game stalled; the headset keeps its last picture.");
    }

    PortVr::OpenXREventStatus eventStatus = g_runtime->PollEvents();
    if (eventStatus != PortVr::OpenXREventStatus::Continue || !g_runtime->IsSessionRunning())
    {
        g_inputFocused = false;
        XrInput::Idle();
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return;
    }

    // A frame the game began before stalling is ended with the old picture.
    if (g_frameBegun)
    {
        g_frameBegun = false;
        EndFrameWithLayer(g_frame, g_retainedLayer);
        return;
    }

    PortVr::OpenXRFrame frame;
    if (g_runtime->WaitFrame(frame) != PortVr::OpenXRFrameStatus::Ready)
        return;

    bool begun;
    {
        auto queueLock = LockGraphicsQueue();
        begun = g_runtime->BeginFrame(frame);
    }

    if (!begun)
        return;

    EndFrameWithLayer(frame, g_retainedLayer);
}

// Ends a headset cycle of the pacing thread with a layer (or none). g_mutex is held.
static XrResult EndPacedFrame(XrSession session, const XrFrameState& state, const RetainedLayer& layer)
{
    XrCompositionLayerProjection projection{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
    const XrCompositionLayerBaseHeader* layers[1]{};
    uint32_t layerCount = 0;

    if (state.shouldRender == XR_TRUE && layer.mode == VRFrameMode::Immersive)
    {
        projection.space = g_runtime->AppSpace();
        projection.viewCount = 2;
        projection.views = layer.views;
        layers[layerCount++] = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projection);
    }
    else if (state.shouldRender == XR_TRUE && layer.mode == VRFrameMode::Flat)
    {
        layers[layerCount++] = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layer.quad);
    }

    XrFrameEndInfo endInfo{ XR_TYPE_FRAME_END_INFO };
    endInfo.displayTime = state.predictedDisplayTime;
    endInfo.environmentBlendMode = g_runtime->EnvironmentBlendMode();
    endInfo.layerCount = layerCount;
    endInfo.layers = layerCount != 0 ? layers : nullptr;

    auto queueLock = LockGraphicsQueue();
    return xrEndFrame(session, &endInfo);
}

// What the pacing thread did, logged every few seconds.
struct PacingStatistics
{
    uint32_t cycles;
    uint32_t freshFrames;
    uint32_t repeatedFrames;
    uint32_t emptyFrames;
    double contentWaitMs;
    double endFrameMs;
    double endFrameMaxMs;
    std::chrono::steady_clock::time_point since = std::chrono::steady_clock::now();
};

// One headset cycle of the pacing thread.
static void PaceFrame()
{
    static PacingStatistics s_statistics;
    static uint64_t s_consumedSerial;
    static std::chrono::steady_clock::time_point s_lastCycleTime;
    static bool s_repeating;

    XrSession session = XR_NULL_HANDLE;
    {
        std::unique_lock lock(g_mutex);

        bool running = false;
        if (g_runtime != nullptr && g_runtime->HasSession())
        {
            PortVr::OpenXREventStatus eventStatus = g_runtime->PollEvents();
            running = eventStatus == PortVr::OpenXREventStatus::Continue && g_runtime->IsSessionRunning();
            session = g_runtime->Session();
        }

        if (running != g_pacingSessionRunning)
        {
            g_pacingSessionRunning = running;
            g_pacingCondition.notify_all();
        }

        if (!running)
        {
            g_inputFocused = false;
            XrInput::Idle();
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return;
        }
    }

    // Blocks until the headset wants its next frame. Never under g_mutex or the
    // graphics queue's lock: the game works meanwhile.
    XrFrameWaitInfo waitInfo{ XR_TYPE_FRAME_WAIT_INFO };
    XrFrameState state{ XR_TYPE_FRAME_STATE };
    XrResult result = xrWaitFrame(session, &waitInfo, &state);
    auto cycleTime = std::chrono::steady_clock::now();

    std::unique_lock lock(g_mutex);
    g_runtime->ObserveResult(result);

    if (XR_FAILED(result))
    {
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return;
    }

    {
        XrFrameBeginInfo beginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
        auto queueLock = LockGraphicsQueue();
        result = xrBeginFrame(session, &beginInfo);
    }

    g_runtime->ObserveResult(result);

    if (XR_FAILED(result))
    {
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return;
    }

    // The game may now begin a frame for this cycle.
    double lastCycleMs = std::chrono::duration<double, std::milli>(cycleTime - s_lastCycleTime).count();
    s_lastCycleTime = cycleTime;

    ++g_pacingCycle;
    g_pacingFrameState = state;
    g_pacingCycleTime = cycleTime;
    g_pacingCondition.notify_all();

    // Wait for the game's frame for as long as the cycle can afford: its period
    // less the time xrEndFrame takes and a margin. Not after an overrun, to
    // catch up with the headset.
    double periodMs = double(state.predictedDisplayPeriod) / 1.0e6;
    bool overran = periodMs > 1.0 && lastCycleMs > periodMs * 1.25;

    if (state.shouldRender == XR_TRUE && periodMs > 1.0 && !overran && g_publishSerial == s_consumedSerial)
    {
        auto deadline = cycleTime + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<double, std::milli>(std::max(0.0, periodMs - g_pacingEndFrameMs - 1.0)));

        g_pacingCondition.wait_until(lock, deadline, []()
            {
                return g_publishSerial != s_consumedSerial || !g_pacingRunning;
            });
    }

    auto endStart = std::chrono::steady_clock::now();
    s_statistics.contentWaitMs += std::chrono::duration<double, std::milli>(endStart - cycleTime).count();

    bool fresh = g_publishSerial != s_consumedSerial;
    s_consumedSerial = g_publishSerial;

    // The swapchains may have been replaced or the session ended while waiting.
    if (g_runtime == nullptr || !g_runtime->HasSession())
        return;

    result = EndPacedFrame(session, state, g_retainedLayer);
    g_runtime->ObserveResult(result);

    auto endTime = std::chrono::steady_clock::now();
    double endFrameMs = std::chrono::duration<double, std::milli>(endTime - endStart).count();
    g_pacingEndFrameMs += (std::min(endFrameMs, 6.0) - g_pacingEndFrameMs) * 0.1;

    // Statistics, and the stalls the keep-alive used to report.
    ++s_statistics.cycles;
    s_statistics.endFrameMs += endFrameMs;
    s_statistics.endFrameMaxMs = std::max(s_statistics.endFrameMaxMs, endFrameMs);

    if (state.shouldRender != XR_TRUE || g_retainedLayer.mode == VRFrameMode::None)
        ++s_statistics.emptyFrames;
    else if (fresh)
        ++s_statistics.freshFrames;
    else
        ++s_statistics.repeatedFrames;

    bool stalled = !fresh && g_retainedLayer.mode != VRFrameMode::None && endTime - g_lastGameFrameTime >= STALL_TIME;
    if (stalled && !s_repeating)
    {
        s_repeating = true;
        g_keepAliveSince = endTime;
        LOGN("[VR] The game stalled; the headset keeps its last picture.");
    }
    else if (fresh && s_repeating)
    {
        s_repeating = false;
        LOGFN("[VR] The game is presenting again after {} ms.",
            std::chrono::duration_cast<std::chrono::milliseconds>(endTime - g_keepAliveSince).count());
    }

    if (endTime - s_statistics.since >= std::chrono::seconds(5))
    {
        double cycles = double(std::max(1u, s_statistics.cycles));

        LOGFN("[VR] Last 5 s, pacing thread: {} headset frames: {} new from the game, {} repeated, {} without a picture. "
            "Waited {:.2f} ms per frame for the game; xrEndFrame {:.2f} ms (max {:.2f}). The game's frames take {:.2f} ms.",
            s_statistics.cycles, s_statistics.freshFrames, s_statistics.repeatedFrames, s_statistics.emptyFrames,
            s_statistics.contentWaitMs / cycles, s_statistics.endFrameMs / cycles, s_statistics.endFrameMaxMs, g_gameFrameMs);

        s_statistics = {};
    }
}

static void StartPacing()
{
    g_lastGameFrameTime = std::chrono::steady_clock::now();
    g_pacingRunning = true;
    g_pacingThread = std::thread([]()
        {
            while (g_pacingRunning)
                PaceFrame();
        });
}

static void StopPacing()
{
    {
        std::lock_guard lock(g_mutex);
        g_pacingRunning = false;
        g_pacingSessionRunning = false;
    }

    g_pacingCondition.notify_all();

    if (g_pacingThread.joinable())
        g_pacingThread.join();
}

static void StartKeepAlive()
{
    g_lastGameFrameTime = std::chrono::steady_clock::now();
    g_keepAliveRunning = true;
    g_keepAliveThread = std::thread([]()
        {
            while (g_keepAliveRunning)
                KeepAlive();
        });
}

static void StopKeepAlive()
{
    g_keepAliveRunning = false;
    if (g_keepAliveThread.joinable())
        g_keepAliveThread.join();
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

    g_lastGameFrameTime = std::chrono::steady_clock::now();

    // With the pacing thread the frame is handed over rather than ended: the
    // layer below, with the images just released, is what the thread's next
    // xrEndFrame submits. Both happen under g_mutex, so the thread never
    // submits these images with another frame's poses.
    if (g_pacingEnabled)
    {
        double frameMs = std::chrono::duration<double, std::milli>(g_lastGameFrameTime - g_gameFrameBeginTime).count();
        g_gameFrameMs += (std::clamp(frameMs, 0.0, 50.0) - g_gameFrameMs) * 0.2;
    }

    auto endFrame = [&](const RetainedLayer& layer)
        {
            g_retainedLayer = layer;

            if (g_pacingEnabled)
            {
                ++g_publishSerial;
                g_pacingCondition.notify_all();
            }
            else
            {
                EndFrameWithLayer(g_frame, layer);
            }
        };

    if (mode == VRFrameMode::None || !g_frame.should_render)
    {
        if (g_pacingEnabled)
        {
            // The images released hold no picture: nothing to show until the next frame.
            endFrame({});
        }
        else
        {
            auto queueLock = LockGraphicsQueue();
            g_runtime->EndFrameWithoutLayers(g_frame);
        }

        return true;
    }

    RetainedLayer layer;
    layer.mode = mode;

    if (mode == VRFrameMode::Immersive)
    {
        // Submit the poses the eyes were rendered with, never fresher ones: the
        // compositor reprojects from these to the display time.
        XrCompositionLayerProjectionView* projectionViews = layer.views;

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

        endFrame(layer);
        return true;
    }

    XrCompositionLayerQuad& quad = layer.quad;
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

    // The image holds the game's picture stretched to the eyes' shape: the quad
    // takes the shape the game drew for. VRScreenWidth is the width of a 16:9
    // screen; other shapes keep its height.
    quad.size.height = std::max(0.25f, Config::VRScreenWidth.Value) * 9.0f / 16.0f;
    quad.size.width = quad.size.height * std::clamp(result.screenAspectRatio, 0.25f, 4.0f);

    endFrame(layer);
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

void VR::SetRenderScale(float scale)
{
    g_pendingRenderScale = scale;
}

bool VR::IsRenderScalePending()
{
    return g_pendingRenderScale.load() > 0.0f;
}

bool VR::ApplyRenderScale()
{
    float scale = g_pendingRenderScale.exchange(-1.0f);
    if (scale <= 0.0f)
        return false;

    std::lock_guard lock(g_mutex);
    if (g_runtime == nullptr || !g_runtime->HasSession() || g_frameBegun)
        return false;

    uint32_t width, height;
    GetRenderSize(scale, width, height);
    if (width == g_renderWidth && height == g_renderHeight)
        return false;

    // The retained layer names the old swapchains.
    g_retainedLayer = {};

    for (XrSwapchainState* swapchain : { &g_screenSwapchain, &g_eyeSwapchains[0], &g_eyeSwapchains[1] })
        DestroySwapchain(*swapchain);

    if (!CreateSwapchain(g_screenSwapchain, width, height, "screen") ||
        !CreateSwapchain(g_eyeSwapchains[0], width, height, "left eye") ||
        !CreateSwapchain(g_eyeSwapchains[1], width, height, "right eye"))
    {
        // Back to the previous resolution, which the runtime accepted before.
        for (XrSwapchainState* swapchain : { &g_screenSwapchain, &g_eyeSwapchains[0], &g_eyeSwapchains[1] })
            DestroySwapchain(*swapchain);

        CreateSwapchain(g_screenSwapchain, g_renderWidth, g_renderHeight, "screen");
        CreateSwapchain(g_eyeSwapchains[0], g_renderWidth, g_renderHeight, "left eye");
        CreateSwapchain(g_eyeSwapchains[1], g_renderWidth, g_renderHeight, "right eye");
        return false;
    }

    LOGFN("[VR] Rendering at {}x{} per eye (scale {:.2f}).", width, height, scale);
    g_renderWidth = width;
    g_renderHeight = height;
    return true;
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
