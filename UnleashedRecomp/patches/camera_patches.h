#pragma once

extern float AdjustFieldOfView(float fieldOfView, float aspectRatio);

// VR: moves a screen position the game projected (1280x720 space) onto the HUD
// panel, where the direction it stands for crosses it. False when not in stereo.
extern bool MapVRScreenPosition(float& x, float& y);
