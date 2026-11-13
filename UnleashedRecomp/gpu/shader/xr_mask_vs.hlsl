#include "../../../tools/XenosRecomp/XenosRecomp/shader_common.h"

// VR: the part of an eye's image the headset's lens hides (the runtime's hidden
// area mesh), drawn into the scene's depth buffer at the nearest depth right
// after the game clears it, so that nothing behind it is shaded. The vertices
// are in the constants, in normalised device coordinates, two per register.

#ifdef __spirv__

#define g_Depth vk::RawBufferLoad<float>(g_PushConstants.SharedConstants + 0)
#define g_Layer vk::RawBufferLoad<uint>(g_PushConstants.SharedConstants + 4)
#define g_Vertex(INDEX) vk::RawBufferLoad<float2>(g_PushConstants.SharedConstants + 16 + (INDEX) * 8)

#else

cbuffer SharedConstants : register(b2, space4)
{
    float g_Depth : packoffset(c0.x);
    uint g_Layer : packoffset(c0.y);
    float4 g_Vertices[4095] : packoffset(c1);
};

#define g_Vertex(INDEX) ((((INDEX) & 1) != 0) ? g_Vertices[(INDEX) / 2].zw : g_Vertices[(INDEX) / 2].xy)

#endif

void main(
    in uint vertexId : SV_VertexID,
    out float4 position : SV_Position
#ifdef VR_STEREO
    , out uint layer : SV_RenderTargetArrayIndex
    , out uint viewport : SV_ViewportArrayIndex
#endif
    )
{
    position = float4(g_Vertex(vertexId), g_Depth, 1.0);

#ifdef VR_STEREO
    layer = g_Layer;
    viewport = 0;
#endif
}
