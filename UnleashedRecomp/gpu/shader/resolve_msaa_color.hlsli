#pragma once

#include "copy_common.hlsli"
#include "vr_stereo.hlsli"

VR_TEXTURE_2D_MS(float4, SAMPLE_COUNT) g_Texture2DMSDescriptorHeap[] : register(t0, space0);

float4 main(in float4 position : SV_Position VR_PS_EYE_INPUTS) : SV_Target
{
    float4 result = g_Texture2DMSDescriptorHeap[g_PushConstants.ResourceDescriptorIndex].Load(VR_LOAD_MS_COORD(position.xy, VR_PS_EYE), 0);
    
    [unroll] for (int i = 1; i < SAMPLE_COUNT; i++)
        result += g_Texture2DMSDescriptorHeap[g_PushConstants.ResourceDescriptorIndex].Load(VR_LOAD_MS_COORD(position.xy, VR_PS_EYE), i);

    return result / SAMPLE_COUNT;
}
