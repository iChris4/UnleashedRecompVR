#include "../../../tools/XenosRecomp/XenosRecomp/shader_common.h"
#include "vr_stereo.hlsli"

// Scales a texture into a destination rectangle (in pixels), black outside it.
// Used to mirror the headset's screen image into the desktop window.

#ifdef __spirv__

#define g_TextureDescriptorIndex vk::RawBufferLoad<uint>(g_PushConstants.SharedConstants + 0)
#define g_DestinationRect vk::RawBufferLoad<float4>(g_PushConstants.SharedConstants + 16)

#else

cbuffer SharedConstants : register(b2, space4)
{
    uint g_TextureDescriptorIndex : packoffset(c0.x);
    float4 g_DestinationRect : packoffset(c1);
};

#endif

float4 main(in float4 position : SV_Position VR_PS_EYE_INPUTS) : SV_Target
{
    float2 texCoord = (position.xy - g_DestinationRect.xy) / g_DestinationRect.zw;
    if (any(texCoord < 0.0) || any(texCoord > 1.0))
        return float4(0.0, 0.0, 0.0, 1.0);

    VR_TEXTURE_2D(float4) texture = g_Texture2DDescriptorHeap[g_TextureDescriptorIndex];
    return float4(texture.SampleLevel(g_SamplerDescriptorHeap[0], VR_SAMPLE_COORD(texCoord, 0), 0).rgb, 1.0);
}
