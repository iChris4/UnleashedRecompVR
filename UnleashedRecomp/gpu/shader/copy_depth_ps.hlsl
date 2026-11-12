#include "copy_common.hlsli"
#include "vr_stereo.hlsli"

VR_TEXTURE_2D(float) g_Texture2DDescriptorHeap[] : register(t0, space0);

float main(in float4 position : SV_Position VR_PS_EYE_INPUTS) : SV_Depth
{
    return g_Texture2DDescriptorHeap[g_PushConstants.ResourceDescriptorIndex].Load(VR_LOAD_COORD(position.xy, VR_PS_EYE));
}
