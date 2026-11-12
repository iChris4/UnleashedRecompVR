#include <api/SWA.h>
#include <ui/game_window.h>
#include <user/config.h>
#include <gpu/video.h>
#include <xr/xr.h>
#include <xr/xr_math.h>
#include "camera_patches.h"
#include "aspect_ratio_patches.h"
#include "inspire_patches.h"

static bool IsVRCutsceneOnScreen()
{
    return Config::VRCutsceneScreen && InspirePatches::s_sceneActive;
}

// VR: the game camera is framed for the headset (wider culling, HUD markers on
// the panel), except in cutscenes on the floating screen and on the world map,
// which places and picks its markers through its own camera.
static bool IsVRFramingActive()
{
    return VR::IsActive() && !IsVRCutsceneOnScreen() && !IsWorldMapActive();
}

bool MapVRScreenPosition(float& x, float& y)
{
    if (!IsVRFramingActive())
        return false;

    float lensX, lensY;
    XrMatrix horizonLock;
    if (!Video::GetVRCameraState(lensX, lensY, horizonLock.m) || lensX <= 0.0f || lensY <= 0.0f)
        return false;

    // The direction in the game camera's view (through its widened lens), then
    // in the levelled space of the panel.
    float view[3] = { (x / 640.0f - 1.0f) / lensX, (1.0f - y / 360.0f) / lensY, -1.0f };
    float levelled[3];

    for (size_t j = 0; j < 3; j++)
        levelled[j] = view[0] * horizonLock.m[0 * 4 + j] + view[1] * horizonLock.m[1 * 4 + j] + view[2] * horizonLock.m[2 * 4 + j];

    float length = sqrtf(levelled[0] * levelled[0] + levelled[1] * levelled[1] + levelled[2] * levelled[2]);
    if (-levelled[2] <= 1.0e-3f * length)
    {
        // Behind the panel: far off the screen.
        x = -100000.0f;
        y = -100000.0f;
        return true;
    }

    // The panel (see LatchVRViews in video.cpp): VRHudWidth wide, the canvas's
    // shape, VRHudDistance ahead.
    float halfWidth = 0.5f * std::max(0.1f, Config::VRHudWidth.Value);
    float aspectRatio = float(Video::s_viewportWidth) / float(std::max(1u, Video::s_viewportHeight));
    float distance = std::max(0.1f, Config::VRHudDistance.Value);

    float canvasX = levelled[0] / -levelled[2] * distance / halfWidth;
    float canvasY = levelled[1] / -levelled[2] * distance / (halfWidth / aspectRatio);

    x = (canvasX + 1.0f) * 640.0f;
    y = (1.0f - canvasY) * 360.0f;
    return true;
}

void CameraAspectRatioMidAsmHook(PPCRegister& r30, PPCRegister& r31)
{
    r30.u32 = 0;

    auto camera = (SWA::CCamera*)g_memory.Translate(r31.u32);

    // Dynamically adjust horizontal aspect ratio to window dimensions.
    camera->m_HorzAspectRatio = g_aspectRatio;
}

float AdjustFieldOfView(float fieldOfView, float aspectRatio)
{
    if (Config::AspectRatio == EAspectRatio::OriginalNarrow)
    {
        // Replicate the original incorrect field of view formula if requested.
        fieldOfView *= NARROW_ASPECT_RATIO;
    }
    else if (aspectRatio < WIDE_ASPECT_RATIO)
    {
        // Use proper VERT+ otherwise for narrow aspect ratios.
        fieldOfView = 2.0 * atan(tan(0.5 * fieldOfView) / aspectRatio * WIDE_ASPECT_RATIO);
    }

    return fieldOfView;
}

void CameraFieldOfViewMidAsmHook(PPCRegister& r31, PPCRegister& f31)
{
    auto camera = (SWA::CCamera*)g_memory.Translate(r31.u32);

    float fieldOfView = AdjustFieldOfView(f31.f64, camera->m_HorzAspectRatio);

    // VR: the game culls with its camera's field of view, which the eyes see past:
    // widen it to [VR] VRCullingFieldOfView. The eyes are drawn with their own
    // lenses, so this only changes what is culled and the game's own projections,
    // which MapVRScreenPosition undoes for the HUD markers.
    if (IsVRFramingActive())
    {
        float cullingFieldOfView = std::clamp(Config::VRCullingFieldOfView.Value, 10.0f, 170.0f) * float(M_PI) / 180.0f;
        fieldOfView = std::max(fieldOfView, cullingFieldOfView);
    }

    f31.f64 = fieldOfView;
}

PPC_FUNC_IMPL(__imp__sub_824697B0);
PPC_FUNC(sub_824697B0)
{
    auto pCamera = (SWA::CCamera*)g_memory.Translate(ctx.r3.u32);

    pCamera->m_InvertX = Config::HorizontalCamera == ECameraRotationMode::Reverse;
    pCamera->m_InvertY = Config::VerticalCamera == ECameraRotationMode::Reverse;

    __imp__sub_824697B0(ctx, base);
}

// SWA::CCamera::UpdateSerial
PPC_FUNC_IMPL(__imp__sub_82467890);
PPC_FUNC(sub_82467890)
{
    if (g_needsResize)
    {
        // Recompute the projection matrix for one frame to fix stretching on pause menu.
        auto r3 = ctx.r3;
        auto r4 = ctx.r4;
        sub_82468E38(ctx, base);
        ctx.r3 = r3;
        ctx.r4 = r4;
    }

    __imp__sub_82467890(ctx, base);
}
