// The action list and suggested bindings are adapted from PrimedGun's openxr_input.cpp.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xr_input.h"

#include <vr/openxr_runtime.h>
#include <os/logger.h>

static constexpr uint32_t HAND_LEFT = 0;
static constexpr uint32_t HAND_RIGHT = 1;
static constexpr uint32_t HAND_COUNT = 2;

// Rumble is re-applied every frame while the game asks for it, so a pulse only
// needs to outlast a frame; a stalled frame then cannot leave a hand buzzing.
static constexpr XrDuration RUMBLE_PULSE_NS = 100'000'000;

// The grip is a button for the game; hysteresis keeps a half-squeeze from chattering.
static constexpr float GRIP_PRESS_THRESHOLD = 0.6f;
static constexpr float GRIP_RELEASE_THRESHOLD = 0.4f;

static PortVr::OpenXRRuntime* g_runtime;

static XrActionSet g_actionSet = XR_NULL_HANDLE;
static XrAction g_thumbstick = XR_NULL_HANDLE;
static XrAction g_thumbstickClick = XR_NULL_HANDLE;
static XrAction g_trigger = XR_NULL_HANDLE;
static XrAction g_squeeze = XR_NULL_HANDLE;
static XrAction g_squeezeClick = XR_NULL_HANDLE;
static XrAction g_buttonPrimary = XR_NULL_HANDLE;   // A / X
static XrAction g_buttonSecondary = XR_NULL_HANDLE; // B / Y
static XrAction g_menu = XR_NULL_HANDLE;
static XrAction g_haptic = XR_NULL_HANDLE;
static XrPath g_handPaths[HAND_COUNT];

static uint64_t g_profileSerial;
static bool g_loggedSyncFailure;
static bool g_gripHeld[HAND_COUNT];
static bool g_hapticActive[HAND_COUNT];

static std::mutex g_padMutex;
static XAMINPUT_GAMEPAD g_pad;
static bool g_padConnected;

static std::atomic<uint32_t> g_rumble; // Left motor in the high 16 bits, right motor in the low 16 bits.

struct XrBinding
{
    XrAction* action;
    const char* path;
};

static bool Check(XrResult result, const char* operation)
{
    if (g_runtime != nullptr)
        g_runtime->ObserveResult(result);

    if (XR_SUCCEEDED(result))
        return true;

    LOGFN_ERROR("[VR] {} failed ({}).", operation, int32_t(result));
    return false;
}

static bool CreateActions()
{
    XrActionSetCreateInfo setInfo{ XR_TYPE_ACTION_SET_CREATE_INFO };
    strncpy(setInfo.actionSetName, "gameplay", XR_MAX_ACTION_SET_NAME_SIZE - 1);
    strncpy(setInfo.localizedActionSetName, "Gameplay", XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE - 1);
    if (!Check(xrCreateActionSet(g_runtime->Instance(), &setInfo, &g_actionSet), "xrCreateActionSet"))
        return false;

    struct Spec
    {
        XrAction* action;
        const char* name;
        const char* localizedName;
        XrActionType type;
    };

    const Spec specs[] =
    {
        { &g_thumbstick, "thumbstick", "Thumbstick", XR_ACTION_TYPE_VECTOR2F_INPUT },
        { &g_thumbstickClick, "thumbstick_click", "Thumbstick Click", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { &g_trigger, "trigger", "Trigger", XR_ACTION_TYPE_FLOAT_INPUT },
        { &g_squeeze, "squeeze", "Grip", XR_ACTION_TYPE_FLOAT_INPUT },
        { &g_squeezeClick, "squeeze_click", "Grip Click", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { &g_buttonPrimary, "button_primary", "A / X", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { &g_buttonSecondary, "button_secondary", "B / Y", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { &g_menu, "menu", "Start", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { &g_haptic, "haptic", "Rumble", XR_ACTION_TYPE_VIBRATION_OUTPUT },
    };

    for (const Spec& spec : specs)
    {
        XrActionCreateInfo info{ XR_TYPE_ACTION_CREATE_INFO };
        info.actionType = spec.type;
        strncpy(info.actionName, spec.name, XR_MAX_ACTION_NAME_SIZE - 1);
        strncpy(info.localizedActionName, spec.localizedName, XR_MAX_LOCALIZED_ACTION_NAME_SIZE - 1);
        info.countSubactionPaths = HAND_COUNT;
        info.subactionPaths = g_handPaths;

        if (!Check(xrCreateAction(g_actionSet, &info, spec.action), spec.name))
            return false;
    }

    return true;
}

static void Suggest(const char* profile, std::vector<XrBinding> bindings)
{
    bindings.push_back({ &g_haptic, "/user/hand/left/output/haptic" });
    bindings.push_back({ &g_haptic, "/user/hand/right/output/haptic" });

    XrPath profilePath = XR_NULL_PATH;
    if (XR_FAILED(xrStringToPath(g_runtime->Instance(), profile, &profilePath)))
        return;

    std::vector<XrActionSuggestedBinding> suggested;
    suggested.reserve(bindings.size());

    for (const XrBinding& binding : bindings)
    {
        XrPath path = XR_NULL_PATH;
        if (XR_SUCCEEDED(xrStringToPath(g_runtime->Instance(), binding.path, &path)))
            suggested.push_back({ *binding.action, path });
    }

    XrInteractionProfileSuggestedBinding info{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
    info.interactionProfile = profilePath;
    info.countSuggestedBindings = uint32_t(suggested.size());
    info.suggestedBindings = suggested.data();

    XrResult result = xrSuggestInteractionProfileBindings(g_runtime->Instance(), &info);
    g_runtime->ObserveResult(result);
    if (XR_FAILED(result))
        LOGFN_WARNING("[VR] xrSuggestInteractionProfileBindings({}) failed ({}).", profile, int32_t(result));
}

static void SuggestBindings()
{
    // Meta Touch (also what SteamVR offers controllers it has no own profile for,
    // such as the PS VR2 Sense controllers), and PICO, which uses the same paths.
    const std::vector<XrBinding> touch =
    {
        { &g_thumbstick, "/user/hand/left/input/thumbstick" },
        { &g_thumbstick, "/user/hand/right/input/thumbstick" },
        { &g_thumbstickClick, "/user/hand/left/input/thumbstick/click" },
        { &g_thumbstickClick, "/user/hand/right/input/thumbstick/click" },
        { &g_trigger, "/user/hand/left/input/trigger/value" },
        { &g_trigger, "/user/hand/right/input/trigger/value" },
        { &g_squeeze, "/user/hand/left/input/squeeze/value" },
        { &g_squeeze, "/user/hand/right/input/squeeze/value" },
        { &g_buttonPrimary, "/user/hand/left/input/x/click" },
        { &g_buttonPrimary, "/user/hand/right/input/a/click" },
        { &g_buttonSecondary, "/user/hand/left/input/y/click" },
        { &g_buttonSecondary, "/user/hand/right/input/b/click" },
        { &g_menu, "/user/hand/left/input/menu/click" },
    };

    Suggest("/interaction_profiles/oculus/touch_controller", touch);
    Suggest("/interaction_profiles/bytedance/pico4_controller", touch);
    Suggest("/interaction_profiles/bytedance/pico_neo3_controller", touch);

    // Valve Index: A / B on both hands, no menu button available to applications.
    Suggest("/interaction_profiles/valve/index_controller",
    {
        { &g_thumbstick, "/user/hand/left/input/thumbstick" },
        { &g_thumbstick, "/user/hand/right/input/thumbstick" },
        { &g_thumbstickClick, "/user/hand/left/input/thumbstick/click" },
        { &g_thumbstickClick, "/user/hand/right/input/thumbstick/click" },
        { &g_trigger, "/user/hand/left/input/trigger/value" },
        { &g_trigger, "/user/hand/right/input/trigger/value" },
        { &g_squeeze, "/user/hand/left/input/squeeze/value" },
        { &g_squeeze, "/user/hand/right/input/squeeze/value" },
        { &g_buttonPrimary, "/user/hand/left/input/a/click" },
        { &g_buttonPrimary, "/user/hand/right/input/a/click" },
        { &g_buttonSecondary, "/user/hand/left/input/b/click" },
        { &g_buttonSecondary, "/user/hand/right/input/b/click" },
    });

    // Windows Mixed Reality: a thumbstick, a menu on each hand and a grip click.
    Suggest("/interaction_profiles/microsoft/motion_controller",
    {
        { &g_thumbstick, "/user/hand/left/input/thumbstick" },
        { &g_thumbstick, "/user/hand/right/input/thumbstick" },
        { &g_thumbstickClick, "/user/hand/left/input/thumbstick/click" },
        { &g_thumbstickClick, "/user/hand/right/input/thumbstick/click" },
        { &g_trigger, "/user/hand/left/input/trigger/value" },
        { &g_trigger, "/user/hand/right/input/trigger/value" },
        { &g_squeezeClick, "/user/hand/left/input/squeeze/click" },
        { &g_squeezeClick, "/user/hand/right/input/squeeze/click" },
        { &g_buttonPrimary, "/user/hand/right/input/trackpad/click" },
        { &g_buttonSecondary, "/user/hand/left/input/trackpad/click" },
        { &g_menu, "/user/hand/left/input/menu/click" },
        { &g_menu, "/user/hand/right/input/menu/click" },
    });

    // HTC Vive wands: the trackpad stands in for the thumbstick.
    Suggest("/interaction_profiles/htc/vive_controller",
    {
        { &g_thumbstick, "/user/hand/left/input/trackpad" },
        { &g_thumbstick, "/user/hand/right/input/trackpad" },
        { &g_buttonPrimary, "/user/hand/right/input/trackpad/click" },
        { &g_buttonSecondary, "/user/hand/left/input/trackpad/click" },
        { &g_trigger, "/user/hand/left/input/trigger/value" },
        { &g_trigger, "/user/hand/right/input/trigger/value" },
        { &g_squeezeClick, "/user/hand/left/input/squeeze/click" },
        { &g_squeezeClick, "/user/hand/right/input/squeeze/click" },
        { &g_menu, "/user/hand/left/input/menu/click" },
        { &g_menu, "/user/hand/right/input/menu/click" },
    });

    // Minimal fallback so an unknown runtime still offers jump and pause.
    Suggest("/interaction_profiles/khr/simple_controller",
    {
        { &g_buttonPrimary, "/user/hand/right/input/select/click" },
        { &g_buttonSecondary, "/user/hand/left/input/select/click" },
        { &g_menu, "/user/hand/left/input/menu/click" },
    });
}

static void LogInteractionProfiles()
{
    std::string profiles[HAND_COUNT];

    for (uint32_t hand = 0; hand < HAND_COUNT; hand++)
    {
        XrInteractionProfileState state{ XR_TYPE_INTERACTION_PROFILE_STATE };
        char path[XR_MAX_PATH_LENGTH]{};
        uint32_t length = 0;

        if (XR_SUCCEEDED(xrGetCurrentInteractionProfile(g_runtime->Session(), g_handPaths[hand], &state)) &&
            state.interactionProfile != XR_NULL_PATH &&
            XR_SUCCEEDED(xrPathToString(g_runtime->Instance(), state.interactionProfile, sizeof(path), &length, path)))
        {
            profiles[hand] = path;
        }
        else
        {
            profiles[hand] = "none";
        }
    }

    LOGFN("[VR] Interaction profiles: left {}, right {}.", profiles[HAND_LEFT], profiles[HAND_RIGHT]);
}

static void ApplyHaptic(uint32_t hand, float amplitude)
{
    XrHapticActionInfo info{ XR_TYPE_HAPTIC_ACTION_INFO };
    info.action = g_haptic;
    info.subactionPath = g_handPaths[hand];

    if (amplitude <= 0.0f)
    {
        xrStopHapticFeedback(g_runtime->Session(), &info);
        return;
    }

    XrHapticVibration vibration{ XR_TYPE_HAPTIC_VIBRATION };
    vibration.amplitude = std::clamp(amplitude, 0.0f, 1.0f);
    vibration.duration = RUMBLE_PULSE_NS;
    vibration.frequency = XR_FREQUENCY_UNSPECIFIED;
    xrApplyHapticFeedback(g_runtime->Session(), &info, reinterpret_cast<const XrHapticBaseHeader*>(&vibration));
}

static void UpdateRumble()
{
    uint32_t rumble = g_rumble.load();
    float amplitudes[HAND_COUNT] =
    {
        float(rumble >> 16) / 65535.0f,
        float(rumble & 0xFFFF) / 65535.0f
    };

    for (uint32_t hand = 0; hand < HAND_COUNT; hand++)
    {
        if (amplitudes[hand] > 0.0f)
        {
            ApplyHaptic(hand, amplitudes[hand]);
            g_hapticActive[hand] = true;
        }
        else if (g_hapticActive[hand])
        {
            ApplyHaptic(hand, 0.0f);
            g_hapticActive[hand] = false;
        }
    }
}

static int16_t ToStickAxis(float value)
{
    return int16_t(std::clamp(value, -1.0f, 1.0f) * 32767.0f);
}

static uint8_t ToTrigger(float value)
{
    return uint8_t(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
}

bool XrInput::Create(PortVr::OpenXRRuntime& runtime)
{
    g_runtime = &runtime;

    if (!Check(xrStringToPath(runtime.Instance(), "/user/hand/left", &g_handPaths[HAND_LEFT]), "xrStringToPath") ||
        !Check(xrStringToPath(runtime.Instance(), "/user/hand/right", &g_handPaths[HAND_RIGHT]), "xrStringToPath") ||
        !CreateActions())
    {
        Destroy();
        return false;
    }

    SuggestBindings();

    XrSessionActionSetsAttachInfo attachInfo{ XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
    attachInfo.countActionSets = 1;
    attachInfo.actionSets = &g_actionSet;
    if (!Check(xrAttachSessionActionSets(runtime.Session(), &attachInfo), "xrAttachSessionActionSets"))
    {
        Destroy();
        return false;
    }

    g_profileSerial = runtime.InteractionProfileSerial();
    LOGN("[VR] Motion controller actions attached.");
    return true;
}

void XrInput::Destroy()
{
    Idle();

    // Destroying the set destroys every action created from it.
    if (g_actionSet != XR_NULL_HANDLE)
        xrDestroyActionSet(g_actionSet);

    g_actionSet = XR_NULL_HANDLE;
    g_thumbstick = g_thumbstickClick = g_trigger = g_squeeze = g_squeezeClick = XR_NULL_HANDLE;
    g_buttonPrimary = g_buttonSecondary = g_menu = g_haptic = XR_NULL_HANDLE;
    g_runtime = nullptr;
}

void XrInput::Idle()
{
    {
        std::lock_guard lock(g_padMutex);
        g_pad = {};
        g_padConnected = false;
    }

    g_gripHeld[HAND_LEFT] = g_gripHeld[HAND_RIGHT] = false;

    if (g_runtime == nullptr || g_haptic == XR_NULL_HANDLE)
        return;

    for (uint32_t hand = 0; hand < HAND_COUNT; hand++)
    {
        if (g_hapticActive[hand])
        {
            ApplyHaptic(hand, 0.0f);
            g_hapticActive[hand] = false;
        }
    }
}

void XrInput::Sync()
{
    if (g_runtime == nullptr || g_actionSet == XR_NULL_HANDLE)
        return;

    XrActiveActionSet activeSet{ g_actionSet, XR_NULL_PATH };
    XrActionsSyncInfo syncInfo{ XR_TYPE_ACTIONS_SYNC_INFO };
    syncInfo.countActiveActionSets = 1;
    syncInfo.activeActionSets = &activeSet;

    XrResult result = xrSyncActions(g_runtime->Session(), &syncInfo);
    g_runtime->ObserveResult(result);
    if (XR_FAILED(result))
    {
        if (!g_loggedSyncFailure)
        {
            g_loggedSyncFailure = true;
            LOGFN_WARNING("[VR] xrSyncActions failed ({}).", int32_t(result));
        }

        Idle();
        return;
    }

    if (g_runtime->InteractionProfileSerial() != g_profileSerial)
    {
        g_profileSerial = g_runtime->InteractionProfileSerial();
        LogInteractionProfiles();
    }

    bool connected = false;

    auto getBoolean = [&](XrAction action, uint32_t hand)
    {
        XrActionStateGetInfo info{ XR_TYPE_ACTION_STATE_GET_INFO };
        info.action = action;
        info.subactionPath = g_handPaths[hand];
        XrActionStateBoolean state{ XR_TYPE_ACTION_STATE_BOOLEAN };
        bool active = XR_SUCCEEDED(xrGetActionStateBoolean(g_runtime->Session(), &info, &state)) && state.isActive;
        connected |= active;
        return active && state.currentState;
    };

    auto getFloat = [&](XrAction action, uint32_t hand)
    {
        XrActionStateGetInfo info{ XR_TYPE_ACTION_STATE_GET_INFO };
        info.action = action;
        info.subactionPath = g_handPaths[hand];
        XrActionStateFloat state{ XR_TYPE_ACTION_STATE_FLOAT };
        bool active = XR_SUCCEEDED(xrGetActionStateFloat(g_runtime->Session(), &info, &state)) && state.isActive;
        connected |= active;
        return active ? state.currentState : 0.0f;
    };

    auto getVector = [&](XrAction action, uint32_t hand)
    {
        XrActionStateGetInfo info{ XR_TYPE_ACTION_STATE_GET_INFO };
        info.action = action;
        info.subactionPath = g_handPaths[hand];
        XrActionStateVector2f state{ XR_TYPE_ACTION_STATE_VECTOR2F };
        bool active = XR_SUCCEEDED(xrGetActionStateVector2f(g_runtime->Session(), &info, &state)) && state.isActive;
        connected |= active;
        return active ? state.currentState : XrVector2f{ 0.0f, 0.0f };
    };

    XAMINPUT_GAMEPAD pad{};

    for (uint32_t hand = 0; hand < HAND_COUNT; hand++)
    {
        bool left = (hand == HAND_LEFT);

        if (getBoolean(g_buttonPrimary, hand))
            pad.wButtons |= left ? XAMINPUT_GAMEPAD_X : XAMINPUT_GAMEPAD_A;

        if (getBoolean(g_buttonSecondary, hand))
            pad.wButtons |= left ? XAMINPUT_GAMEPAD_Y : XAMINPUT_GAMEPAD_B;

        if (getBoolean(g_thumbstickClick, hand))
            pad.wButtons |= left ? XAMINPUT_GAMEPAD_LEFT_THUMB : XAMINPUT_GAMEPAD_RIGHT_THUMB;

        float squeeze = getFloat(g_squeeze, hand);
        bool squeezeClick = getBoolean(g_squeezeClick, hand);
        g_gripHeld[hand] = squeezeClick || squeeze >= (g_gripHeld[hand] ? GRIP_RELEASE_THRESHOLD : GRIP_PRESS_THRESHOLD);

        if (g_gripHeld[hand])
            pad.wButtons |= left ? XAMINPUT_GAMEPAD_LEFT_SHOULDER : XAMINPUT_GAMEPAD_RIGHT_SHOULDER;

        uint8_t trigger = ToTrigger(getFloat(g_trigger, hand));
        XrVector2f stick = getVector(g_thumbstick, hand);

        if (left)
        {
            pad.bLeftTrigger = trigger;
            pad.sThumbLX = ToStickAxis(stick.x);
            pad.sThumbLY = ToStickAxis(stick.y);
        }
        else
        {
            pad.bRightTrigger = trigger;
            pad.sThumbRX = ToStickAxis(stick.x);
            pad.sThumbRY = ToStickAxis(stick.y);
        }

        if (getBoolean(g_menu, hand))
        {
            // Touch-style controllers have a single menu button: the left grip turns it into Back.
            bool back = !left || g_gripHeld[HAND_LEFT];
            pad.wButtons |= back ? XAMINPUT_GAMEPAD_BACK : XAMINPUT_GAMEPAD_START;
        }
    }

    {
        std::lock_guard lock(g_padMutex);
        g_pad = pad;
        g_padConnected = connected;
    }

    UpdateRumble();
}

bool XrInput::GetGamepad(XAMINPUT_GAMEPAD& pad)
{
    std::lock_guard lock(g_padMutex);
    pad = g_pad;
    return g_padConnected;
}

void XrInput::SetRumble(uint16_t leftMotorSpeed, uint16_t rightMotorSpeed)
{
    g_rumble = (uint32_t(leftMotorSpeed) << 16) | rightMotorSpeed;
}
