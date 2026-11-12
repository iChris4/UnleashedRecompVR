#include "movie_common.hlsli"

void main(in VertexShaderInput In VR_VS_INSTANCE_INPUT, out Interpolators Out VR_VS_SPIRV_LAYER_OUTPUTS)
{
    VR_GAME_VS_EYE_STRUCT(Out);
    Out.ProjPos = In.ObjPos;
    Out.ProjPos.xy += g_HalfPixelOffset * Out.ProjPos.w;
    Out.UV = In.UV;
}
