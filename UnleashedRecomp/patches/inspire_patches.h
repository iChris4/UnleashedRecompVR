#pragma once

class InspirePatches
{
public:
    static std::string s_sceneName;

    // A cutscene scene exists (from its constructor to its destructor). Any thread.
    static inline std::atomic<bool> s_sceneActive;

    static void DrawDebug();
    static void Update();
};
