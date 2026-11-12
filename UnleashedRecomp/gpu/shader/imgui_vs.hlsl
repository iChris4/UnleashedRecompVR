#include "imgui_common.hlsli"

void main(in float2 position : POSITION, in float2 uv : TEXCOORD, in float4 color : COLOR VR_VS_INSTANCE_INPUT, out Interpolators interpolators VR_VS_SPIRV_LAYER_OUTPUTS)
{    
    if (g_PushConstants.ShaderModifier == IMGUI_SHADER_MODIFIER_TEXT_SKEW)
    {
        if (position.y < g_PushConstants.Origin.y)
            position.x += g_PushConstants.Scale.x;
    }
    else if (g_PushConstants.ShaderModifier != IMGUI_SHADER_MODIFIER_HORIZONTAL_MARQUEE_FADE && 
        g_PushConstants.ShaderModifier != IMGUI_SHADER_MODIFIER_VERTICAL_MARQUEE_FADE)
    {
        position.xy = g_PushConstants.Origin + (position.xy - g_PushConstants.Origin) * g_PushConstants.Scale;
    }
    
    interpolators.Position = float4(position.xy * g_PushConstants.InverseDisplaySize * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    interpolators.UV = uv;
    interpolators.Color = color;
    VR_HOST_VS_EYE_STRUCT(interpolators);
}
