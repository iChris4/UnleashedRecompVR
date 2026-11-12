#pragma once

#include <plume_render_interface.h>
#include <xbox.h>

// The headset's views for one frame, for the renderer. Orientations and
// positions (in metres) are relative to the head pose the view was last
// recentred on (its position and heading), which is where the game camera
// stands and looks.
struct VRViews
{
    struct Eye
    {
        float orientation[4]; // x, y, z, w
        float position[3];
        float angleLeft;
        float angleRight;
        float angleUp;
        float angleDown;

        // The pose in the application space, as submitted to the runtime.
        float spaceOrientation[4];
        float spacePosition[3];
    };

    bool valid = false;
    Eye eyes[2]{};
    float basePosition[3]{}; // The recentred head position in the application space.
};

// The headset images to fill for the frame being presented. Either the screen
// (flat frames) or both eyes (immersive frames) are submitted.
struct VRImages
{
    plume::RenderTexture* screen = nullptr;
    plume::RenderTexture* eyes[2]{};
};

enum class VRFrameMode
{
    // Nothing was rendered for the headset this frame.
    None,

    // The game's picture on a floating screen: movies, loading, 2D menus.
    Flat,

    // The world drawn per eye with head tracking.
    Immersive
};

struct VRFrameResult
{
    VRFrameMode mode = VRFrameMode::None;

    // A cutscene on the floating screen: only its 16:9 middle is shown.
    bool cutscene = false;

    // The views the eyes were rendered with (immersive frames).
    VRViews views;
};

// OpenXR integration.
//
// Threads: Init and CreateSession run on the main thread inside
// Video::CreateHostDevice. The frame functions run on whichever guest thread
// presents (the main thread, or the loading thread while it owns presentation);
// they are serialised internally. The render thread never calls OpenXR: it reads
// the views with GetViews and renders into the images AcquireImages returned.
struct VR
{
    // Set from --vr or [VR] VREnabled before the video device is created.
    static inline bool s_requested;

#ifdef UNLEASHED_RECOMP_OPENXR
    // Creates the OpenXR instance and finds the headset. False keeps the game flat.
    static bool Init();
    static bool IsInitialized();

    // The adapter the runtime renders on, as plume's preferred device name.
    static std::string GetAdapterName();

    // Creates the session and its swapchains on the game's device and queue.
    static bool CreateSession(plume::RenderDevice* device, plume::RenderCommandQueue* queue);

    // A session exists. The game then renders at the eye resolution, whatever
    // the window size, and the window only mirrors the headset.
    static bool IsActive();

    // Releases everything; the game continues flat.
    static void Shutdown();

    // Waits for the headset's next frame, begins it and locates the eyes. Does
    // nothing when a frame is already begun or the session is not running.
    static void BeginFrame();

    // Acquires the frame's images (all null when the headset shows nothing).
    static void AcquireImages(VRImages& images);

    // Releases the images and submits the frame as the render thread rendered
    // it. True when a frame begun with xrWaitFrame was ended, so the headset
    // paced this frame.
    static bool EndFrame(const VRFrameResult& result);

    // The views located for the frame begun last. Any thread.
    static void GetViews(VRViews& views);

    // Recentres on the head: the game camera and the screen move in front of it.
    // The runtime's own recenter (a reference space change) and holding both
    // thumbsticks in do the same.
    static void Recenter();

    // The game's pad (the first user's, real and motion controllers merged), as
    // the game reads it: both thumbsticks held in for a second recentre. Any thread.
    static void UpdateRecenterButtons(bool bothThumbsticksHeld);

    // The format the headset images are rendered in before they are copied
    // into the swapchain images, and the per-eye render resolution.
    static plume::RenderFormat GetImageFormat();
    static uint32_t GetRenderWidth();
    static uint32_t GetRenderHeight();

    // The headset has input focus: the player is in the game rather than the
    // runtime's dashboard. Input then counts as focused even if the window is not.
    static bool IsInputFocused();

    // The motion controllers as a pad (host byte order), as of the last frame.
    // False when no motion controller is bound.
    static bool GetGamepad(XAMINPUT_GAMEPAD& pad);

    // The game's rumble (XInput motor speeds), applied to the hands each frame.
    static void SetRumble(uint16_t leftMotorSpeed, uint16_t rightMotorSpeed);
#else
    // Builds without OpenXR: VR is never active.
    static bool Init() { return false; }
    static bool IsInitialized() { return false; }
    static std::string GetAdapterName() { return {}; }
    static bool CreateSession(plume::RenderDevice*, plume::RenderCommandQueue*) { return false; }
    static bool IsActive() { return false; }
    static void Shutdown() {}
    static void BeginFrame() {}
    static void AcquireImages(VRImages&) {}
    static bool EndFrame(const VRFrameResult&) { return false; }
    static void GetViews(VRViews& views) { views = {}; }
    static void Recenter() {}
    static void UpdateRecenterButtons(bool) {}
    static plume::RenderFormat GetImageFormat() { return plume::RenderFormat::B8G8R8A8_UNORM; }
    static uint32_t GetRenderWidth() { return 1920; }
    static uint32_t GetRenderHeight() { return 1080; }
    static bool IsInputFocused() { return false; }
    static bool GetGamepad(XAMINPUT_GAMEPAD&) { return false; }
    static void SetRumble(uint16_t, uint16_t) {}
#endif
};
