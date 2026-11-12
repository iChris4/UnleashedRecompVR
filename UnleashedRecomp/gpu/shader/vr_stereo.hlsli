#pragma once

// VR single-pass stereo: the host shaders' VR_STEREO variants, used when both
// eyes are drawn at once (see shader_common.h). Every 2D view is then an array
// view, the vertex shader picks the eye, the render target layer and the
// viewport, and passes the eye to the pixel shader right after the position.
// On D3D12 the pixel shader also declares the layer and viewport after the eye,
// so that both stages pack the interpolators into the same registers.
//
// Without VR_STEREO every macro leaves the shader as it was.

#ifdef VR_STEREO

#define VR_TEXTURE_2D(TYPE) Texture2DArray<TYPE>
#define VR_TEXTURE_2D_MS(TYPE, COUNT) Texture2DMSArray<TYPE, COUNT>

#define VR_SAMPLE_COORD(texCoord, layer) float3(texCoord, layer)
#define VR_LOAD_COORD(position, layer) int4(position, layer, 0)
#define VR_LOAD_MS_COORD(position, layer) int3(position, layer)
#define VR_GET_DIMENSIONS(texture, width, height) { uint vrLayerCount; texture.GetDimensions(width, height, vrLayerCount); }

// Parameter list pieces, each starting with a comma: after the vertex shader's
// last input, after its position output, after its last output, and after the
// pixel shader's position input.
#define VR_VS_INSTANCE_INPUT , in uint iStereoInstanceId : SV_InstanceID
#define VR_VS_EYE_OUTPUT , out nointerpolation uint oEye : EYE
#define VR_VS_LAYER_OUTPUTS , out uint oLayer : SV_RenderTargetArrayIndex, out uint oViewport : SV_ViewportArrayIndex

#ifdef __spirv__
#define VR_PS_EYE_INPUTS , in nointerpolation uint iEye : EYE
#else
#define VR_PS_EYE_INPUTS , in nointerpolation uint iEye : EYE, in uint iStereoLayer : SV_RenderTargetArrayIndex, in uint iStereoViewport : SV_ViewportArrayIndex
#endif

#define VR_PS_EYE iEye

// Draws the host issues itself: two instances draw both eyes into a two-layer
// target, one instance the left eye into layer 0.
#define VR_HOST_VS_EYE() uint eye = iStereoInstanceId & 1; oEye = eye; oLayer = eye; oViewport = eye

// Draws the game issues (CSD, movies): like the recompiled shaders, from
// g_VREyeMode (shader_common.h).
#define VR_GAME_VS_EYE() uint eye = VR_STEREO_EYE(iStereoInstanceId); oEye = eye; oLayer = VR_STEREO_LAYER(eye); oViewport = eye

// The same for interpolator structures shared by both stages: the eye after the
// position, and on D3D12 the layer and viewport. Vulkan's vertex shader writes
// those as separate outputs (VR_VS_SPIRV_LAYER_OUTPUTS after the structure).
#ifdef __spirv__
#define VR_INTERPOLATORS nointerpolation uint Eye : EYE;
#define VR_VS_SPIRV_LAYER_OUTPUTS VR_VS_LAYER_OUTPUTS
#define VR_VS_SET_LAYER(interpolators, layer, viewport) oLayer = (layer); oViewport = (viewport)
#else
#define VR_INTERPOLATORS nointerpolation uint Eye : EYE; uint Layer : SV_RenderTargetArrayIndex; uint Viewport : SV_ViewportArrayIndex;
#define VR_VS_SPIRV_LAYER_OUTPUTS
#define VR_VS_SET_LAYER(interpolators, layer, viewport) interpolators.Layer = (layer); interpolators.Viewport = (viewport)
#endif

#define VR_HOST_VS_EYE_STRUCT(interpolators) uint eye = iStereoInstanceId & 1; interpolators.Eye = eye; VR_VS_SET_LAYER(interpolators, eye, eye)
#define VR_GAME_VS_EYE_STRUCT(interpolators) uint eye = VR_STEREO_EYE(iStereoInstanceId); interpolators.Eye = eye; VR_VS_SET_LAYER(interpolators, VR_STEREO_LAYER(eye), eye)

#else

#define VR_TEXTURE_2D(TYPE) Texture2D<TYPE>
#define VR_TEXTURE_2D_MS(TYPE, COUNT) Texture2DMS<TYPE, COUNT>

#define VR_SAMPLE_COORD(texCoord, layer) (texCoord)
#define VR_LOAD_COORD(position, layer) int3(position, 0)
#define VR_LOAD_MS_COORD(position, layer) int2(position)
#define VR_GET_DIMENSIONS(texture, width, height) texture.GetDimensions(width, height)

#define VR_VS_INSTANCE_INPUT
#define VR_VS_EYE_OUTPUT
#define VR_VS_LAYER_OUTPUTS
#define VR_PS_EYE_INPUTS
#define VR_PS_EYE 0

#define VR_HOST_VS_EYE()
#define VR_GAME_VS_EYE()

#define VR_INTERPOLATORS
#define VR_VS_SPIRV_LAYER_OUTPUTS
#define VR_HOST_VS_EYE_STRUCT(interpolators)
#define VR_GAME_VS_EYE_STRUCT(interpolators)

#endif
