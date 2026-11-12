#include "../../../tools/XenosRecomp/XenosRecomp/shader_common.h"
#include "vr_stereo.hlsli"

// VR: the UI texture as a quad on the HUD panel, for one eye. The panel
// transform takes the canvas in normalised device coordinates to clip space:
// clip = [x, y, 0, 1] * g_PanelMatrix (row vectors).

#ifdef __spirv__

#define g_PanelRow(INDEX) vk::RawBufferLoad<float4>(g_PushConstants.SharedConstants + (INDEX) * 16, 0x10)

#else

cbuffer SharedConstants : register(b2, space4)
{
    float4 g_PanelMatrix[4] : packoffset(c0);
    uint g_TextureDescriptorIndex : packoffset(c4.x);
};

#define g_PanelRow(INDEX) g_PanelMatrix[INDEX]

#endif

void main(in uint vertexId : SV_VertexID VR_VS_INSTANCE_INPUT, out float4 position : SV_Position VR_VS_EYE_OUTPUT, out float2 texCoord : TEXCOORD VR_VS_LAYER_OUTPUTS)
{
    VR_HOST_VS_EYE();

    const float2 corners[6] =
    {
        float2(0.0, 0.0), float2(1.0, 0.0), float2(0.0, 1.0),
        float2(0.0, 1.0), float2(1.0, 0.0), float2(1.0, 1.0)
    };

    texCoord = corners[vertexId];

    float2 ndc = float2(texCoord.x * 2.0 - 1.0, 1.0 - texCoord.y * 2.0);
    position = ndc.x * g_PanelRow(0) + ndc.y * g_PanelRow(1) + g_PanelRow(3);
    position.z = 0.0;
}
