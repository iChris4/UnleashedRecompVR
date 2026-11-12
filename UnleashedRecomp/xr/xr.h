#pragma once

#include <plume_render_interface.h>
#include <xbox.h>

// OpenXR integration. The headset shows the game on a floating screen.
//
// Threads: Init and CreateSession run on the main thread inside
// Video::CreateHostDevice. The frame functions run on whichever guest thread
// presents (the main thread, or the loading thread while it owns presentation);
// they are serialised internally. The render thread never calls OpenXR: it only
// renders into the image AcquireScreenImage returned for the current frame.
struct VR
{
    // Set from --vr or [VR] Enabled before the video device is created.
    static inline bool s_requested;

#ifdef UNLEASHED_RECOMP_OPENXR
    // Creates the OpenXR instance and finds the headset. False keeps the game flat.
    static bool Init();
    static bool IsInitialized();

    // The adapter the runtime renders on, as plume's preferred device name.
    static std::string GetAdapterName();

    // Creates the session and its swapchains on the game's device and queue.
    static bool CreateSession(plume::RenderDevice* device, plume::RenderCommandQueue* queue);

    // A session exists. The game then renders its 16:9 picture at the screen
    // resolution, whatever the window size.
    static bool IsActive();

    // Releases everything; the game continues flat.
    static void Shutdown();

    // Waits for the headset's next frame and begins it. Does nothing when a
    // frame is already begun or the session is not running.
    static void BeginFrame();

    // The screen image to fill for the frame begun last, or null when the
    // headset shows nothing this frame. Acquired once per frame.
    static plume::RenderTexture* AcquireScreenImage();

    // Releases the image and submits the frame. True when a frame begun with
    // xrWaitFrame was ended, so the headset paced this frame.
    static bool EndFrame();

    // Places the screen in front of the head again on the next frame. The
    // runtime's own recenter (a reference space change) does this too.
    static void Recenter();

    // The format to render the screen image in before it is copied into the
    // swapchain image, and the screen's resolution.
    static plume::RenderFormat GetScreenFormat();
    static uint32_t GetScreenWidth();
    static uint32_t GetScreenHeight();

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
    static plume::RenderTexture* AcquireScreenImage() { return nullptr; }
    static bool EndFrame() { return false; }
    static void Recenter() {}
    static plume::RenderFormat GetScreenFormat() { return plume::RenderFormat::B8G8R8A8_UNORM; }
    static uint32_t GetScreenWidth() { return 1920; }
    static uint32_t GetScreenHeight() { return 1080; }
    static bool IsInputFocused() { return false; }
    static bool GetGamepad(XAMINPUT_GAMEPAD&) { return false; }
    static void SetRumble(uint16_t, uint16_t) {}
#endif
};
