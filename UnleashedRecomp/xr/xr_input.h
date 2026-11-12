#pragma once

#include <openxr/openxr.h>
#include <xbox.h>

namespace PortVr
{
    class OpenXRRuntime;
}

// OpenXR motion controllers as an Xbox 360 pad, by physical position:
//
//   right A / B -> A / B            left X / Y -> X / Y
//   triggers    -> LT / RT          grips      -> LB / RB
//   thumbsticks -> sticks           stick clicks -> L3 / R3
//   left menu   -> Start            left menu with the left grip held -> Back
//   right menu (controllers that have one) -> Back
//
// Sync runs on the presenting thread once per xrWaitFrame; the game's input
// threads read the last pad through GetGamepad. Rumble requested by the game is
// applied to the hands on the next Sync (left motor -> left hand, right -> right).
namespace XrInput
{
    // Creates the action set and attaches it to the session (allowed once per session).
    // False leaves VR running without motion controllers.
    bool Create(PortVr::OpenXRRuntime& runtime);
    void Destroy();

    // Reads the controllers for the frame. Call only while the session is focused.
    void Sync();

    // Releases every button and stops the haptics, while the session is not focused.
    void Idle();

    // The pad as of the last Sync, in host byte order. False when no controller is bound.
    bool GetGamepad(XAMINPUT_GAMEPAD& pad);

    // XInput motor speeds, 0-65535. Any thread.
    void SetRumble(uint16_t leftMotorSpeed, uint16_t rightMotorSpeed);
}
