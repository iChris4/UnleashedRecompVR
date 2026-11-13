#pragma once

// VR: the HUD panel transform, from SharedConstants in video.cpp (vrHudMatrix at
// byte 288 / c18, the left eye's then the right eye's, and vrHudEnabled at byte
// 416 / c26.x). When enabled, a point of the HUD canvas in normalised device
// coordinates is put on a panel in front of the game camera and projected for
// the eye being drawn. The VR_STEREO variants follow g_VREyeMode's panel bit
// instead (shader_common.h), which declares the matrices for them.
//
// Include after the constant buffers. On D3D12 the shader declares in its
// SharedConstants buffer, without VR_STEREO:
//     float4 g_VRHudMatrix[4] : packoffset(c18);
//     uint g_VRHudEnabled : packoffset(c26.x);

#ifdef VR_STEREO

float4 ApplyVRHud(float2 ndc, float z, uint eye)
{
    float4 position = float4(ndc, z, 1.0);
    VR_STEREO_APPLY_PANEL(position, eye);
    return position;
}

#else

#ifdef __spirv__
#define g_VRHudRow(INDEX) vk::RawBufferLoad<float4>(g_PushConstants.SharedConstants + 288 + (INDEX) * 16, 0x10)
#define g_VRHudEnabled vk::RawBufferLoad<uint>(g_PushConstants.SharedConstants + 416)
#else
#define g_VRHudRow(INDEX) g_VRHudMatrix[INDEX]
#endif

// Row-vector transform of (x, y, 0, 1); depth keeps the HUD's own value. The
// eye drawn has its matrix first.
float4 ApplyVRHud(float2 ndc, float z, uint eye)
{
    if (g_VRHudEnabled != 0)
    {
        float4 clip = ndc.x * g_VRHudRow(0) + ndc.y * g_VRHudRow(1) + g_VRHudRow(3);
        return float4(clip.xy, z * clip.w, clip.w);
    }

    return float4(ndc, z, 1.0);
}

#endif
