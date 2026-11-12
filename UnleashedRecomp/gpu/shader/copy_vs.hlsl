#include "vr_stereo.hlsli"

void main(in uint vertexId : SV_VertexID VR_VS_INSTANCE_INPUT, out float4 position : SV_Position VR_VS_EYE_OUTPUT, out float2 texCoord : TEXCOORD VR_VS_LAYER_OUTPUTS)
{
    VR_HOST_VS_EYE();
    texCoord = float2((vertexId << 1) & 2, vertexId & 2);
    position = float4(texCoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
}
