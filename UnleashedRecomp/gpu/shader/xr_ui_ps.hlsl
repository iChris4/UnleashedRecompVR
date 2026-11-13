#include "../../../tools/XenosRecomp/XenosRecomp/shader_common.h"
#include "vr_stereo.hlsli"

// VR: samples the UI texture, whose colours are premultiplied by alpha (ImGui
// drew into it over a transparent clear), for blending with ONE, INV_SRC_ALPHA.

#ifdef __spirv__

#define g_TextureDescriptorIndex vk::RawBufferLoad<uint>(g_PushConstants.SharedConstants + 128)

#else

cbuffer SharedConstants : register(b2, space4)
{
    uint g_TextureDescriptorIndex : packoffset(c8.x);
};

#endif

float4 main(in float4 position : SV_Position VR_PS_EYE_INPUTS, in float2 texCoord : TEXCOORD) : SV_Target
{
    VR_TEXTURE_2D(float4) texture = g_Texture2DDescriptorHeap[g_TextureDescriptorIndex];
    return texture.SampleLevel(g_SamplerDescriptorHeap[0], VR_SAMPLE_COORD(texCoord, 0), 0);
}
