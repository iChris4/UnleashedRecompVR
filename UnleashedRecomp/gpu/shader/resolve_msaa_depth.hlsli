#pragma once

#include "copy_common.hlsli"
#include "vr_stereo.hlsli"

VR_TEXTURE_2D_MS(float, SAMPLE_COUNT) g_Texture2DMSDescriptorHeap[] : register(t0, space0);

float main(in float4 position : SV_Position VR_PS_EYE_INPUTS) : SV_Depth
{
    VR_TEXTURE_2D_MS(float, SAMPLE_COUNT) texture = g_Texture2DMSDescriptorHeap[g_PushConstants.ResourceDescriptorIndex];
    uint layer = VR_SOURCE_LAYER_MS(texture);
    float result = texture.Load(VR_LOAD_MS_COORD(position.xy, layer), 0);
    
    [unroll] for (int i = 1; i < SAMPLE_COUNT; i++)
        result = min(result, texture.Load(VR_LOAD_MS_COORD(position.xy, layer), i));

    return result;
}
