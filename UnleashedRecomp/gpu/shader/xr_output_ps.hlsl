#include "../../../tools/XenosRecomp/XenosRecomp/shader_common.h"
#include "vr_stereo.hlsli"

// VR: the game's picture, drawn for the shape of the screen it lays its 2D out
// for, stretched over a headset image (each eye's lens is in the projection the
// picture was drawn with) and gamma-corrected like the desktop output. In
// single-pass stereo each eye's picture is a layer of the texture.

#ifdef __spirv__

#define g_Gamma vk::RawBufferLoad<float3>(g_PushConstants.SharedConstants + 0)
#define g_TextureDescriptorIndex vk::RawBufferLoad<uint>(g_PushConstants.SharedConstants + 12)
#define g_InverseOutputSize vk::RawBufferLoad<float2>(g_PushConstants.SharedConstants + 16)
#define g_Layer vk::RawBufferLoad<uint>(g_PushConstants.SharedConstants + 24)

#else

cbuffer SharedConstants : register(b2, space4)
{
    float3 g_Gamma;
    uint g_TextureDescriptorIndex;
    float2 g_InverseOutputSize;
    uint g_Layer;
};

#endif

float4 main(in float4 position : SV_Position VR_PS_EYE_INPUTS) : SV_Target
{
    VR_TEXTURE_2D(float4) texture = g_Texture2DDescriptorHeap[g_TextureDescriptorIndex];

    float4 color = texture.SampleLevel(g_SamplerDescriptorHeap[0], VR_SAMPLE_COORD(position.xy * g_InverseOutputSize, g_Layer), 0);
    color.rgb = pow(color.rgb, g_Gamma);
    return color;
}
