// SPDX-License-Identifier: GPL-3.0-or-later
//
// UnleashedRecompVR: no-op stand-in for PrimedGun's frame-timing diagnostics.
// openxr_runtime.cpp is vendored verbatim and calls these at its frame-protocol
// points; this header gives them empty bodies so the core needs no other file.

#pragma once

#include <openxr/openxr.h>

#include <cstdint>
#include <string_view>
#include <utility>

namespace PortVr {
namespace diagnostics {

enum class Stage : uint8_t {
    PollEvents, BeginCall, LocateViews, InputSync, SyncActions, Publish,
};

class Stopwatch {
public:
    Stopwatch() noexcept = default;
};

template <typename Call>
decltype(auto) Measure(Stage, Call&& call) {
    return std::forward<Call>(call)();
}

inline void OnWaitFrame(const Stopwatch&, int64_t, int64_t) noexcept {}
inline void OnBeginFrame() noexcept {}
inline void OnEndFrame(const Stopwatch&) noexcept {}
inline void OnReferenceSpaceChange(std::string_view, bool, int64_t) noexcept {}

} // namespace diagnostics
} // namespace PortVr
