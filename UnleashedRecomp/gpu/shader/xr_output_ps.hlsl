#include "../../../tools/XenosRecomp/XenosRecomp/shader_common.h"

// VR: the game's picture, drawn for the shape of the screen it lays its 2D out
// for, stretched over a headset image (each eye's lens is in the projection the
// picture was drawn with) and gamma-corrected like the desktop output.

#ifdef __spirv__

#define g_Gamma vk::RawBufferLoad<float3>(g_PushConstants.SharedConstants + 0)
#define g_TextureDescriptorIndex vk::RawBufferLoad<uint>(g_PushConstants.SharedConstants + 12)
#define g_InverseOutputSize vk::RawBufferLoad<float2>(g_PushConstants.SharedConstants + 16)

#else

cbuffer SharedConstants : register(b2, space4)
{
    float3 g_Gamma;
    uint g_TextureDescriptorIndex;
    float2 g_InverseOutputSize;
};

#endif

float4 main(in float4 position : SV_Position) : SV_Target
{
    Texture2D<float4> texture = g_Texture2DDescriptorHeap[g_TextureDescriptorIndex];

    float4 color = texture.SampleLevel(g_SamplerDescriptorHeap[0], position.xy * g_InverseOutputSize, 0);
    color.rgb = pow(color.rgb, g_Gamma);
    return color;
}
