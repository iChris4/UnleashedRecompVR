#include "video.h"

#include "imgui/imgui_common.h"
#include "imgui/imgui_snapshot.h"
#include "imgui/imgui_font_builder.h"

#include <app.h>
#include <bc_diff.h>
#include <cpu/guest_thread.h>
#include <decompressor.h>
#include <kernel/function.h>
#include <kernel/heap.h>
#include <hid/hid.h>
#include <kernel/memory.h>
#include <kernel/xdbf.h>
#include <res/bc_diff/button_bc_diff.bin.h>
#include <res/font/im_font_atlas.dds.h>
#include <shader/shader_cache.h>
#include <SWA.h>
#include <ui/achievement_menu.h>
#include <ui/achievement_overlay.h>
#include <ui/button_guide.h>
#include <ui/fader.h>
#include <ui/imgui_utils.h>
#include <ui/installer_wizard.h>
#include <ui/message_window.h>
#include <ui/options_menu.h>
#include <ui/game_window.h>
#include <ui/black_bar.h>
#include <patches/aspect_ratio_patches.h>
#include <patches/inspire_patches.h>
#include <user/config.h>
#include <sdl_listener.h>
#include <xxHashMap.h>
#include <os/logger.h>
#include <os/process.h>
#include <xr/xr.h>
#include <xr/xr_math.h>
#include <bit>

#if defined(ASYNC_PSO_DEBUG) || defined(PSO_CACHING)
#include <magic_enum/magic_enum.hpp>
#endif

#define UNLEASHED_RECOMP
#include "../../tools/XenosRecomp/XenosRecomp/shader_common.h"

#ifdef UNLEASHED_RECOMP_D3D12
#include "shader/blend_color_alpha_ps.hlsl.dxil.h"
#include "shader/blend_color_alpha_ps.hlsl.stereo.dxil.h"
#include "shader/copy_vs.hlsl.dxil.h"
#include "shader/copy_vs.hlsl.stereo.dxil.h"
#include "shader/copy_color_ps.hlsl.dxil.h"
#include "shader/copy_color_ps.hlsl.stereo.dxil.h"
#include "shader/copy_depth_ps.hlsl.dxil.h"
#include "shader/copy_depth_ps.hlsl.stereo.dxil.h"
#include "shader/csd_filter_ps.hlsl.dxil.h"
#include "shader/csd_filter_ps.hlsl.stereo.dxil.h"
#include "shader/csd_no_tex_vs.hlsl.dxil.h"
#include "shader/csd_no_tex_vs.hlsl.stereo.dxil.h"
#include "shader/csd_vs.hlsl.dxil.h"
#include "shader/csd_vs.hlsl.stereo.dxil.h"
#include "shader/enhanced_motion_blur_ps.hlsl.dxil.h"
#include "shader/enhanced_motion_blur_ps.hlsl.stereo.dxil.h"
#include "shader/gamma_correction_ps.hlsl.dxil.h"
#include "shader/gamma_correction_ps.hlsl.stereo.dxil.h"
#include "shader/gaussian_blur_3x3.hlsl.dxil.h"
#include "shader/gaussian_blur_3x3.hlsl.stereo.dxil.h"
#include "shader/gaussian_blur_5x5.hlsl.dxil.h"
#include "shader/gaussian_blur_5x5.hlsl.stereo.dxil.h"
#include "shader/gaussian_blur_7x7.hlsl.dxil.h"
#include "shader/gaussian_blur_7x7.hlsl.stereo.dxil.h"
#include "shader/gaussian_blur_9x9.hlsl.dxil.h"
#include "shader/gaussian_blur_9x9.hlsl.stereo.dxil.h"
#include "shader/imgui_ps.hlsl.dxil.h"
#include "shader/imgui_ps.hlsl.stereo.dxil.h"
#include "shader/imgui_vs.hlsl.dxil.h"
#include "shader/imgui_vs.hlsl.stereo.dxil.h"
#include "shader/movie_ps.hlsl.dxil.h"
#include "shader/movie_ps.hlsl.stereo.dxil.h"
#include "shader/movie_vs.hlsl.dxil.h"
#include "shader/movie_vs.hlsl.stereo.dxil.h"
#include "shader/resolve_msaa_color_2x.hlsl.dxil.h"
#include "shader/resolve_msaa_color_2x.hlsl.stereo.dxil.h"
#include "shader/resolve_msaa_color_4x.hlsl.dxil.h"
#include "shader/resolve_msaa_color_4x.hlsl.stereo.dxil.h"
#include "shader/resolve_msaa_color_8x.hlsl.dxil.h"
#include "shader/resolve_msaa_color_8x.hlsl.stereo.dxil.h"
#include "shader/resolve_msaa_depth_2x.hlsl.dxil.h"
#include "shader/resolve_msaa_depth_2x.hlsl.stereo.dxil.h"
#include "shader/resolve_msaa_depth_4x.hlsl.dxil.h"
#include "shader/resolve_msaa_depth_4x.hlsl.stereo.dxil.h"
#include "shader/resolve_msaa_depth_8x.hlsl.dxil.h"
#include "shader/resolve_msaa_depth_8x.hlsl.stereo.dxil.h"
#include "shader/xr_blit_ps.hlsl.dxil.h"
#include "shader/xr_blit_ps.hlsl.stereo.dxil.h"
#include "shader/xr_output_ps.hlsl.dxil.h"
#include "shader/xr_output_ps.hlsl.stereo.dxil.h"
#include "shader/xr_ui_ps.hlsl.dxil.h"
#include "shader/xr_ui_ps.hlsl.stereo.dxil.h"
#include "shader/xr_ui_vs.hlsl.dxil.h"
#include "shader/xr_ui_vs.hlsl.stereo.dxil.h"
#include "shader/xr_mask_vs.hlsl.dxil.h"
#include "shader/xr_mask_vs.hlsl.stereo.dxil.h"
#endif

#include "shader/blend_color_alpha_ps.hlsl.spirv.h"
#include "shader/blend_color_alpha_ps.hlsl.stereo.spirv.h"
#include "shader/copy_vs.hlsl.spirv.h"
#include "shader/copy_vs.hlsl.stereo.spirv.h"
#include "shader/copy_color_ps.hlsl.spirv.h"
#include "shader/copy_color_ps.hlsl.stereo.spirv.h"
#include "shader/copy_depth_ps.hlsl.spirv.h"
#include "shader/copy_depth_ps.hlsl.stereo.spirv.h"
#include "shader/csd_filter_ps.hlsl.spirv.h"
#include "shader/csd_filter_ps.hlsl.stereo.spirv.h"
#include "shader/csd_no_tex_vs.hlsl.spirv.h"
#include "shader/csd_no_tex_vs.hlsl.stereo.spirv.h"
#include "shader/csd_vs.hlsl.spirv.h"
#include "shader/csd_vs.hlsl.stereo.spirv.h"
#include "shader/enhanced_motion_blur_ps.hlsl.spirv.h"
#include "shader/enhanced_motion_blur_ps.hlsl.stereo.spirv.h"
#include "shader/gamma_correction_ps.hlsl.spirv.h"
#include "shader/gamma_correction_ps.hlsl.stereo.spirv.h"
#include "shader/gaussian_blur_3x3.hlsl.spirv.h"
#include "shader/gaussian_blur_3x3.hlsl.stereo.spirv.h"
#include "shader/gaussian_blur_5x5.hlsl.spirv.h"
#include "shader/gaussian_blur_5x5.hlsl.stereo.spirv.h"
#include "shader/gaussian_blur_7x7.hlsl.spirv.h"
#include "shader/gaussian_blur_7x7.hlsl.stereo.spirv.h"
#include "shader/gaussian_blur_9x9.hlsl.spirv.h"
#include "shader/gaussian_blur_9x9.hlsl.stereo.spirv.h"
#include "shader/imgui_ps.hlsl.spirv.h"
#include "shader/imgui_ps.hlsl.stereo.spirv.h"
#include "shader/imgui_vs.hlsl.spirv.h"
#include "shader/imgui_vs.hlsl.stereo.spirv.h"
#include "shader/movie_ps.hlsl.spirv.h"
#include "shader/movie_ps.hlsl.stereo.spirv.h"
#include "shader/movie_vs.hlsl.spirv.h"
#include "shader/movie_vs.hlsl.stereo.spirv.h"
#include "shader/resolve_msaa_color_2x.hlsl.spirv.h"
#include "shader/resolve_msaa_color_2x.hlsl.stereo.spirv.h"
#include "shader/resolve_msaa_color_4x.hlsl.spirv.h"
#include "shader/resolve_msaa_color_4x.hlsl.stereo.spirv.h"
#include "shader/resolve_msaa_color_8x.hlsl.spirv.h"
#include "shader/resolve_msaa_color_8x.hlsl.stereo.spirv.h"
#include "shader/resolve_msaa_depth_2x.hlsl.spirv.h"
#include "shader/resolve_msaa_depth_2x.hlsl.stereo.spirv.h"
#include "shader/resolve_msaa_depth_4x.hlsl.spirv.h"
#include "shader/resolve_msaa_depth_4x.hlsl.stereo.spirv.h"
#include "shader/resolve_msaa_depth_8x.hlsl.spirv.h"
#include "shader/resolve_msaa_depth_8x.hlsl.stereo.spirv.h"
#include "shader/xr_blit_ps.hlsl.spirv.h"
#include "shader/xr_blit_ps.hlsl.stereo.spirv.h"
#include "shader/xr_output_ps.hlsl.spirv.h"
#include "shader/xr_output_ps.hlsl.stereo.spirv.h"
#include "shader/xr_ui_ps.hlsl.spirv.h"
#include "shader/xr_ui_ps.hlsl.stereo.spirv.h"
#include "shader/xr_ui_vs.hlsl.spirv.h"
#include "shader/xr_ui_vs.hlsl.stereo.spirv.h"
#include "shader/xr_mask_vs.hlsl.spirv.h"
#include "shader/xr_mask_vs.hlsl.stereo.spirv.h"

#ifdef _WIN32
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

namespace plume
{
#ifdef UNLEASHED_RECOMP_D3D12
    extern std::unique_ptr<RenderInterface> CreateD3D12Interface();
#endif
#ifdef SDL_VULKAN_ENABLED
    extern std::unique_ptr<RenderInterface> CreateVulkanInterface(RenderWindow sdlWindow);
#else
    extern std::unique_ptr<RenderInterface> CreateVulkanInterface();
#endif

    static std::unique_ptr<RenderInterface> CreateVulkanInterfaceWrapper() {
#ifdef SDL_VULKAN_ENABLED
        return CreateVulkanInterface(GameWindow::s_renderWindow);
#else
        return CreateVulkanInterface();
#endif
    }
}

#pragma pack(push, 1)
struct PipelineState
{
    GuestShader* vertexShader = nullptr;
    GuestShader* pixelShader = nullptr;
    GuestVertexDeclaration* vertexDeclaration = nullptr;
    bool instancing = false;
    bool zEnable = true;
    bool zWriteEnable = true;
    RenderBlend srcBlend = RenderBlend::ONE;
    RenderBlend destBlend = RenderBlend::ZERO;
    RenderCullMode cullMode = RenderCullMode::NONE;
    RenderComparisonFunction zFunc = RenderComparisonFunction::LESS;
    bool alphaBlendEnable = false;
    RenderBlendOperation blendOp = RenderBlendOperation::ADD;
    float slopeScaledDepthBias = 0.0f;
    int32_t depthBias = 0;
    RenderBlend srcBlendAlpha = RenderBlend::ONE;
    RenderBlend destBlendAlpha = RenderBlend::ZERO;
    RenderBlendOperation blendOpAlpha = RenderBlendOperation::ADD;
    uint32_t colorWriteEnable = uint32_t(RenderColorWriteEnable::ALL);
    RenderPrimitiveTopology primitiveTopology = RenderPrimitiveTopology::TRIANGLE_LIST;
    uint8_t vertexStrides[16]{};
    RenderFormat renderTargetFormat{};
    RenderFormat depthStencilFormat{};
    RenderSampleCounts sampleCount = RenderSampleCount::COUNT_1;
    bool enableAlphaToCoverage = false;
    uint32_t specConstants = 0;
};
#pragma pack(pop)

struct UploadAllocation
{
    const RenderBuffer* buffer;
    uint64_t offset;
    uint8_t* memory;
    uint64_t deviceAddress;
};

struct SharedConstants
{
    uint32_t texture2DIndices[16]{};
    uint32_t texture3DIndices[16]{};
    uint32_t textureCubeIndices[16]{};
    uint32_t samplerIndices[16]{};
    uint32_t booleans{};
    uint32_t swappedTexcoords{};
    float halfPixelOffsetX{};
    float halfPixelOffsetY{};
    float alphaThreshold{};

    // VR single-pass stereo: how the draw reaches the eyes (g_VREyeMode, the
    // VR_EYE_MODE_* bits).
    uint32_t vrEyeMode{};
    uint32_t padding0[2]{};

    // VR: the HUD panel in front of the game camera, as each eye sees it: clip =
    // [x, y, 0, 1] * matrix (registers c18 and c22). The CSD (HUD) vertex shaders
    // put the HUD's canvas on it when vrHudEnabled is set, with the first matrix
    // (the eye frame replay draws). In single-pass stereo the VR_STEREO shaders
    // follow g_VREyeMode's panel bit instead, with each eye's matrix.
    float vrHudMatrix[2][16]{};
    uint32_t vrHudEnabled{};
};

static_assert(offsetof(SharedConstants, vrEyeMode) == 276);
static_assert(offsetof(SharedConstants, vrHudMatrix) == 288);
static_assert(offsetof(SharedConstants, vrHudEnabled) == 416);

// Depth bias values here are only used when the render device has 
// dynamic depth bias capability enabled. Otherwise, they get unused
// and the values get assigned in the pipeline state instead.

static GuestSurface* g_renderTarget;
static GuestSurface* g_depthStencil;
static RenderFramebuffer* g_framebuffer;
static RenderViewport g_viewport(0.0f, 0.0f, 1280.0f, 720.0f);
static PipelineState g_pipelineState;
static int32_t g_depthBias;
static float g_slopeScaledDepthBias;
static uint32_t g_vertexShaderConstants[0x400];
static uint32_t g_pixelShaderConstants[0x380];
static SharedConstants g_sharedConstants;
static GuestTexture* g_textures[16];
static RenderSamplerDesc g_samplerDescs[16];
static bool g_scissorTestEnable = false;
static RenderRect g_scissorRect;
static RenderVertexBufferView g_vertexBufferViews[16];
static RenderInputSlot g_inputSlots[16];
static RenderIndexBufferView g_indexBufferView({}, 0, RenderFormat::R16_UINT);

struct DirtyStates
{
    bool renderTargetAndDepthStencil;
    bool viewport;
    bool pipelineState;
    bool depthBias;
    bool sharedConstants;
    bool scissorRect;
    bool vertexShaderConstants;
    uint8_t vertexStreamFirst;
    uint8_t vertexStreamLast;
    bool indices;
    bool pixelShaderConstants;

    DirtyStates(bool value)
        : renderTargetAndDepthStencil(value)
        , viewport(value)
        , pipelineState(value)
        , depthBias(value)
        , sharedConstants(value)
        , scissorRect(value)
        , vertexShaderConstants(value)
        , vertexStreamFirst(value ? 0 : 255)
        , vertexStreamLast(value ? 15 : 0)
        , indices(value)
        , pixelShaderConstants(value)
    {
    }
};

static DirtyStates g_dirtyStates(true);

template<typename T>
static void SetDirtyValue(bool& dirtyState, T& dest, const T& src)
{
    if (dest != src)
    {
        dest = src;
        dirtyState = true;
    }
}

static constexpr size_t PROFILER_VALUE_COUNT = 256;
static size_t g_profilerValueIndex;

struct Profiler
{
    std::atomic<double> value;
    double values[PROFILER_VALUE_COUNT];
    std::chrono::steady_clock::time_point start;

    void Begin()
    {
        start = std::chrono::steady_clock::now();
    }

    void End()
    {
        value = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    }

    void Set(double v)
    {
        value = v;
    }

    void Reset()
    {
        End();
        Begin();
    }

    double UpdateAndReturnAverage()
    {
        values[g_profilerValueIndex] = value;
        return std::accumulate(values, values + PROFILER_VALUE_COUNT, 0.0) / PROFILER_VALUE_COUNT;
    }
};

static double g_applicationValues[PROFILER_VALUE_COUNT];
static Profiler g_gpuFrameProfiler;
static Profiler g_presentProfiler;
static Profiler g_updateDirectorProfiler;
static Profiler g_renderDirectorProfiler;
static Profiler g_frameFenceProfiler;
static Profiler g_presentWaitProfiler;
static Profiler g_swapChainAcquireProfiler;
static Profiler g_executeWaitProfiler;

static bool g_profilerVisible;
static bool g_profilerWasToggled;

#ifdef UNLEASHED_RECOMP_D3D12
static bool g_vulkan = false;
#else
static constexpr bool g_vulkan = true;
#endif

static bool g_triangleStripWorkaround = false;

static bool g_hardwareResolve = true;
static bool g_hardwareDepthResolve = true;

static std::unique_ptr<RenderInterface> g_interface;
static std::unique_ptr<RenderDevice> g_device;

static RenderDeviceCapabilities g_capabilities;

static constexpr size_t NUM_FRAMES = 2;
static constexpr size_t NUM_QUERIES = 2;

static uint32_t g_frame = 0;
static uint32_t g_nextFrame = 1;

static std::unique_ptr<RenderCommandQueue> g_queue;
static std::unique_ptr<RenderCommandList> g_commandLists[NUM_FRAMES];
static std::unique_ptr<RenderCommandFence> g_commandFences[NUM_FRAMES];
static std::unique_ptr<RenderQueryPool> g_queryPools[NUM_FRAMES];

// VR: where the GPU's time goes in a headset frame. The render thread writes a
// timestamp whenever the kind of work changes (a segment); the game thread
// reads them back with the frame's own and adds the times up by kind, for the
// VR log. A frame with more changes than queries leaves the rest to its last
// segment.
enum GpuSegmentKind : uint8_t
{
    // Square targets: the shadow maps (and the small luminance chain).
    GPU_SEGMENT_SHADOWS,

    // Draws with a depth test: the world.
    GPU_SEGMENT_SCENE,

    // Draws without one: post-processing, 2D, and the copies between them.
    GPU_SEGMENT_FLAT,

    // The headset's images and the window.
    GPU_SEGMENT_OUTPUT,

    GPU_SEGMENT_KIND_COUNT
};

static constexpr uint32_t NUM_SEGMENT_QUERIES = 128;
static constexpr uint32_t GPU_SEGMENTS_UNUSED = UINT32_MAX;

static std::unique_ptr<RenderQueryPool> g_segmentQueryPools[NUM_FRAMES];
static uint8_t g_gpuSegmentKinds[NUM_FRAMES][NUM_SEGMENT_QUERIES];
static uint32_t g_gpuSegmentCounts[NUM_FRAMES] = { GPU_SEGMENTS_UNUSED, GPU_SEGMENTS_UNUSED };
static int32_t g_gpuSegmentKind = -1;

static std::mutex g_gpuSegmentMutex;
static double g_gpuSegmentMs[GPU_SEGMENT_KIND_COUNT];
static uint32_t g_gpuSegmentFrames;
static uint64_t g_gpuSegmentTotal;

// [VR] VRPerformanceTest: 1 turns the hidden area mask, 2 foveated rendering, on
// and off every other second; the frames' GPU segments are then added up apart
// (state 1 on, 0 off, -1 no test) for the VR log to compare.
static int8_t g_gpuSegmentTestStates[NUM_FRAMES] = { -1, -1 };
static double g_gpuSegmentTestMs[2][GPU_SEGMENT_KIND_COUNT];
static uint32_t g_gpuSegmentTestFrames[2];

static void BeginGpuSegment(GpuSegmentKind kind);
static bool g_commandListStates[NUM_FRAMES];

static Mutex g_copyMutex;
static std::unique_ptr<RenderCommandQueue> g_copyQueue;
static std::unique_ptr<RenderCommandList> g_copyCommandList;
static std::unique_ptr<RenderCommandFence> g_copyCommandFence;

static std::unique_ptr<RenderSwapChain> g_swapChain;
static bool g_swapChainValid;

static constexpr RenderFormat BACKBUFFER_FORMAT = RenderFormat::B8G8R8A8_UNORM;

static std::unique_ptr<RenderCommandSemaphore> g_acquireSemaphores[NUM_FRAMES];
static std::unique_ptr<RenderCommandSemaphore> g_renderSemaphores[NUM_FRAMES];
static uint32_t g_backBufferIndex;
static std::unique_ptr<GuestSurface> g_backBufferHolder;
static GuestSurface* g_backBuffer;

static std::unique_ptr<RenderTexture> g_intermediaryBackBufferTexture;
static std::unique_ptr<RenderTextureView> g_intermediaryBackBufferTextureView;
static uint32_t g_intermediaryBackBufferTextureWidth;
static uint32_t g_intermediaryBackBufferTextureHeight;
static uint32_t g_intermediaryBackBufferTextureDescriptorIndex;

static std::unique_ptr<RenderPipeline> g_gammaCorrectionPipeline;

// VR: a gamma-corrected picture (the screen, or one eye), in the format the
// OpenXR swapchains can be copied from. Copied into the headset's image and
// scaled into the window.
static std::unique_ptr<RenderTexture> g_vrOutputTexture;
static std::unique_ptr<RenderTextureView> g_vrOutputTextureView;
static std::unique_ptr<RenderFramebuffer> g_vrOutputFramebuffer;
static uint32_t g_vrOutputTextureWidth;
static uint32_t g_vrOutputTextureHeight;
static uint32_t g_vrOutputTextureDescriptorIndex;
static std::unique_ptr<RenderPipeline> g_vrOutputPipeline;
static std::unique_ptr<RenderPipeline> g_vrMirrorPipeline;

// The headset images acquired for the frame being presented. Written by the
// presenting thread before it enqueues ExecuteCommandList, read by the render thread.
static VRImages g_vrImages;

// How the render thread drew the frame for the headset. Written by the render
// thread before it signals g_executedCommandList, read by the presenting thread after.
static VRFrameResult g_vrFrameResult;

struct std::unique_ptr<RenderDescriptorSet> g_textureDescriptorSet;
struct std::unique_ptr<RenderDescriptorSet> g_samplerDescriptorSet;

enum
{
    TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D,
    TEXTURE_DESCRIPTOR_NULL_TEXTURE_3D,
    TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE,
    TEXTURE_DESCRIPTOR_NULL_COUNT
};

struct TextureDescriptorAllocator
{
    Mutex mutex;
    uint32_t capacity = TEXTURE_DESCRIPTOR_NULL_COUNT;
    std::vector<uint32_t> freed;

    uint32_t allocate()
    {
        std::lock_guard lock(mutex);

        uint32_t value;
        if (!freed.empty())
        {
            value = freed.back();
            freed.pop_back();
        }
        else
        {
            value = capacity;
            ++capacity;
        }

        return value;
    }

    void free(uint32_t value)
    {
        assert(value != NULL);
        std::lock_guard lock(mutex);
        freed.push_back(value);
    }
};

static std::unique_ptr<RenderTexture> g_blankTextures[TEXTURE_DESCRIPTOR_NULL_COUNT];
static std::unique_ptr<RenderTextureView> g_blankTextureViews[TEXTURE_DESCRIPTOR_NULL_COUNT];

static TextureDescriptorAllocator g_textureDescriptorAllocator;

static std::unique_ptr<RenderPipelineLayout> g_pipelineLayout;
static xxHashMap<std::unique_ptr<RenderPipeline>> g_pipelines;

#ifdef ASYNC_PSO_DEBUG
static std::atomic<uint32_t> g_pipelinesCreatedInRenderThread;
static std::atomic<uint32_t> g_pipelinesCreatedAsynchronously;
static std::atomic<uint32_t> g_pipelinesDropped;
static std::atomic<uint32_t> g_pipelinesCurrentlyCompiling;
static std::string g_pipelineDebugText;
static Mutex g_debugMutex;
#endif

#ifdef PSO_CACHING
static xxHashMap<PipelineState> g_pipelineStatesToCache;
static Mutex g_pipelineCacheMutex;
#endif

static std::atomic<uint32_t> g_compilingPipelineTaskCount;
static std::atomic<uint32_t> g_pendingPipelineTaskCount;

enum class PipelineTaskType
{
    Null,
    DatabaseData,
    PrecompilePipelines,
    RecompilePipelines
};

struct PipelineTask
{
    PipelineTaskType type{};
    boost::shared_ptr<Hedgehog::Database::CDatabaseData> databaseData;
};

static Mutex g_pipelineTaskMutex;
static std::vector<PipelineTask> g_pipelineTaskQueue;

static void EnqueuePipelineTask(PipelineTaskType type, const boost::shared_ptr<Hedgehog::Database::CDatabaseData>& databaseData)
{
    // Precompiled pipelines deliberately do not increment 
    // this counter to overlap the compilation with intro logos.
    if (type != PipelineTaskType::PrecompilePipelines)
        ++g_compilingPipelineTaskCount;

    {
        std::lock_guard lock(g_pipelineTaskMutex);
        g_pipelineTaskQueue.emplace_back(type, databaseData);
    }

    if ((++g_pendingPipelineTaskCount) == 1)
        g_pendingPipelineTaskCount.notify_one();
}

static const PipelineState g_pipelineStateCache[] =
{
#include "cache/pipeline_state_cache.h"
};

#include "cache/vertex_element_cache.h"

static uint8_t* const g_vertexDeclarationCache[] =
{
#include "cache/vertex_declaration_cache.h"
};

static xxHashMap<std::pair<uint32_t, std::unique_ptr<RenderSampler>>> g_samplerStates;

static Mutex g_vertexDeclarationMutex;
static xxHashMap<GuestVertexDeclaration*> g_vertexDeclarations;

struct UploadBuffer
{
    static constexpr size_t SIZE = 16 * 1024 * 1024;

    std::unique_ptr<RenderBuffer> buffer;
    uint8_t* memory = nullptr;
    uint64_t deviceAddress = 0;
};

struct UploadAllocator
{
    std::vector<UploadBuffer> buffers;
    uint32_t index = 0;
    uint32_t offset = 0;

    UploadAllocation allocate(uint32_t size, uint32_t alignment)
    {
        assert(size <= UploadBuffer::SIZE);

        offset = (offset + alignment - 1) & ~(alignment - 1);

        if (offset + size > UploadBuffer::SIZE)
        {
            ++index;
            offset = 0;
        }

        if (buffers.size() <= index)
            buffers.resize(index + 1);

        auto& buffer = buffers[index];
        if (buffer.buffer == nullptr)
        {
            buffer.buffer = g_device->createBuffer(RenderBufferDesc::UploadBuffer(UploadBuffer::SIZE, RenderBufferFlag::CONSTANT | RenderBufferFlag::VERTEX | RenderBufferFlag::INDEX));
            buffer.memory = reinterpret_cast<uint8_t*>(buffer.buffer->map());
            buffer.deviceAddress = buffer.buffer->getDeviceAddress();
        }
        
        auto ref = buffer.buffer->at(offset);
        offset += size;

        return { ref.ref, ref.offset, buffer.memory + ref.offset, buffer.deviceAddress + ref.offset };
    }

    template<bool TByteSwap, typename T>
    UploadAllocation allocate(const T* memory, uint32_t size, uint32_t alignment)
    {
        auto result = allocate(size, alignment);

        if constexpr (TByteSwap)
        {
            auto destination = reinterpret_cast<T*>(result.memory);

            for (size_t i = 0; i < size; i += sizeof(T))
            {
                *destination = ByteSwap(*memory);
                ++destination;
                ++memory;
            }
        }
        else
        {
            memcpy(result.memory, memory, size);
        }

        return result;
    }

    void reset()
    {
        index = 0;
        offset = 0;
    }
};

static UploadAllocator g_uploadAllocators[NUM_FRAMES];

struct IntermediaryUploadAllocator
{
    static constexpr size_t SIZE = 16 * 1024 * 1024;

    std::vector<std::unique_ptr<uint8_t[]>> buffers;
    uint32_t index = 0;
    uint32_t offset = 0;

    uint8_t* allocate(uint32_t size)
    {
        assert(size <= SIZE);

        if (offset + size > SIZE)
        {
            ++index;
            offset = 0;
        }

        if (buffers.size() <= index)
            buffers.resize(index + 1);

        auto& buffer = buffers[index];
        if (buffer == nullptr)
            buffer = std::make_unique_for_overwrite<uint8_t[]>(SIZE);

        auto result = buffer.get() + offset;
        offset += ((size + 0xF) & ~0xF);

        return result;
    }

    uint8_t* allocate(const void* memory, uint32_t size)
    {
        auto result = allocate(size);
        memcpy(result, memory, size);
        return result;
    }

    void reset()
    {
        index = 0;
        offset = 0;
    }
};

static IntermediaryUploadAllocator g_intermediaryUploadAllocator;

static std::vector<GuestResource*> g_tempResources[NUM_FRAMES];
static std::vector<std::unique_ptr<RenderBuffer>> g_tempBuffers[NUM_FRAMES];

template<GuestPrimitiveType PrimitiveType>
struct PrimitiveIndexData
{
    std::vector<uint16_t> indexData;
    RenderBufferReference indexBuffer;
    uint32_t currentIndexCount = 0;

    uint32_t prepare(uint32_t guestPrimCount)
    {
        uint32_t primCount;
        uint32_t indexCountPerPrimitive;

        switch (PrimitiveType)
        {
        case D3DPT_TRIANGLEFAN:
            primCount = guestPrimCount - 2;
            indexCountPerPrimitive = 3; 
            break;
        case D3DPT_QUADLIST:
            primCount = guestPrimCount / 4;
            indexCountPerPrimitive = 6;
            break;
        default:
            assert(false && "Unknown primitive type.");
            break;
        }

        uint32_t indexCount = primCount * indexCountPerPrimitive;

        if (indexData.size() < indexCount)
        {
            const size_t oldPrimCount = indexData.size() / indexCountPerPrimitive;
            indexData.resize(indexCount);

            for (size_t i = oldPrimCount; i < primCount; i++)
            {
                switch (PrimitiveType)
                {
                case D3DPT_TRIANGLEFAN:
                {
                    indexData[i * 3 + 0] = 0;
                    indexData[i * 3 + 1] = static_cast<uint16_t>(i + 1);
                    indexData[i * 3 + 2] = static_cast<uint16_t>(i + 2);
                    break;
                }
                case D3DPT_QUADLIST:
                {
                    indexData[i * 6 + 0] = static_cast<uint16_t>(i * 4 + 0);
                    indexData[i * 6 + 1] = static_cast<uint16_t>(i * 4 + 1);
                    indexData[i * 6 + 2] = static_cast<uint16_t>(i * 4 + 2);

                    indexData[i * 6 + 3] = static_cast<uint16_t>(i * 4 + 0);
                    indexData[i * 6 + 4] = static_cast<uint16_t>(i * 4 + 2);
                    indexData[i * 6 + 5] = static_cast<uint16_t>(i * 4 + 3);
                    break;
                }
                default:
                    assert(false && "Unknown primitive type.");
                    break;
                }
            }
        }

        if (indexBuffer == NULL || currentIndexCount < indexCount)
        {
            auto allocation = g_uploadAllocators[g_frame].allocate<false>(indexData.data(), indexCount * 2, 2);
            indexBuffer = allocation.buffer->at(allocation.offset);
            currentIndexCount = indexCount;
        }

        SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.buffer, indexBuffer);
        SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.size, indexCount * 2);
        SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.format, RenderFormat::R16_UINT);

        return indexCount;
    }

    void reset()
    {
        indexBuffer = {};
        currentIndexCount = 0;
    }
};

static PrimitiveIndexData<D3DPT_TRIANGLEFAN> g_triangleFanIndexData;
static PrimitiveIndexData<D3DPT_QUADLIST> g_quadIndexData;

static void DestructTempResources()
{
    for (auto resource : g_tempResources[g_frame])
    {
        switch (resource->type)
        {
        case ResourceType::Texture:
        case ResourceType::VolumeTexture:
        {
            const auto texture = reinterpret_cast<GuestTexture*>(resource);

            if (texture->mappedMemory != nullptr)
                g_userHeap.Free(texture->mappedMemory);

            g_textureDescriptorAllocator.free(texture->descriptorIndex);

            if (texture->patchedTexture != nullptr)
                g_textureDescriptorAllocator.free(texture->patchedTexture->descriptorIndex); 
            
            if (texture->recreatedCubeMapTexture != nullptr)
                g_textureDescriptorAllocator.free(texture->recreatedCubeMapTexture->descriptorIndex);

            texture->~GuestTexture();
            break;
        }

        case ResourceType::VertexBuffer:
        case ResourceType::IndexBuffer:
        {
            const auto buffer = reinterpret_cast<GuestBuffer*>(resource);

            if (buffer->mappedMemory != nullptr)
                g_userHeap.Free(buffer->mappedMemory);

            buffer->~GuestBuffer();
            break;
        }

        case ResourceType::RenderTarget:
        case ResourceType::DepthStencil:
        {
            const auto surface = reinterpret_cast<GuestSurface*>(resource);

            if (surface->descriptorIndex != NULL)
                g_textureDescriptorAllocator.free(surface->descriptorIndex);

            surface->~GuestSurface();
            break;
        }

        case ResourceType::VertexDeclaration:
            reinterpret_cast<GuestVertexDeclaration*>(resource)->~GuestVertexDeclaration();
            break;

        case ResourceType::VertexShader:
        case ResourceType::PixelShader:
        {
            reinterpret_cast<GuestShader*>(resource)->~GuestShader();
            break;
        }
        }

        g_userHeap.Free(resource);
    }

    g_tempResources[g_frame].clear();
    g_tempBuffers[g_frame].clear();
}

static std::thread::id g_presentThreadId = std::this_thread::get_id();
static std::atomic<bool> g_readyForCommands;

PPC_FUNC_IMPL(__imp__sub_824ECA00);
PPC_FUNC(sub_824ECA00)
{
    // Guard against thread ownership changes when between command lists.
    g_readyForCommands.wait(false);
    g_presentThreadId = std::this_thread::get_id();
    __imp__sub_824ECA00(ctx, base);
}

static ankerl::unordered_dense::map<RenderTexture*, RenderTextureLayout> g_barrierMap;

static void AddBarrier(GuestBaseTexture* texture, RenderTextureLayout layout)
{
    if (texture != nullptr && texture->layout != layout)
    {
        g_barrierMap[texture->texture] = layout;
        texture->layout = layout;
    }
}

static std::vector<RenderTextureBarrier> g_barriers;

static void FlushBarriers()
{
    if (!g_barrierMap.empty())
    {
        for (auto& [texture, layout] : g_barrierMap)
            g_barriers.emplace_back(texture, layout);

        g_commandLists[g_frame]->barriers(RenderBarrierStage::GRAPHICS | RenderBarrierStage::COPY, g_barriers);

        g_barrierMap.clear();
        g_barriers.clear();
    }
}

static std::unique_ptr<uint8_t[]> g_shaderCache;
static std::unique_ptr<uint8_t[]> g_buttonBcDiff;

static void LoadEmbeddedResources()
{
    if (g_vulkan)
    {
        g_shaderCache = std::make_unique<uint8_t[]>(g_spirvCacheDecompressedSize);
        ZSTD_decompress(g_shaderCache.get(), g_spirvCacheDecompressedSize, g_compressedSpirvCache, g_spirvCacheCompressedSize);
    }
#ifdef UNLEASHED_RECOMP_D3D12
    else
    {
        g_shaderCache = std::make_unique<uint8_t[]>(g_dxilCacheDecompressedSize);
        ZSTD_decompress(g_shaderCache.get(), g_dxilCacheDecompressedSize, g_compressedDxilCache, g_dxilCacheCompressedSize);
    }
#endif

    g_buttonBcDiff = decompressZstd(g_button_bc_diff, g_button_bc_diff_uncompressed_size);
}

// VR single-pass stereo: the shaders' VR_STEREO variants replace the cache.
static void LoadStereoShaderCache()
{
    if (g_vulkan)
    {
        g_shaderCache = std::make_unique<uint8_t[]>(g_stereoSpirvCacheDecompressedSize);
        ZSTD_decompress(g_shaderCache.get(), g_stereoSpirvCacheDecompressedSize, g_compressedStereoSpirvCache, g_stereoSpirvCacheCompressedSize);
    }
#ifdef UNLEASHED_RECOMP_D3D12
    else
    {
        g_shaderCache = std::make_unique<uint8_t[]>(g_stereoDxilCacheDecompressedSize);
        ZSTD_decompress(g_shaderCache.get(), g_stereoDxilCacheDecompressedSize, g_compressedStereoDxilCache, g_stereoDxilCacheCompressedSize);
    }
#endif
}

enum class CsdFilterState
{
    Unknown,
    On,
    Off
};

static CsdFilterState g_csdFilterState;

static ankerl::unordered_dense::set<GuestSurface*> g_pendingSurfaceCopies;
static ankerl::unordered_dense::set<GuestSurface*> g_pendingMsaaResolves;

enum class RenderCommandType
{
    SetRenderState,
    DestructResource,
    UnlockTextureRect,
    UnlockBuffer16,
    UnlockBuffer32,
    DrawImGui,
    ExecuteCommandList,
    BeginCommandList,
    StretchRect,
    SetRenderTarget,
    SetDepthStencilSurface,
    ExecutePendingStretchRectCommands,
    Clear,
    SetViewport,
    SetTexture,
    SetScissorRect,
    SetSamplerState,
    SetBooleans,
    SetVertexShaderConstants,
    SetPixelShaderConstants,
    AddPipeline,
    DrawPrimitive,
    DrawIndexedPrimitive,
    DrawPrimitiveUP,
    SetVertexDeclaration,
    SetVertexShader,
    SetStreamSource,
    SetIndices,
    SetPixelShader,

    // Only in the VR replay: re-issues an upload the left eye's pass made.
    VRReplayUpload,
};

struct RenderCommand
{
    RenderCommandType type;
    union
    {
        struct
        {
            GuestRenderState type;
            uint32_t value;
        } setRenderState;

        struct 
        {
            GuestResource* resource;
        } destructResource;

        struct
        {
            GuestTexture* texture;
        } unlockTextureRect;

        struct
        {
            GuestBuffer* buffer;
        } unlockBuffer;

        struct 
        {
            GuestDevice* device;
            uint32_t flags;
            GuestTexture* texture;
        } stretchRect;

        struct 
        {
            GuestSurface* renderTarget;
        } setRenderTarget;

        struct 
        {
            GuestSurface* depthStencil;
        } setDepthStencilSurface;

        struct 
        {
            uint32_t flags;
            float color[4];
            float z;
        } clear;

        struct 
        {
            float x;
            float y;
            float width;
            float height;
            float minDepth;
            float maxDepth;
        } setViewport;

        struct 
        {
            uint32_t index;
            GuestTexture* texture;
        } setTexture;

        struct 
        {
            int32_t left;
            int32_t top;
            int32_t right;
            int32_t bottom;
        } setScissorRect;

        struct
        {
            uint32_t index;
            uint32_t data0;
            uint32_t data3;
            uint32_t data5;
        } setSamplerState;

        struct
        {
            uint32_t booleans;
        } setBooleans;

        struct
        {
            uint8_t* memory;
            uint32_t index;
            uint32_t size;
        } setVertexShaderConstants;  
        
        struct
        {
            uint8_t* memory;
            uint32_t index;
            uint32_t size;
        } setPixelShaderConstants;

        struct
        {
            XXH64_hash_t hash;
            RenderPipeline* pipeline;
        } addPipeline;

        struct 
        {
            uint32_t primitiveType; 
            uint32_t startVertex; 
            uint32_t primitiveCount;
        } drawPrimitive;

        struct 
        {
            uint32_t primitiveType;
            int32_t baseVertexIndex; 
            uint32_t startIndex;
            uint32_t primCount;
        } drawIndexedPrimitive;

        struct 
        {
            uint32_t primitiveType;
            uint32_t primitiveCount; 
            uint8_t* vertexStreamZeroData;
            uint32_t vertexStreamZeroSize;
            uint32_t vertexStreamZeroStride;
            CsdFilterState csdFilterState;
        } drawPrimitiveUP;

        struct 
        {
            GuestVertexDeclaration* vertexDeclaration;
        } setVertexDeclaration;

        struct 
        {
            GuestShader* shader;
        } setVertexShader;

        struct 
        {
            uint32_t index;
            GuestBuffer* buffer;
            uint32_t offset;
            uint32_t stride;
        } setStreamSource;

        struct 
        {
            GuestBuffer* buffer;
        } setIndices;

        struct
        {
            GuestShader* shader;
        } setPixelShader;

        struct
        {
            uint32_t recordIndex;
        } vrReplayUpload;
    };
};

static moodycamel::BlockingConcurrentQueue<RenderCommand> g_renderQueue;

// VR stereo by frame replay. The render thread draws each frame for the left
// eye as its commands arrive and records them; at ExecuteCommandList it outputs
// that eye, restores the state it had when the frame began and replays the
// recording for the right eye. The game's camera constants are rewritten per eye
// when they are uploaded (FlushRenderStateForRenderThread). Commands with side
// effects of their own (resource lifetime, pipelines, uploads) are not replayed;
// uploads are re-issued from the copies the left eye's pass made, since the
// game's CPU-side data may have changed since.

// -1: not rendering for the headset; 0: the left eye, live; 1: the right eye, replayed.
static int32_t g_vrPass = -1;

// VR: single-pass stereo ([VR] VRSinglePass) draws both eyes at once into
// targets with a layer per eye, where frame replay draws one eye after the
// other. Fixed for the run once the session exists, before any shader, texture
// view or pipeline: the shaders are the VR_STEREO variants, every 2D view is an
// array view, and pipelines take a viewport and scissor rectangle per eye.
static bool g_vrSinglePass;

// g_VREyeMode's bits (shader_common.h): the eye mode (the left eye, the right
// eye, or both, odd instances drawing the right eye), whether the target has a
// layer per eye, whether the draw's 2D goes on the HUD panel, and whether both
// eyes read the first block of vertex or pixel shader constants.
enum : uint32_t
{
    VR_EYE_MODE_LEFT = 0,
    VR_EYE_MODE_RIGHT = 1,
    VR_EYE_MODE_BOTH = 2,
    VR_EYE_MODE_MASK = 3,
    VR_EYE_MODE_LAYERED = 4,
    VR_EYE_MODE_PANEL = 8,
    VR_EYE_MODE_SHARED_VERTEX_CONSTANTS = 16,
    VR_EYE_MODE_SHARED_PIXEL_CONSTANTS = 32
};

// How the draw being flushed reaches the eyes in single-pass stereo.
enum class VRStereoDraw
{
    // Once, for the left eye, into layer 0: targets with one layer, or no
    // headset frame.
    LeftEye,

    // Once with twice the instances, odd ones for the right eye.
    BothEyes,

    // Once per eye: the game's instanced draws, whose instance data the
    // hardware would otherwise step through twice as fast.
    EachEye
};

static VRStereoDraw g_vrStereoDraw = VRStereoDraw::LeftEye;

// The blocks of vertex and pixel shader constants last uploaded: one for both
// eyes, or one per eye when they are rewritten per eye.
static uint32_t g_vrVertexConstantBlocks = 1;
static uint32_t g_vrPixelConstantBlocks = 1;

// VR: only the constant registers the shaders declare are uploaded ([VR]
// VRPartialConstantUploads), and the registers the last uploads hold. Fixed for
// the run once the session exists.
static bool g_vrPartialConstantUploads;
static uint64_t g_vrUploadedVertexConstants[4];
static uint64_t g_vrUploadedPixelConstants[4];

static constexpr uint64_t VR_ALL_CONSTANTS[4] = { ~0ull, ~0ull, ~0ull, ~0ull };

static const uint64_t* GetVRConstantMask(const GuestShader* shader)
{
    return (g_vrPartialConstantUploads && shader != nullptr) ? shader->vrConstantMask : VR_ALL_CONSTANTS;
}

static bool IsVRConstantSubset(const uint64_t* mask, const uint64_t* uploaded)
{
    return ((mask[0] & ~uploaded[0]) | (mask[1] & ~uploaded[1]) | (mask[2] & ~uploaded[2]) | (mask[3] & ~uploaded[3])) == 0;
}

// The layers of a render target or depth surface, and of a texture the game
// resolves into: one per eye in single-pass stereo, except square ones (the
// shadow maps, the luminance chain, cube faces) and small ones, drawn once for
// the left eye and read by both.
static uint32_t GetVRLayerCount(uint32_t width, uint32_t height)
{
    return (g_vrSinglePass && width != height && std::max(width, height) > 64) ? 2 : 1;
}

// The viewports (and scissor rectangles) every pipeline takes: one per eye in
// single-pass stereo, the vertex shaders selecting the eye's.
static uint32_t GetVRViewportCount()
{
    return g_vrSinglePass ? 2 : 1;
}

// The shading rate image bound to the frame's command list (see VRFoveation).
static const RenderTexture* g_boundShadingRateImage;

static void SetShadingRateImage(const RenderTexture* image)
{
    if (image != g_boundShadingRateImage)
    {
        g_commandLists[g_frame]->setShadingRateImage(image);
        g_boundShadingRateImage = image;
    }
}

// Sets the viewport of a pass the host draws, for every eye.
static void SetHostViewport(RenderCommandList* commandList, const RenderViewport& viewport)
{
    RenderViewport viewports[2] = { viewport, viewport };
    commandList->setViewports(viewports, GetVRViewportCount());
}

// Sets the scissor rectangle of a pass the host draws, for every eye.
static void SetHostScissor(RenderCommandList* commandList, const RenderRect& rect)
{
    RenderRect rects[2] = { rect, rect };
    commandList->setScissors(rects, GetVRViewportCount());
}
static bool g_vrRecording;
static std::vector<RenderCommand> g_vrRecordedCommands;

struct VRUploadRecord
{
    // Buffer uploads.
    GuestBuffer* buffer;
    RenderBuffer* uploadBuffer;

    // Texture uploads.
    GuestTexture* texture;
    UploadAllocation allocation;
    uint32_t pitch;
};

static std::vector<VRUploadRecord> g_vrUploadRecords;

// The render-thread state at the start of the frame, restored before the replay.
struct VRStateSnapshot
{
    RenderViewport viewport;
    PipelineState pipelineState;
    int32_t depthBias;
    float slopeScaledDepthBias;
    uint32_t vertexShaderConstants[0x400];
    uint32_t pixelShaderConstants[0x380];
    SharedConstants sharedConstants;
    GuestTexture* textures[16];
    RenderSamplerDesc samplerDescs[16];
    bool scissorTestEnable;
    RenderRect scissorRect;
    RenderVertexBufferView vertexBufferViews[16];
    RenderInputSlot inputSlots[16];
    RenderIndexBufferView indexBufferView;
};

static VRStateSnapshot g_vrSnapshot;

// The views both eyes are drawn with this frame, latched at its first camera draw.
struct VREyeState
{
    float projectionTerms[4];

    // The game camera's view space to the eye's, through the horizon lock.
    XrMatrix eyeFromView;

    // The HUD canvas (normalised device coordinates) on a panel in front of the
    // game camera, as this eye sees it: clip = [x, y, 0, 1] * hudMatrix.
    XrMatrix hudMatrix;
};

// The UI drawn with ImGui in stereo frames: drawn once into this texture during
// the left eye's pass, then put on the HUD panel in each eye.
static std::unique_ptr<RenderTexture> g_vrUiTexture;
static std::unique_ptr<RenderTextureView> g_vrUiTextureView;
static uint32_t g_vrUiTextureWidth;
static uint32_t g_vrUiTextureHeight;
static std::unique_ptr<RenderFramebuffer> g_vrUiFramebuffer;
static uint32_t g_vrUiTextureDescriptorIndex;
static std::unique_ptr<RenderPipeline> g_vrUiCompositePipeline;
static bool g_vrUiUsed;

// The next draw is on the HUD panel (a CSD draw or a 2D primitive moved there):
// set just before FlushRenderStateForRenderThread, which consumes it. While the
// current draw is on the panel, the game's scissor rectangle is moved onto the
// panel with it.
static bool g_vrNextDrawOnPanel;
static bool g_vrScissorOnPanel;

// The draw being flushed comes from DrawPrimitiveUP (for the VR log).
static bool g_vrDrawIsUP;

static bool g_vrViewsLatched;
static VRViews g_vrViews;

// Counts the views latched, so that what was computed from older ones is redone.
static uint32_t g_vrViewsGeneration;

static VREyeState g_vrEyes[2];
static uint32_t g_vrCameraDrawCount;

// A camera looking at the world through a planar mirror, as the reflection map
// is drawn: world to clip is R * L * S, where R is world to the mirrored view (a
// rigid transform with a mirror in it), L a perspective lens, and S an optional
// vertical image flip (the water shaders read the map upside down). Wherever the
// game puts the mirror (view, projection, or a reflected camera with its image
// flipped), the product has this form; DecomposeVRReflection recovers R and L.
struct VRReflection
{
    XrMatrix mirroredView;
    float lensX;
    float lensY;
    float offsetX;
    float offsetY;
    bool flipped;

    // The depth column of world to clip is not the lens's: a clip plane at the
    // water (an oblique near plane), kept as is. Otherwise the lens's depth terms.
    bool oblique;
    float depthScale;
    float depthOffset;
    float depthColumn[4];
};

// How the left eye's draws were classified, logged every few seconds.
struct VRDrawStatistics
{
    uint32_t immersiveFrames;
    uint32_t flatFrames;
    uint32_t cameraShaderDraws;
    uint32_t notPerspective;
    uint32_t otherSize;
    uint32_t noTarget;
    uint32_t noViews;
    uint32_t accepted;
    uint32_t acceptedSameLens;

    // Reflection draws given per-eye cameras (into the reflection map, or with a
    // mirror in a projection using the main lens).
    uint32_t reflectionPerEye;
    uint32_t reflectionFlipped;
    uint32_t reflectionOblique;

    // The reflection map: draws reading it, the size of the texture they read,
    // whether that texture was resolved from the target being drawn, and the
    // draws into the surface it is resolved from.
    uint32_t reflectionMapReads;
    uint32_t reflectionMapUnresolved;
    uint32_t reflectionMapSelf;
    uint32_t reflectionMapWidth;
    uint32_t reflectionMapHeight;
    uint32_t reflectionMapDraws;
    uint32_t reflectionMapNoCamera;
    uint32_t reflectionMapNotMirrored;

    // The first draw into the reflection map and the first scene-sized camera
    // draw of the period, logged with their matrices.
    bool reflectionSampleTaken;
    bool reflectionSampleDecomposed;
    uint64_t reflectionSampleShader;
    float reflectionSampleProjection[16];
    float reflectionSampleView[16];
    VRReflection reflectionSample;
    bool mainSampleTaken;
    float mainSampleProjection[16];
    float mainSampleView[16];

    uint32_t lastOtherWidth;
    uint32_t lastOtherHeight;
    float lastOtherProjectionX;
    float lastOtherProjectionY;
    uint32_t lastNotPerspectiveWidth;
    uint32_t lastNotPerspectiveHeight;
    float lastNotPerspective23;
    float lastNotPerspective33;

    // Boost blur passes left without their effect.
    uint32_t boostPassesNeutralised;

    // Depth of field passes left without their effect, and frames of cutscenes
    // shown on the floating screen.
    uint32_t depthOfFieldNeutralised;
    uint32_t cutsceneFrames;

    // Draws, clears and copies of the shadow maps the right eye's pass skipped.
    uint32_t shadowWorkReused;

    // Persistent bloom passes given this frame's bloom for the previous one, the
    // game's persistence factor, and how often the previous one had been
    // resolved from the pass's own target (the feedback loop).
    uint32_t bloomFeedbackReplaced;
    float lastBloomPersistence;
    uint32_t bloomHistoryFromOwnTarget;

    // Light shaft passes, how many had the light behind the left eye, and the
    // last light position on screen, as the game gave it and for the left eye.
    uint32_t lightShaftPasses;
    uint32_t lightShaftsBehind;
    float lastLightShaftGame[2];
    float lastLightShaftEye[2];

    // Draws into the back buffer left over the whole view in stereo frames:
    // CSD draws covering the canvas, full-screen 2D primitives, 2D draws from
    // vertex buffers, and camera draws whose projection is not a perspective one;
    // with the kind, shader, viewport and target of the last one, and for a
    // camera draw its projection's scale and offset terms ([0], [5], [12], [13]).
    uint32_t viewLockedCsd;
    uint32_t viewLocked2D;
    uint32_t viewLockedBuffer2D;
    uint32_t viewLockedOrtho;
    const char* lastViewLockedKind;
    uint64_t lastViewLockedShader;
    float lastViewLockedViewport[4];
    uint32_t lastViewLockedTarget[2];
    float lastViewLockedProjection[4];

    // 2D draws into the back buffer by other shaders than the HUD's.
    uint32_t twoDOnPanel;
    uint32_t twoDFullScreen;
    uint32_t twoDUnhandled;
    uint64_t lastUnhandledShader;
    uint32_t lastUnhandledStride;
    uint32_t lastUnhandledPositionType;
    const char* lastUnhandledReason;

    // Single-pass stereo: draws by how they reached the eyes, draws with a
    // target of each kind whose layers differ (drawn for the left eye), and
    // constant uploads with a block per eye.
    uint32_t leftEyeDraws;
    uint32_t bothEyesDraws;
    uint32_t eachEyeDraws;
    uint32_t layerMismatchDraws;
    uint32_t perEyeVertexConstants;
    uint32_t perEyePixelConstants;

    // Timings of the headset frames. The game thread: the time between its
    // presents, its update and render directors (the game's logic, and the
    // game issuing its draws), and its waits for the headset (xrWaitFrame), for
    // the render thread to submit the frame, and for the GPU to finish an older
    // frame. The render thread: its time from the start of a frame to its output
    // (waits for the game's commands included), its time processing commands,
    // and of that its time flushing state and issuing draws. The GPU's time on
    // the frame's command list (the last one measured), and pipelines the render
    // thread had to create itself.
    // Draws shaded with a foveation image.
    uint32_t foveatedDraws;

    // Hidden area mask draws, the triangles of the last one and the share of
    // the image they cover.
    uint32_t hiddenAreaDraws;
    uint32_t hiddenAreaTriangles;
    float hiddenAreaShare;

    uint32_t timedFrames;
    double constantUploadMs;
    double gameFrameMs;
    double gameFrameMaxMs;
    double gameUpdateMs;
    double gameRenderMs;
    double gameWorkMaxMs;
    double headsetWaitMs;
    double renderThreadWaitMs;
    double gpuFenceWaitMs;
    double frameSpanMs;
    double frameSpanMaxMs;
    double renderThreadBusyMs;
    double renderThreadBusyMaxMs;
    double drawMs;
    double gpuFrameMs;
    double gpuFrameMaxMs;
    uint32_t renderThreadPipelines;
    double renderThreadPipelineMs;
};

static std::chrono::steady_clock::time_point g_vrFrameStartTime;

// The hidden area mask ([VR] VRHiddenAreaMask): the part of each eye's image the
// lens hides (VR::GetHiddenAreaMesh) is drawn at the nearest depth into the
// scene's depth buffers right after the game clears them, so nothing is shaded
// there. Only in frames following a stereo frame, and not in cutscenes shown on
// the floating screen, whose picture is seen whole.
static std::vector<float> g_vrHiddenAreaMeshes[2];
static uint32_t g_vrHiddenAreaSerials[2];
static bool g_vrHiddenAreaFrame;
static bool g_vrLastFrameImmersive;
static std::unique_ptr<RenderShader> g_vrMaskShader;
static std::unique_ptr<RenderPipeline> g_vrMaskPipelines[4][2];

// Fixed foveated rendering ([VR] VRFoveation: 0 off, 1 low, 2 medium, 3 high):
// draws into the scene's targets are shaded at 2x2 or 4x4 pixels per pixel
// shader invocation away from the lens centre of the eye they are for, from
// shading rate images of a texel per screen tile holding each eye's pattern.
// The HUD, menus, copies and the headset output keep the full rate, and so do
// frames shown on the floating screen.
//
// D3D12 (variable rate shading tier 2): an image per eye for frame replay, which
// draws one eye at a time (images 0 and 1), and for single-pass stereo an image
// with a layer per eye (image 2).
//
// Vulkan (VK_KHR_fragment_shading_rate): the image is an attachment of every
// framebuffer, so there is one for the device, g_vrShadingRateAttachment,
// updated in place and only used by the draws that enable it. In single-pass
// stereo it has a layer per eye; frame replay writes the eye's pattern into its
// first layer before each eye's pass. Where the device reads one layer for all
// the target's (g_vrShadingRateAttachmentLayers 1), single-pass stereo uses the
// finer rate of the two eyes per tile (pattern 2).
struct VRFoveation
{
    bool built = false;
    std::unique_ptr<RenderTexture> images[3];

    // The eyes' patterns and the one both share, a byte per tile.
    std::vector<uint8_t> patterns[3];
    uint32_t tilesX = 0;
    uint32_t tilesY = 0;
    uint32_t sceneWidth = 0;
    uint32_t sceneHeight = 0;
    int32_t level = 0;
    float terms[2][4]{};

    // The tiles at each rate (1x1, 2x2, 4x4), in the left eye's pattern.
    uint32_t tileCounts[3]{};
};

static VRFoveation g_vrFoveation;

// Vulkan: the device's shading rate attachment (see VRFoveation), a square of
// this many tiles, zeroed (the full rate) at the first frame.
static std::unique_ptr<RenderTexture> g_vrShadingRateAttachment;
static uint32_t g_vrShadingRateAttachmentSize;
static uint32_t g_vrShadingRateAttachmentLayers = 1;
static bool g_vrShadingRateAttachmentReady;

static void UploadVRFoveationPattern(RenderTexture* texture, uint32_t layer, uint32_t pattern);
static std::vector<std::unique_ptr<RenderTexture>> g_vrRetiredFoveationImages[NUM_FRAMES];
static bool g_vrFoveationFrame;
static bool g_vrEyesValid;

// The render thread's time processing commands since the frame's output, and
// the start of the batch of commands it is processing (see g_renderThread).
static double g_renderThreadBusyMs;
static std::chrono::steady_clock::time_point g_renderThreadBatchStart;

// Adds the time a draw takes on the render thread to the statistics.
struct VRDrawTimer
{
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    ~VRDrawTimer();
};

static VRDrawStatistics g_vrStatistics;

// The main camera's lens (the absolute frustum scales of its projection), from
// the scene-sized camera draws. Other targets drawn with the same lens (planar
// reflections) are drawn per eye too.
static bool g_vrMainLensValid;
static bool g_vrMainLensSeenThisFrame;
static float g_vrMainLensX;
static float g_vrMainLensY;

// The game camera's view, from the frame's first scene-sized camera draw (or,
// before that draw, an earlier frame's), for the horizon lock.
static bool g_vrMainViewValid;
static XrMatrix g_vrMainView;

// The game camera as last drawn, for the guest (Video::GetVRCameraState).
static std::mutex g_vrCameraStateMutex;
static bool g_vrCameraStateValid;
static std::chrono::steady_clock::time_point g_vrCameraStateTime;
static float g_vrCameraStateLensX;
static float g_vrCameraStateLensY;
static XrMatrix g_vrCameraStateHorizonLock = XrMatrixIdentity();

// The surface the reflection map (g_ReflectionMapSampler) was last resolved from,
// as the left eye's draws read it, and its value latched at the start of the
// frame so that both eyes classify the same draws alike. Compared by address only.
static GuestSurface* g_vrReflectionSurfaceSeen;
static GuestSurface* g_vrReflectionSurface;

// How a draw's constants are rewritten for the eye being drawn.
enum class VRDrawKind
{
    None,

    // Through the game camera: per-eye view and projection.
    Camera,

    // A planar reflection with the mirror folded into the projection: per-eye
    // projection around the mirror, view kept.
    Reflection
};

// VR, groundwork for single-pass stereo: what each surface was used for by the
// left eye's passes of the last 5 s, to decide which need a layer per eye. Only
// surfaces drawn into or cleared are described (their fields are copied while
// they are bound, so a destroyed one is never read); the surfaces their draws
// read are matched by address. An entry is dropped with its surface.
struct VRSurfaceUse
{
    uint32_t order;
    uint32_t width;
    uint32_t height;
    uint32_t guestFormat;
    RenderFormat format;
    uint32_t sampleCount;
    bool backBuffer;
    bool orthographicDepth;
    bool perspectiveDepth;
    bool described;
    uint32_t frames;
    uint32_t lastFrame;
    uint32_t cameraDraws;
    uint32_t reflectionDraws;
    uint32_t otherDraws;
    uint32_t clears;
    uint32_t resolves;
    uint32_t lastResolveWidth;
    uint32_t lastResolveHeight;
    uint32_t reads;
    std::array<const GuestSurface*, 8> sources;
    uint32_t sourceCount;
};

static ankerl::unordered_dense::map<const GuestSurface*, VRSurfaceUse> g_vrSurfaceUses;
static uint32_t g_vrSurfaceFrame;
static uint32_t g_vrSurfaceCount;

static VRSurfaceUse& NoteVRSurface(const GuestSurface* surface)
{
    auto [it, inserted] = g_vrSurfaceUses.try_emplace(surface);
    VRSurfaceUse& use = it->second;

    if (inserted)
        use.order = ++g_vrSurfaceCount;

    use.width = surface->width;
    use.height = surface->height;
    use.guestFormat = surface->guestFormat;
    use.format = surface->format;
    use.sampleCount = uint32_t(surface->sampleCount);
    use.backBuffer = (surface == g_backBuffer);
    use.orthographicDepth = surface->vrOrthographicDepth;
    use.perspectiveDepth = surface->vrPerspectiveDepth;
    use.described = true;

    if (use.lastFrame != g_vrSurfaceFrame)
    {
        use.lastFrame = g_vrSurfaceFrame;
        ++use.frames;
    }

    return use;
}

static void NoteVRSurfaceClear(const GuestSurface* renderTarget, const GuestSurface* depthStencil, uint32_t flags)
{
    if (renderTarget != nullptr && (flags & D3DCLEAR_TARGET) != 0)
        ++NoteVRSurface(renderTarget).clears;

    if (depthStencil != nullptr && (flags & D3DCLEAR_ZBUFFER) != 0)
        ++NoteVRSurface(depthStencil).clears;
}

static void NoteVRSurfaceResolve(const GuestSurface* surface, const GuestTexture* texture)
{
    if (surface == nullptr)
        return;

    VRSurfaceUse& use = NoteVRSurface(surface);
    ++use.resolves;
    use.lastResolveWidth = texture->width;
    use.lastResolveHeight = texture->height;
}

// What the last uploaded constants were rewritten for: the shader and kind of a
// camera draw, the pixel shader of a neutralised boost or bloom pass, or null,
// and whether the shared constants had the bloom feedback texture replaced. A
// change forces a new upload.
static GuestShader* g_vrVertexPatchShader;
static VRDrawKind g_vrVertexPatchKind;
static GuestShader* g_vrPixelPatchShader;
static bool g_vrSharedPatch;

static void RecordVRCommand(const RenderCommand& cmd)
{
    switch (cmd.type)
    {
    case RenderCommandType::DestructResource:
    case RenderCommandType::UnlockTextureRect: // Recorded as VRReplayUpload.
    case RenderCommandType::UnlockBuffer16:    // Recorded as VRReplayUpload.
    case RenderCommandType::UnlockBuffer32:    // Recorded as VRReplayUpload.
    case RenderCommandType::ExecuteCommandList:
    case RenderCommandType::BeginCommandList:
    case RenderCommandType::AddPipeline:
    case RenderCommandType::VRReplayUpload:
        break;

    default:
        g_vrRecordedCommands.push_back(cmd);
        break;
    }
}

static void RecordVRUpload(const VRUploadRecord& record)
{
    if (!g_vrRecording)
        return;

    RenderCommand cmd;
    cmd.type = RenderCommandType::VRReplayUpload;
    cmd.vrReplayUpload.recordIndex = uint32_t(g_vrUploadRecords.size());
    g_vrUploadRecords.push_back(record);
    g_vrRecordedCommands.push_back(cmd);
}

static float LoadGuestConstant(const uint32_t* constants, size_t index)
{
    return std::bit_cast<float>(ByteSwap(constants[index]));
}

static XrMatrix LoadGuestMatrix(const uint32_t* constants, size_t firstRegister)
{
    XrMatrix matrix;
    for (size_t i = 0; i < 16; i++)
        matrix.m[i] = LoadGuestConstant(constants, firstRegister * 4 + i);

    return matrix;
}

static void UpdateVRFoveation();

// Called at the end of BeginCommandList while VR is active.
static void BeginVRFrame()
{
    g_vrFrameStartTime = std::chrono::steady_clock::now();
    g_vrPass = 0;
    ++g_vrSurfaceFrame;
    g_vrRecording = !g_vrSinglePass;
    g_vrRecordedCommands.clear();
    g_vrUploadRecords.clear();
    g_vrViewsLatched = false;
    g_vrViews = {};
    g_vrCameraDrawCount = 0;
    g_vrVertexPatchShader = nullptr;
    g_vrVertexPatchKind = VRDrawKind::None;
    g_vrPixelPatchShader = nullptr;
    g_vrSharedPatch = false;
    g_vrFrameResult = {};
    g_vrUiUsed = false;
    g_vrNextDrawOnPanel = false;
    g_vrScissorOnPanel = false;
    g_vrMainLensSeenThisFrame = false;
    g_vrReflectionSurface = g_vrReflectionSurfaceSeen;

    // Cutscenes go on the floating screen as the game frames them ([VR]
    // VRCutsceneScreen): their cuts and camera moves are hard to watch around
    // the head. With no views latched, nothing is drawn per eye.
    bool vrCutscene = Config::VRCutsceneScreen && InspirePatches::s_sceneActive;

    if (vrCutscene)
    {
        g_vrViewsLatched = true;
        ++g_vrStatistics.cutsceneFrames;
    }

    // [VR] VRPerformanceTest: the feature tested is on every other second.
    int32_t performanceTest = Config::VRPerformanceTest;
    bool testOn = (std::chrono::steady_clock::now().time_since_epoch() / std::chrono::seconds(1)) % 2 == 0;
    bool testing = (performanceTest == 1 || performanceTest == 2) && g_vrLastFrameImmersive && !vrCutscene;
    g_gpuSegmentTestStates[g_frame] = testing ? int8_t(testOn ? 1 : 0) : int8_t(-1);

    g_vrHiddenAreaFrame = Config::VRHiddenAreaMask && g_vrLastFrameImmersive && !vrCutscene && (performanceTest != 1 || testOn);

    UpdateVRFoveation();
    g_vrFoveationFrame = g_vrFoveation.built && Config::VRFoveation > 0 && g_vrLastFrameImmersive && !vrCutscene &&
        (performanceTest != 2 || testOn);

    // Vulkan, frame replay: the left eye's pattern for its pass (see VRFoveation).
    if (g_vrFoveationFrame && g_capabilities.shadingRateAttachment && !g_vrSinglePass)
        UploadVRFoveationPattern(g_vrShadingRateAttachment.get(), 0, 0);

    if (g_vrHiddenAreaFrame)
    {
        for (uint32_t eye = 0; eye < 2; eye++)
            VR::GetHiddenAreaMesh(eye, g_vrHiddenAreaMeshes[eye], g_vrHiddenAreaSerials[eye]);
    }

    // Single-pass stereo replays nothing.
    if (g_vrSinglePass)
        return;

    g_vrSnapshot.viewport = g_viewport;
    g_vrSnapshot.pipelineState = g_pipelineState;
    g_vrSnapshot.depthBias = g_depthBias;
    g_vrSnapshot.slopeScaledDepthBias = g_slopeScaledDepthBias;
    memcpy(g_vrSnapshot.vertexShaderConstants, g_vertexShaderConstants, sizeof(g_vertexShaderConstants));
    memcpy(g_vrSnapshot.pixelShaderConstants, g_pixelShaderConstants, sizeof(g_pixelShaderConstants));
    g_vrSnapshot.sharedConstants = g_sharedConstants;
    memcpy(g_vrSnapshot.textures, g_textures, sizeof(g_textures));
    std::copy(std::begin(g_samplerDescs), std::end(g_samplerDescs), g_vrSnapshot.samplerDescs);
    g_vrSnapshot.scissorTestEnable = g_scissorTestEnable;
    g_vrSnapshot.scissorRect = g_scissorRect;
    std::copy(std::begin(g_vertexBufferViews), std::end(g_vertexBufferViews), g_vrSnapshot.vertexBufferViews);
    std::copy(std::begin(g_inputSlots), std::end(g_inputSlots), g_vrSnapshot.inputSlots);
    g_vrSnapshot.indexBufferView = g_indexBufferView;
}

// Puts the render-thread state back to where the frame began, for the replay.
static void RestoreVRFrameStart(GuestSurface* backBuffer)
{
    // The back buffer was bound at the start; CreateDevice may have moved it since.
    g_renderTarget = backBuffer;
    g_depthStencil = nullptr;
    g_framebuffer = nullptr;

    g_viewport = g_vrSnapshot.viewport;
    g_pipelineState = g_vrSnapshot.pipelineState;
    g_depthBias = g_vrSnapshot.depthBias;
    g_slopeScaledDepthBias = g_vrSnapshot.slopeScaledDepthBias;
    memcpy(g_vertexShaderConstants, g_vrSnapshot.vertexShaderConstants, sizeof(g_vertexShaderConstants));
    memcpy(g_pixelShaderConstants, g_vrSnapshot.pixelShaderConstants, sizeof(g_pixelShaderConstants));
    g_sharedConstants = g_vrSnapshot.sharedConstants;
    memcpy(g_textures, g_vrSnapshot.textures, sizeof(g_textures));

    // Sampler descriptor indices are only updated when a descriptor changes, so
    // the descriptors must match the indices restored with the shared constants.
    std::copy(std::begin(g_vrSnapshot.samplerDescs), std::end(g_vrSnapshot.samplerDescs), g_samplerDescs);

    g_scissorTestEnable = g_vrSnapshot.scissorTestEnable;
    g_scissorRect = g_vrSnapshot.scissorRect;
    std::copy(std::begin(g_vrSnapshot.vertexBufferViews), std::end(g_vrSnapshot.vertexBufferViews), g_vertexBufferViews);
    std::copy(std::begin(g_vrSnapshot.inputSlots), std::end(g_vrSnapshot.inputSlots), g_inputSlots);
    g_indexBufferView = g_vrSnapshot.indexBufferView;

    g_dirtyStates = DirtyStates(true);
    g_vrVertexPatchShader = nullptr;
    g_vrVertexPatchKind = VRDrawKind::None;
    g_vrPixelPatchShader = nullptr;
    g_vrSharedPatch = false;
    g_vrNextDrawOnPanel = false;
    g_vrScissorOnPanel = false;

    // The output pass and ImGui bound their own layouts and descriptors.
    FlushBarriers();

    // Vulkan: the right eye's foveation pattern for its pass (see VRFoveation).
    if (g_vrFoveationFrame && g_capabilities.shadingRateAttachment)
        UploadVRFoveationPattern(g_vrShadingRateAttachment.get(), 0, 1);

    auto& commandList = g_commandLists[g_frame];
    commandList->setGraphicsPipelineLayout(g_pipelineLayout.get());
    commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 0);
    commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 1);
    commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 2);
    commandList->setGraphicsDescriptorSet(g_samplerDescriptorSet.get(), 3);
}

static void NormalizeVR3(float* v)
{
    float length = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (length > 0.0f)
    {
        v[0] /= length;
        v[1] /= length;
        v[2] /= length;
    }
}

// The horizon lock: a rotation of the game camera's view space that takes out
// the camera's roll ([VR] VRLockRoll) and pitch ([VR] VRLockPitch), so that the
// eyes see the horizon level and the head alone tilts the view. The HUD panel
// sits in the levelled space. Identity when the camera looks straight up or
// down and the lock would be undefined.
static XrMatrix GetVRHorizonLock()
{
    XrMatrix lock = XrMatrixIdentity();
    if (!g_vrMainViewValid || (!Config::VRLockRoll && !Config::VRLockPitch))
        return lock;

    // The camera's axes in the world are the view's columns (row vectors: viewPoint = worldPoint * view).
    const float* v = g_vrMainView.m;
    float axes[3][3] =
    {
        { v[0], v[4], v[8] },  // right
        { v[1], v[5], v[9] },  // up
        { v[2], v[6], v[10] }  // back
    };

    for (auto& axis : axes)
        NormalizeVR3(axis);

    float forward[3] = { -axes[2][0], -axes[2][1], -axes[2][2] };
    float levelAxes[3][3];

    if (Config::VRLockPitch)
    {
        // The heading alone; looking straight down or up, the up axis gives it.
        float headingX = forward[0], headingZ = forward[2];
        if (headingX * headingX + headingZ * headingZ < 0.05f * 0.05f)
        {
            float sign = forward[1] < 0.0f ? 1.0f : -1.0f;
            headingX = sign * axes[1][0];
            headingZ = sign * axes[1][2];
        }

        float length = sqrtf(headingX * headingX + headingZ * headingZ);
        if (length < 1.0e-4f)
            return lock;

        headingX /= length;
        headingZ /= length;

        // right = forward x up, up = +Y, back = -forward.
        float levelRight[3] = { -headingZ, 0.0f, headingX };
        float levelUp[3] = { 0.0f, 1.0f, 0.0f };
        float levelBack[3] = { -headingX, 0.0f, -headingZ };
        memcpy(levelAxes[0], levelRight, sizeof(levelRight));
        memcpy(levelAxes[1], levelUp, sizeof(levelUp));
        memcpy(levelAxes[2], levelBack, sizeof(levelBack));
    }
    else
    {
        // The forward axis kept; the right axis flattened onto the floor.
        float levelRight[3] = { -forward[2], 0.0f, forward[0] };
        float length = sqrtf(levelRight[0] * levelRight[0] + levelRight[2] * levelRight[2]);
        if (length < 0.05f)
            return lock;

        levelRight[0] /= length;
        levelRight[2] /= length;

        // up = right x forward.
        float levelUp[3] =
        {
            levelRight[1] * forward[2] - levelRight[2] * forward[1],
            levelRight[2] * forward[0] - levelRight[0] * forward[2],
            levelRight[0] * forward[1] - levelRight[1] * forward[0]
        };

        memcpy(levelAxes[0], levelRight, sizeof(levelRight));
        memcpy(levelAxes[1], levelUp, sizeof(levelUp));
        memcpy(levelAxes[2], axes[2], sizeof(axes[2]));
    }

    // view * lock = levelled view: lock = transpose(axes) * levelAxes.
    for (size_t i = 0; i < 3; i++)
    {
        for (size_t j = 0; j < 3; j++)
            lock.m[i * 4 + j] = axes[i][0] * levelAxes[j][0] + axes[i][1] * levelAxes[j][1] + axes[i][2] * levelAxes[j][2];
    }

    return lock;
}

static void LatchVRViews()
{
    g_vrViewsLatched = true;
    ++g_vrViewsGeneration;

    VR::GetViews(g_vrViews);
    if (!g_vrViews.valid)
        return;

    float unitsPerMeter = std::clamp(Config::VRWorldScale.Value, 0.001f, 1000.0f);

    // The HUD panel: the canvas's shape, VRHudWidth wide, VRHudDistance ahead of
    // the game camera (row vectors: [x, y, 0, 1] * panel = view-space point).
    float aspectRatio = float(Video::s_viewportWidth) / float(std::max(1u, Video::s_viewportHeight));
    float halfWidth = 0.5f * std::max(0.1f, Config::VRHudWidth.Value) * unitsPerMeter;

    XrMatrix panel = XrMatrixIdentity();
    panel.m[0] = halfWidth;
    panel.m[5] = halfWidth / aspectRatio;
    panel.m[14] = -std::max(0.1f, Config::VRHudDistance.Value) * unitsPerMeter;

    XrMatrix horizonLock = GetVRHorizonLock();

    for (size_t i = 0; i < 2; i++)
    {
        const VRViews::Eye& eye = g_vrViews.eyes[i];
        VREyeState& eyeState = g_vrEyes[i];
        XrProjectionFrustumTerms(eye.angleLeft, eye.angleRight, eye.angleUp, eye.angleDown, eyeState.projectionTerms);

        // The world goes through the horizon lock first; the HUD panel is already level.
        XrMatrix eyeFromLevelled = XrMatrixEyeFromView(eye.orientation, eye.position, unitsPerMeter);
        eyeState.eyeFromView = XrMatrixMultiply(horizonLock, eyeFromLevelled);

        // Only x, y and w matter: the HUD keeps its own depth.
        XrMatrix projection{};
        projection.m[0] = eyeState.projectionTerms[0];
        projection.m[5] = eyeState.projectionTerms[1];
        projection.m[8] = eyeState.projectionTerms[2];
        projection.m[9] = eyeState.projectionTerms[3];
        projection.m[11] = -1.0f;

        eyeState.hudMatrix = XrMatrixMultiply(XrMatrixMultiply(panel, eyeFromLevelled), projection);
    }

    g_vrEyesValid = true;

    // Single-pass stereo: the panel's matrices are the frame's, one per eye.
    if (g_vrSinglePass)
    {
        for (size_t i = 0; i < 2; i++)
            memcpy(g_sharedConstants.vrHudMatrix[i], g_vrEyes[i].hudMatrix.m, sizeof(g_sharedConstants.vrHudMatrix[i]));

        g_dirtyStates.sharedConstants = true;
    }
}

// The frame is shown in stereo (so far, during the left eye's pass).
static bool IsVRFrameImmersive()
{
    return g_vrPass >= 0 && g_vrViews.valid && g_vrCameraDrawCount > 0;
}

// VR: the shadow maps are drawn for the game camera's frustum, the same for
// both eyes. They are the square depth surfaces the left eye's pass saw
// orthographic draws use and no perspective camera draw. There are two: one
// D3DFMT_D24FS8 like the scene's depth buffer, one D3DFMT_D24S8. Unless [VR]
// VRReuseShadows is off, the right eye's pass reuses the left eye's: draws and
// clears into them, with no other target or a dummy one of their size, and
// copies out of them are skipped, while every state change still runs. A shadow
// map drawn through one surface several times keeps the copies the left eye's
// pass made of it; the last one, never copied, is read from the surface (see
// ProcStretchRect).
static bool IsVRShadowMap(const GuestSurface* surface)
{
    return surface != nullptr && surface->vrOrthographicDepth && !surface->vrPerspectiveDepth;
}

static bool ShouldReuseVRShadowWork()
{
    if (g_vrPass != 1 || !Config::VRReuseShadows || !IsVRShadowMap(g_depthStencil))
        return false;

    if (g_renderTarget != nullptr && (g_renderTarget->width != g_depthStencil->width || g_renderTarget->height != g_depthStencil->height))
        return false;

    ++g_vrStatistics.shadowWorkReused;
    return true;
}

// Counts a draw into the back buffer left over the whole view of a stereo frame
// (see VRDrawStatistics), keeping what the log shows of the last one.
static void NoteVRViewLocked(uint32_t& counter, const char* kind)
{
    if (g_vrPass != 0 || !IsVRFrameImmersive())
        return;

    VRDrawStatistics& s = g_vrStatistics;
    ++counter;
    s.lastViewLockedKind = kind;
    s.lastViewLockedShader = g_pipelineState.vertexShader != nullptr ? g_pipelineState.vertexShader->vrHash : 0;
    s.lastViewLockedViewport[0] = g_viewport.x;
    s.lastViewLockedViewport[1] = g_viewport.y;
    s.lastViewLockedViewport[2] = g_viewport.width;
    s.lastViewLockedViewport[3] = g_viewport.height;
    s.lastViewLockedTarget[0] = g_renderTarget != nullptr ? g_renderTarget->width : 0;
    s.lastViewLockedTarget[1] = g_renderTarget != nullptr ? g_renderTarget->height : 0;
    memset(s.lastViewLockedProjection, 0, sizeof(s.lastViewLockedProjection));
}

static void SetVRHudOnPanel(bool onPanel)
{
    // Single-pass stereo: the matrices are set with the views (LatchVRViews), and
    // the draw's eye mode carries the panel bit (FlushRenderStateForRenderThread).
    if (!g_vrSinglePass)
    {
        if (onPanel)
        {
            const float* matrix = g_vrEyes[g_vrPass].hudMatrix.m;
            if (memcmp(g_sharedConstants.vrHudMatrix[0], matrix, sizeof(g_sharedConstants.vrHudMatrix[0])) != 0)
            {
                memcpy(g_sharedConstants.vrHudMatrix[0], matrix, sizeof(g_sharedConstants.vrHudMatrix[0]));
                g_dirtyStates.sharedConstants = true;
            }
        }

        SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.vrHudEnabled, uint32_t(onPanel));
    }

    g_vrNextDrawOnPanel = onPanel;
}

// The game's scissor rectangle (render-target pixels over the 2D canvas the
// viewport covers) for a draw moved onto the HUD panel: its corners go through
// the eye's panel matrix, and the result is their bounding box. A tilted head
// makes the box a little larger than the moved rectangle.
static RenderRect GetVRPanelScissorRect(uint32_t eye)
{
    RenderRect viewportRect(
        int32_t(g_viewport.x),
        int32_t(g_viewport.y),
        int32_t(g_viewport.x + g_viewport.width),
        int32_t(g_viewport.y + g_viewport.height));

    if (g_vrPass < 0 || g_viewport.width <= 0.0f || g_viewport.height <= 0.0f)
        return viewportRect;

    const float* matrix = g_vrEyes[eye].hudMatrix.m;
    float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;

    for (int32_t corner = 0; corner < 4; corner++)
    {
        float pixelX = float((corner & 1) != 0 ? g_scissorRect.right : g_scissorRect.left);
        float pixelY = float((corner & 2) != 0 ? g_scissorRect.bottom : g_scissorRect.top);
        float x = (pixelX - g_viewport.x) / g_viewport.width * 2.0f - 1.0f;
        float y = 1.0f - (pixelY - g_viewport.y) / g_viewport.height * 2.0f;

        float clipX = x * matrix[0] + y * matrix[4] + matrix[12];
        float clipY = x * matrix[1] + y * matrix[5] + matrix[13];
        float clipW = x * matrix[3] + y * matrix[7] + matrix[15];

        // A corner behind the eye: the panel is out of view anyway.
        if (clipW <= 1.0e-4f)
            return viewportRect;

        float panelX = g_viewport.x + (clipX / clipW + 1.0f) * 0.5f * g_viewport.width;
        float panelY = g_viewport.y + (1.0f - clipY / clipW) * 0.5f * g_viewport.height;
        minX = std::min(minX, panelX);
        minY = std::min(minY, panelY);
        maxX = std::max(maxX, panelX);
        maxY = std::max(maxY, panelY);
    }

    RenderRect rect(
        std::clamp(int32_t(std::floor(minX)), viewportRect.left, viewportRect.right),
        std::clamp(int32_t(std::floor(minY)), viewportRect.top, viewportRect.bottom),
        std::clamp(int32_t(std::ceil(maxX)), viewportRect.left, viewportRect.right),
        std::clamp(int32_t(std::ceil(maxY)), viewportRect.top, viewportRect.bottom));

    return rect;
}

static void NoteVRUnhandled2D(GuestShader* shader, uint32_t stride, uint32_t positionType, const char* reason)
{
    if (g_vrPass != 0)
        return;

    ++g_vrStatistics.twoDUnhandled;
    g_vrStatistics.lastUnhandledShader = (shader != nullptr && shader->shaderCacheEntry != nullptr) ? shader->shaderCacheEntry->hash : 0;
    g_vrStatistics.lastUnhandledStride = stride;
    g_vrStatistics.lastUnhandledPositionType = positionType;
    g_vrStatistics.lastUnhandledReason = reason;
}

// VR: a 2D draw into the back buffer by a shader of the game's own (menu text,
// loading screens, transitions, the debug primitives) whose vertices are
// already in clip space with w = 1: in stereo frames its uploaded vertices are
// moved onto the HUD panel for the eye being drawn. A draw covering exactly the
// screen (a filter or a fade) stays over the whole view; one reaching well past
// it is an element zooming in or out, and goes on the panel. `uploaded` is the
// little-endian copy the GPU reads, `vertices` the game's big-endian data.
// Returns whether the draw goes on the panel. In single-pass stereo the vertex
// shader moves it, for each eye (g_VREyeMode's panel bit).
//
// A float4 position gets the panel's perspective in its w. A float3 one (the
// shader supplies w = 1, as FxFilterT does) gets the divided position, so its
// texture coordinates interpolate without perspective across the panel: fine
// for a panel seen nearly face-on.
static bool PlaceVR2DPrimitiveOnPanel(const uint8_t* vertices, uint8_t* uploaded, uint32_t size, uint32_t stride)
{
    GuestShader* vertexShader = g_pipelineState.vertexShader;
    if (g_renderTarget != g_backBuffer || vertexShader == nullptr || vertexShader->vrProjectionRegister >= 0 || vertexShader->vrHudShader)
        return false;

    GuestVertexDeclaration* declaration = g_pipelineState.vertexDeclaration;
    if (declaration == nullptr || stride < 12)
    {
        NoteVRUnhandled2D(vertexShader, stride, 0, "no declaration");
        return false;
    }

    uint32_t positionOffset = UINT32_MAX;
    uint32_t positionType = 0;

    for (uint32_t i = 0; i < declaration->vertexElementCount; i++)
    {
        const GuestVertexElement& element = declaration->vertexElements[i];
        if (element.stream == 0xFF)
            break;

        if (element.stream == 0 && element.usage == D3DDECLUSAGE_POSITION && element.usageIndex == 0)
        {
            positionOffset = element.offset;
            positionType = element.type;
            break;
        }
    }

    bool hasW = (positionType == D3DDECLTYPE_FLOAT4);
    uint32_t positionSize = hasW ? 16 : 12;

    if (positionOffset == UINT32_MAX || (!hasW && positionType != D3DDECLTYPE_FLOAT3) || positionOffset + positionSize > stride)
    {
        NoteVRUnhandled2D(vertexShader, stride, positionType, "position not float3 or float4");
        return false;
    }

    auto loadFloat = [&](uint32_t offset)
        {
            return std::bit_cast<float>(ByteSwap(*reinterpret_cast<const uint32_t*>(vertices + offset)));
        };

    float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;

    for (uint32_t offset = positionOffset; offset + positionSize <= size; offset += stride)
    {
        float x = loadFloat(offset);
        float y = loadFloat(offset + 4);
        float w = hasW ? loadFloat(offset + 12) : 1.0f;

        if (std::abs(w - 1.0f) > 0.01f)
        {
            NoteVRUnhandled2D(vertexShader, stride, positionType, "w not 1");
            return false;
        }

        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
    }

    bool coversScreen = minX <= -0.99f && minY <= -0.99f && maxX >= 0.99f && maxY >= 0.99f;
    bool withinScreen = minX >= -1.05f && minY >= -1.05f && maxX <= 1.05f && maxY <= 1.05f;

    if (coversScreen && withinScreen)
    {
        if (g_vrPass == 0)
            ++g_vrStatistics.twoDFullScreen;

        NoteVRViewLocked(g_vrStatistics.viewLocked2D, "2D full screen");
        return false;
    }

    if (g_vrPass == 0)
        ++g_vrStatistics.twoDOnPanel;

    if (g_vrSinglePass)
        return true;

    const float* matrix = g_vrEyes[g_vrPass].hudMatrix.m;

    for (uint32_t offset = positionOffset; offset + positionSize <= size; offset += stride)
    {
        float x = loadFloat(offset);
        float y = loadFloat(offset + 4);
        float z = loadFloat(offset + 8);

        // [x, y, 0, 1] * matrix; the depth keeps its own value.
        float clipX = x * matrix[0] + y * matrix[4] + matrix[12];
        float clipY = x * matrix[1] + y * matrix[5] + matrix[13];
        float clipW = x * matrix[3] + y * matrix[7] + matrix[15];

        float* position = reinterpret_cast<float*>(uploaded + offset);

        if (hasW)
        {
            position[0] = clipX;
            position[1] = clipY;
            position[2] = z * clipW;
            position[3] = clipW;
        }
        else
        {
            // Behind the eye the panel is out of view; keep the vertex off screen.
            float inverseW = clipW > 1.0e-4f ? 1.0f / clipW : 0.0f;
            position[0] = clipW > 1.0e-4f ? clipX * inverseW : 2.0f;
            position[1] = clipW > 1.0e-4f ? clipY * inverseW : 2.0f;
            position[2] = z;
        }
    }

    return true;
}

// Whether a CSD draw covers the whole canvas. Untextured ones (a screen fade, a
// dimmed background) stay over the whole view rather than going on the panel.
// Positions are the first two (big-endian) floats of each vertex, in the pixels
// of the viewport size the CSD vertex shader divides by (register c180).
static bool CoversVRHudCanvas(const uint8_t* vertices, uint32_t size, uint32_t stride)
{
    float canvasWidth = LoadGuestConstant(g_vertexShaderConstants, 180 * 4 + 0);
    float canvasHeight = LoadGuestConstant(g_vertexShaderConstants, 180 * 4 + 1);
    if (stride < 8 || size < stride || canvasWidth <= 0.0f || canvasHeight <= 0.0f)
        return false;

    float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;

    for (uint32_t offset = 0; offset + 8 <= size; offset += stride)
    {
        float x = std::bit_cast<float>(ByteSwap(*reinterpret_cast<const uint32_t*>(vertices + offset)));
        float y = std::bit_cast<float>(ByteSwap(*reinterpret_cast<const uint32_t*>(vertices + offset + 4)));
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
    }

    return minX <= 1.0f && minY <= 1.0f && maxX >= canvasWidth - 1.0f && maxY >= canvasHeight - 1.0f;
}

// The scene's internal resolution, as the SetResolution hook gives it to the game.
static void GetVRSceneSize(int32_t& width, int32_t& height)
{
    width = int32_t(round(Video::s_viewportWidth * Config::ResolutionScale));
    height = int32_t(round(Video::s_viewportHeight * Config::ResolutionScale));
}

static float DotVR3(const float* a, const float* b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// Splits world to clip into the parts of VRReflection. The lens's w column is
// minus R's third column, and its x and y columns are R's first two scaled by the
// lens (plus an offset along the third), so R and the lens follow from those
// three columns; R's last column is (0, 0, 0, 1). Shadow maps (orthographic: no
// w column) and anything that is not a rigid camera do not decompose. A rigid R
// with a positive determinant holds no mirror: the image flip is then taken to
// be vertical and in the lens, as the water shaders' upside-down reads imply.
static bool DecomposeVRReflection(const XrMatrix& worldToClip, VRReflection& reflection)
{
    float x[4], y[4], z[4], w[4];
    for (size_t row = 0; row < 4; row++)
    {
        x[row] = worldToClip.m[row * 4 + 0];
        y[row] = worldToClip.m[row * 4 + 1];
        z[row] = worldToClip.m[row * 4 + 2];
        w[row] = worldToClip.m[row * 4 + 3];
    }

    // A homogeneous scale changes nothing: make the w column's axis a unit vector.
    float scale = sqrtf(DotVR3(w, w));
    if (!(scale > 1e-6f))
        return false;

    float axisX[4], axisY[4], axisZ[4];
    for (size_t row = 0; row < 4; row++)
    {
        x[row] /= scale;
        y[row] /= scale;
        z[row] /= scale;
        axisZ[row] = -w[row] / scale;
    }

    reflection.offsetX = DotVR3(x, axisZ);
    reflection.offsetY = DotVR3(y, axisZ);
    for (size_t row = 0; row < 4; row++)
    {
        axisX[row] = x[row] - reflection.offsetX * axisZ[row];
        axisY[row] = y[row] - reflection.offsetY * axisZ[row];
    }

    reflection.lensX = sqrtf(DotVR3(axisX, axisX));
    reflection.lensY = sqrtf(DotVR3(axisY, axisY));
    if (!(reflection.lensX > 0.05f && reflection.lensX < 50.0f && reflection.lensY > 0.05f && reflection.lensY < 50.0f))
        return false;

    for (size_t row = 0; row < 4; row++)
    {
        axisX[row] /= reflection.lensX;
        axisY[row] /= reflection.lensY;
    }

    if (std::abs(DotVR3(axisX, axisY)) > 0.02f)
        return false;

    float axisYCrossZ[3] =
    {
        axisY[1] * axisZ[2] - axisY[2] * axisZ[1],
        axisY[2] * axisZ[0] - axisY[0] * axisZ[2],
        axisY[0] * axisZ[1] - axisY[1] * axisZ[0]
    };

    reflection.flipped = DotVR3(axisX, axisYCrossZ) > 0.0f;
    if (reflection.flipped)
    {
        for (float& value : axisY)
            value = -value;

        reflection.offsetY = -reflection.offsetY;
    }

    for (size_t row = 0; row < 4; row++)
    {
        reflection.mirroredView.m[row * 4 + 0] = axisX[row];
        reflection.mirroredView.m[row * 4 + 1] = axisY[row];
        reflection.mirroredView.m[row * 4 + 2] = axisZ[row];
        reflection.mirroredView.m[row * 4 + 3] = (row == 3) ? 1.0f : 0.0f;
    }

    reflection.depthScale = DotVR3(z, axisZ);
    reflection.depthOffset = z[3] - reflection.depthScale * axisZ[3];

    float residual[3] =
    {
        z[0] - reflection.depthScale * axisZ[0],
        z[1] - reflection.depthScale * axisZ[1],
        z[2] - reflection.depthScale * axisZ[2]
    };

    reflection.oblique = sqrtf(DotVR3(residual, residual)) > 1e-3f + 0.01f * std::abs(reflection.depthScale);
    memcpy(reflection.depthColumn, z, sizeof(z));
    return true;
}

// World to clip of the current draw, from its vertex shader's view and projection.
static bool GetVRWorldToClip(const GuestShader* vertexShader, XrMatrix& view, XrMatrix& projection, XrMatrix& worldToClip)
{
    if (vertexShader == nullptr || vertexShader->vrProjectionRegister < 0 || vertexShader->vrViewRegister < 0 || vertexShader->vrViewRegisterCount != 4)
        return false;

    view = LoadGuestMatrix(g_vertexShaderConstants, vertexShader->vrViewRegister);
    projection = LoadGuestMatrix(g_vertexShaderConstants, vertexShader->vrProjectionRegister);
    worldToClip = XrMatrixMultiply(view, projection);
    return true;
}

static void CountVRReflection(const VRReflection& reflection)
{
    ++g_vrStatistics.reflectionPerEye;

    if (reflection.flipped)
        ++g_vrStatistics.reflectionFlipped;

    if (reflection.oblique)
        ++g_vrStatistics.reflectionOblique;
}

// A draw into the reflection map's surface: a reflection draw when its camera
// decomposes (see VRReflection), whatever its projection looks like. The first
// one of each logging period is kept for the log, with the engine's projection
// and view registers (c0, c4) when the shader declares neither.
static VRDrawKind GetVRReflectionMapDrawKind(GuestShader* vertexShader, bool leftEye)
{
    XrMatrix view{}, projection{}, worldToClip{};
    VRReflection reflection{};
    bool hasCamera = GetVRWorldToClip(vertexShader, view, projection, worldToClip);
    bool decomposed = hasCamera && DecomposeVRReflection(worldToClip, reflection);

    if (leftEye)
    {
        VRDrawStatistics& s = g_vrStatistics;
        ++s.reflectionMapDraws;

        if (!hasCamera)
            ++s.reflectionMapNoCamera;
        else if (!decomposed)
            ++s.reflectionMapNotMirrored;

        if (!s.reflectionSampleTaken)
        {
            if (!hasCamera)
            {
                projection = LoadGuestMatrix(g_vertexShaderConstants, 0);
                view = LoadGuestMatrix(g_vertexShaderConstants, 4);
            }

            s.reflectionSampleTaken = true;
            s.reflectionSampleDecomposed = decomposed;
            s.reflectionSampleShader = (vertexShader != nullptr && vertexShader->shaderCacheEntry != nullptr) ? vertexShader->shaderCacheEntry->hash : 0;
            memcpy(s.reflectionSampleProjection, projection.m, sizeof(projection.m));
            memcpy(s.reflectionSampleView, view.m, sizeof(view.m));
            s.reflectionSample = reflection;
        }
    }

    if (!decomposed)
        return VRDrawKind::None;

    if (!g_vrViewsLatched)
        LatchVRViews();

    if (leftEye)
    {
        if (g_vrViews.valid)
            CountVRReflection(reflection);
        else
            ++g_vrStatistics.noViews;
    }

    return g_vrViews.valid ? VRDrawKind::Reflection : VRDrawKind::None;
}

// Remembers the surface the reflection map read by the current draw was drawn
// into, so that the next frames draw into it per eye. A map resolved from the
// target being drawn would be the scene itself and is ignored.
static void NoteVRReflectionMapRead(GuestSurface* renderTarget)
{
    GuestShader* pixelShader = g_pipelineState.pixelShader;
    if (pixelShader == nullptr || pixelShader->vrReflectionSamplerRegister < 0)
        return;

    ++g_vrStatistics.reflectionMapReads;

    GuestTexture* texture = g_textures[pixelShader->vrReflectionSamplerRegister];
    if (texture == nullptr || texture->vrResolvedFrom == nullptr)
    {
        ++g_vrStatistics.reflectionMapUnresolved;
        return;
    }

    g_vrStatistics.reflectionMapWidth = texture->width;
    g_vrStatistics.reflectionMapHeight = texture->height;

    if (texture->vrResolvedFrom == renderTarget)
    {
        ++g_vrStatistics.reflectionMapSelf;
        return;
    }

    g_vrReflectionSurfaceSeen = texture->vrResolvedFrom;
}

// A draw through the game camera: its vertex shader reads the camera's
// projection and view, the projection is a perspective one, and it renders into
// the scene's targets (full, half or quarter resolution, within 3% for any
// alignment the game applies) or uses the main camera's lens (a planar
// reflection), rather than a shadow map, a cube map or another target with a
// camera of its own. Draws into the reflection map, and draws whose projection
// folds a mirror into the main camera's lens, are reflection draws.
static VRDrawKind GetVRDrawKind(GuestSurface* renderTarget, GuestSurface* depthStencil)
{
    GuestShader* vertexShader = g_pipelineState.vertexShader;
    bool leftEye = (g_vrPass == 0);

    if (g_vrReflectionSurface != nullptr && renderTarget == g_vrReflectionSurface)
        return GetVRReflectionMapDrawKind(vertexShader, leftEye);

    if (vertexShader == nullptr || vertexShader->vrProjectionRegister < 0 || vertexShader->vrViewRegister < 0)
        return VRDrawKind::None;

    if (leftEye)
        ++g_vrStatistics.cameraShaderDraws;

    GuestSurface* surface = renderTarget != nullptr ? renderTarget : depthStencil;

    size_t projection = size_t(vertexShader->vrProjectionRegister) * 4;
    float projection23 = LoadGuestConstant(g_vertexShaderConstants, projection + 11);
    float projection33 = LoadGuestConstant(g_vertexShaderConstants, projection + 15);

    if (std::abs(projection23 + 1.0f) > 0.001f || std::abs(projection33) > 0.001f)
    {
        XrMatrix view, gameProjection, worldToClip;
        VRReflection reflection;

        if (g_vrMainLensValid &&
            GetVRWorldToClip(vertexShader, view, gameProjection, worldToClip) &&
            DecomposeVRReflection(worldToClip, reflection) &&
            std::abs(reflection.lensX - g_vrMainLensX) <= 0.03f * g_vrMainLensX &&
            std::abs(reflection.lensY - g_vrMainLensY) <= 0.03f * g_vrMainLensY)
        {
            if (!g_vrViewsLatched)
                LatchVRViews();

            if (leftEye)
            {
                if (g_vrViews.valid)
                    CountVRReflection(reflection);
                else
                    ++g_vrStatistics.noViews;
            }

            return g_vrViews.valid ? VRDrawKind::Reflection : VRDrawKind::None;
        }

        if (leftEye && depthStencil != nullptr && depthStencil->width == depthStencil->height &&
            (depthStencil->guestFormat == D3DFMT_D24FS8 || depthStencil->guestFormat == D3DFMT_D24S8))
        {
            depthStencil->vrOrthographicDepth = true;
        }

        if (leftEye)
        {
            ++g_vrStatistics.notPerspective;
            g_vrStatistics.lastNotPerspectiveWidth = surface != nullptr ? surface->width : 0;
            g_vrStatistics.lastNotPerspectiveHeight = surface != nullptr ? surface->height : 0;
            g_vrStatistics.lastNotPerspective23 = projection23;
            g_vrStatistics.lastNotPerspective33 = projection33;

            if (renderTarget == g_backBuffer && IsVRFrameImmersive())
            {
                NoteVRViewLocked(g_vrStatistics.viewLockedOrtho, "camera, not perspective");
                g_vrStatistics.lastViewLockedProjection[0] = LoadGuestConstant(g_vertexShaderConstants, projection + 0);
                g_vrStatistics.lastViewLockedProjection[1] = LoadGuestConstant(g_vertexShaderConstants, projection + 5);
                g_vrStatistics.lastViewLockedProjection[2] = LoadGuestConstant(g_vertexShaderConstants, projection + 12);
                g_vrStatistics.lastViewLockedProjection[3] = LoadGuestConstant(g_vertexShaderConstants, projection + 13);
            }
        }

        return VRDrawKind::None;
    }

    if (surface == nullptr)
    {
        if (leftEye)
            ++g_vrStatistics.noTarget;

        return VRDrawKind::None;
    }

    int32_t sceneWidth, sceneHeight;
    GetVRSceneSize(sceneWidth, sceneHeight);

    bool sceneSized = false;

    for (int32_t divisor : { 1, 2, 4 })
    {
        int32_t width = sceneWidth / divisor;
        int32_t height = sceneHeight / divisor;

        if (std::abs(int32_t(surface->width) - width) <= std::max(2, width * 3 / 100) &&
            std::abs(int32_t(surface->height) - height) <= std::max(2, height * 3 / 100))
        {
            sceneSized = true;
            break;
        }
    }

    float lensX = std::abs(LoadGuestConstant(g_vertexShaderConstants, projection + 0));
    float lensY = std::abs(LoadGuestConstant(g_vertexShaderConstants, projection + 5));

    if (sceneSized)
    {
        // The world comes first; later scene-sized draws may use cameras of their own (3D HUD items).
        if (leftEye && !g_vrMainLensSeenThisFrame)
        {
            g_vrMainLensX = lensX;
            g_vrMainLensY = lensY;
            g_vrMainLensValid = true;
            g_vrMainLensSeenThisFrame = true;

            g_vrMainView = LoadGuestMatrix(g_vertexShaderConstants, vertexShader->vrViewRegister);
            g_vrMainViewValid = true;

            {
                std::lock_guard lock(g_vrCameraStateMutex);
                g_vrCameraStateValid = true;
                g_vrCameraStateTime = std::chrono::steady_clock::now();
                g_vrCameraStateLensX = lensX;
                g_vrCameraStateLensY = lensY;
                g_vrCameraStateHorizonLock = GetVRHorizonLock();
            }

            if (!g_vrStatistics.mainSampleTaken)
            {
                g_vrStatistics.mainSampleTaken = true;

                for (size_t i = 0; i < 16; i++)
                {
                    g_vrStatistics.mainSampleProjection[i] = LoadGuestConstant(g_vertexShaderConstants, projection + i);
                    g_vrStatistics.mainSampleView[i] = LoadGuestConstant(g_vertexShaderConstants, size_t(vertexShader->vrViewRegister) * 4 + i);
                }
            }
        }
    }
    else
    {
        bool sameLens = g_vrMainLensValid &&
            std::abs(lensX - g_vrMainLensX) <= 0.01f * g_vrMainLensX &&
            std::abs(lensY - g_vrMainLensY) <= 0.01f * g_vrMainLensY;

        if (!sameLens)
        {
            if (leftEye)
            {
                ++g_vrStatistics.otherSize;
                g_vrStatistics.lastOtherWidth = surface->width;
                g_vrStatistics.lastOtherHeight = surface->height;
                g_vrStatistics.lastOtherProjectionX = lensX;
                g_vrStatistics.lastOtherProjectionY = lensY;
            }

            return VRDrawKind::None;
        }

        if (leftEye)
            ++g_vrStatistics.acceptedSameLens;
    }

    if (!g_vrViewsLatched)
        LatchVRViews();

    if (leftEye)
        ++(g_vrViews.valid ? g_vrStatistics.accepted : g_vrStatistics.noViews);

    return g_vrViews.valid ? VRDrawKind::Camera : VRDrawKind::None;
}

bool Video::GetVRCameraState(float& lensX, float& lensY, float horizonLock[16])
{
    std::lock_guard lock(g_vrCameraStateMutex);
    if (!g_vrCameraStateValid || std::chrono::steady_clock::now() - g_vrCameraStateTime > std::chrono::seconds(1))
        return false;

    lensX = g_vrCameraStateLensX;
    lensY = g_vrCameraStateLensY;
    memcpy(horizonLock, g_vrCameraStateHorizonLock.m, sizeof(g_vrCameraStateHorizonLock.m));
    return true;
}

static const char* GetVRFormatName(RenderFormat format)
{
    switch (format)
    {
    case RenderFormat::R8G8B8A8_UNORM: return "RGBA8";
    case RenderFormat::B8G8R8A8_UNORM: return "BGRA8";
    case RenderFormat::R16G16B16A16_FLOAT: return "RGBA16F";
    case RenderFormat::R16G16_FLOAT: return "RG16F";
    case RenderFormat::R32_FLOAT: return "R32F";
    case RenderFormat::R32G32_FLOAT: return "RG32F";
    case RenderFormat::R8_UNORM: return "R8";
    case RenderFormat::D32_FLOAT: return "D32F";
    default: return "other";
    }
}

// The surface dump: one line per surface the left eye's passes drew into,
// cleared or copied out of, in order of first use, with what another layer
// of each would cost, then the totals by kind.
static void LogVRSurfaceUses()
{
    if (g_vrSurfaceUses.empty())
        return;

    std::vector<std::pair<const GuestSurface*, const VRSurfaceUse*>> uses;
    for (const auto& [surface, use] : g_vrSurfaceUses)
    {
        if (use.described)
            uses.emplace_back(surface, &use);
    }

    std::sort(uses.begin(), uses.end(), [](const auto& a, const auto& b) { return a.second->order < b.second->order; });

    auto findOrder = [](const GuestSurface* surface) -> int64_t
        {
            auto findResult = g_vrSurfaceUses.find(surface);
            return (findResult != g_vrSurfaceUses.end() && findResult->second.described) ? int64_t(findResult->second.order) : -1;
        };

    uint64_t cameraBytes = 0;
    uint64_t otherBytes = 0;

    for (const auto& [surface, use] : uses)
    {
        uint64_t bytes = uint64_t(use->width) * use->height * RenderFormatSize(use->format) * std::max(use->sampleCount, 1u);
        bool perEye = use->cameraDraws != 0 || use->reflectionDraws != 0;
        (perEye ? cameraBytes : otherBytes) += bytes;

        std::string sources;
        for (uint32_t i = 0; i < use->sourceCount; i++)
        {
            int64_t order = findOrder(use->sources[i]);
            sources += (i == 0 ? "" : " ") + (order >= 0 ? fmt::format("#{}", order) : std::string("?"));
        }

        LOGFN("[VR] Surface #{}: {}x{} {} (guest {:08X}) x{}{}{}{}, {} frames, {:.1f} MB. Draws: camera {}, reflection {}, other {}; clears {}; "
            "copied out {} (last into {}x{}); read by {} draws; its draws read [{}].",
            use->order, use->width, use->height, GetVRFormatName(use->format), use->guestFormat, use->sampleCount,
            use->backBuffer ? ", back buffer" : "",
            use->orthographicDepth ? ", orthographic depth" : "",
            use->perspectiveDepth ? ", perspective depth" : "",
            use->frames, double(bytes) / (1024.0 * 1024.0),
            use->cameraDraws, use->reflectionDraws, use->otherDraws, use->clears,
            use->resolves, use->lastResolveWidth, use->lastResolveHeight, use->reads, sources);
    }

    LOGFN("[VR] Surfaces: {} described; {:.1f} MB drawn through the camera, {:.1f} MB otherwise.",
        uses.size(), double(cameraBytes) / (1024.0 * 1024.0), double(otherBytes) / (1024.0 * 1024.0));

    g_vrSurfaceUses.clear();
    g_vrSurfaceCount = 0;
}

static void LogVRStatistics()
{
    static auto s_lastLogTime = std::chrono::steady_clock::now();

    auto now = std::chrono::steady_clock::now();
    if (now - s_lastLogTime < std::chrono::seconds(5))
        return;

    s_lastLogTime = now;

    const VRDrawStatistics& s = g_vrStatistics;
    int32_t sceneWidth, sceneHeight;
    GetVRSceneSize(sceneWidth, sceneHeight);

    LOGFN("[VR] Last 5 s: {} immersive / {} flat frames. Camera-shader draws {}: accepted {} ({} same lens), "
        "not perspective {} (last {}x{}, [2][3] {:.3f}, [3][3] {:.3f}), "
        "other target {} (last {}x{} lens {:.3f}/{:.3f}; scene {}x{} lens {:.3f}/{:.3f}), no target {}, no views {}.",
        s.immersiveFrames, s.flatFrames, s.cameraShaderDraws, s.accepted, s.acceptedSameLens,
        s.notPerspective, s.lastNotPerspectiveWidth, s.lastNotPerspectiveHeight, s.lastNotPerspective23, s.lastNotPerspective33,
        s.otherSize, s.lastOtherWidth, s.lastOtherHeight, s.lastOtherProjectionX, s.lastOtherProjectionY,
        sceneWidth, sceneHeight, g_vrMainLensX, g_vrMainLensY, s.noTarget, s.noViews);

    LOGFN("[VR] Last 5 s, 2D draws into the back buffer: on the panel {}, full screen {}, left in place {} (last: shader {:016X}, stride {}, position type {:X}, {}). "
        "Boost passes neutralised {}. Bloom feedback replaced {} (persistence {:.3f}, history from its own target {}). "
        "Light shaft passes {} ({} with the light behind the eye; last at {:.3f}, {:.3f}, for the left eye {:.3f}, {:.3f}). "
        "Depth of field passes neutralised {}. Cutscene frames on the screen {}. Shadow map work reused by the right eye {}.",
        s.twoDOnPanel, s.twoDFullScreen, s.twoDUnhandled, s.lastUnhandledShader, s.lastUnhandledStride,
        s.lastUnhandledPositionType, s.lastUnhandledReason != nullptr ? s.lastUnhandledReason : "-", s.boostPassesNeutralised,
        s.bloomFeedbackReplaced, s.lastBloomPersistence, s.bloomHistoryFromOwnTarget,
        s.lightShaftPasses, s.lightShaftsBehind, s.lastLightShaftGame[0], s.lastLightShaftGame[1],
        s.lastLightShaftEye[0], s.lastLightShaftEye[1], s.depthOfFieldNeutralised, s.cutsceneFrames, s.shadowWorkReused);

    if (s.timedFrames != 0)
    {
        double n = double(s.timedFrames);

        LOGFN("[VR] Last 5 s, timings per frame: game thread {:.2f} ms (max {:.2f}): update {:.2f} ms, render {:.2f} ms (max of both {:.2f}), "
            "waits for the headset {:.2f} ms, for the render thread {:.2f} ms, for the GPU {:.2f} ms. "
            "Render thread {:.2f} ms (max {:.2f}), busy {:.2f} ms (max {:.2f}), of which draws {:.2f} ms (constant uploads {:.2f} ms). GPU {:.2f} ms (max {:.2f}). "
            "Pipelines created on the render thread: {} ({:.1f} ms).",
            s.gameFrameMs / n, s.gameFrameMaxMs, s.gameUpdateMs / n, s.gameRenderMs / n, s.gameWorkMaxMs,
            s.headsetWaitMs / n, s.renderThreadWaitMs / n, s.gpuFenceWaitMs / n,
            s.frameSpanMs / n, s.frameSpanMaxMs, s.renderThreadBusyMs / n, s.renderThreadBusyMaxMs, s.drawMs / n, s.constantUploadMs / n,
            s.gpuFrameMs / n, s.gpuFrameMaxMs, s.renderThreadPipelines, s.renderThreadPipelineMs);
    }

    if (s.foveatedDraws != 0)
    {
        const VRFoveation& f = g_vrFoveation;
        uint32_t tiles = std::max(1u, f.tileCounts[0] + f.tileCounts[1] + f.tileCounts[2]);

        LOGFN("[VR] Last 5 s, foveated rendering (level {}): {} draws; each eye's image is {:.0f}% at full rate, "
            "{:.0f}% at 2x2 and {:.0f}% at 4x4 pixels per shading.",
            f.level, s.foveatedDraws, 100.0 * f.tileCounts[0] / tiles, 100.0 * f.tileCounts[1] / tiles, 100.0 * f.tileCounts[2] / tiles);
    }

    if (s.hiddenAreaDraws != 0)
    {
        LOGFN("[VR] Last 5 s, hidden area mask: {} draws of {} triangles covering {:.1f}% of the image.",
            s.hiddenAreaDraws, s.hiddenAreaTriangles, s.hiddenAreaShare * 100.0f);
    }

    {
        double segmentMs[GPU_SEGMENT_KIND_COUNT];
        uint32_t segmentFrames;
        uint64_t segmentTotal;
        double testMs[2][GPU_SEGMENT_KIND_COUNT];
        uint32_t testFrames[2];
        {
            std::lock_guard lock(g_gpuSegmentMutex);
            memcpy(segmentMs, g_gpuSegmentMs, sizeof(segmentMs));
            segmentFrames = g_gpuSegmentFrames;
            segmentTotal = g_gpuSegmentTotal;
            memcpy(testMs, g_gpuSegmentTestMs, sizeof(testMs));
            memcpy(testFrames, g_gpuSegmentTestFrames, sizeof(testFrames));
            memset(g_gpuSegmentMs, 0, sizeof(g_gpuSegmentMs));
            g_gpuSegmentFrames = 0;
            g_gpuSegmentTotal = 0;
            memset(g_gpuSegmentTestMs, 0, sizeof(g_gpuSegmentTestMs));
            memset(g_gpuSegmentTestFrames, 0, sizeof(g_gpuSegmentTestFrames));
        }

        if (testFrames[0] != 0 && testFrames[1] != 0)
        {
            auto describe = [&](uint32_t state)
                {
                    double n = double(testFrames[state]);
                    double total = 0.0;
                    for (double ms : testMs[state])
                        total += ms;

                    return fmt::format("world {:.2f} ms, post-processing and 2D {:.2f} ms, total {:.2f} ms ({} frames)",
                        testMs[state][GPU_SEGMENT_SCENE] / n, testMs[state][GPU_SEGMENT_FLAT] / n, total / n, testFrames[state]);
                };

            LOGFN("[VR] Last 5 s, performance test of {}: on: {}; off: {}.",
                Config::VRPerformanceTest == 1 ? "the hidden area mask" : "foveated rendering", describe(1), describe(0));
        }

        if (segmentFrames != 0)
        {
            double n = double(segmentFrames);

            LOGFN("[VR] Last 5 s, GPU time per frame: shadow maps {:.2f} ms, world (draws with a depth test) {:.2f} ms, "
                "post-processing and 2D (draws without one) {:.2f} ms, headset output {:.2f} ms ({:.0f} segments per frame of {}).",
                segmentMs[GPU_SEGMENT_SHADOWS] / n, segmentMs[GPU_SEGMENT_SCENE] / n, segmentMs[GPU_SEGMENT_FLAT] / n,
                segmentMs[GPU_SEGMENT_OUTPUT] / n, double(segmentTotal) / n, NUM_SEGMENT_QUERIES);
        }
    }

    if (g_vrSinglePass)
    {
        LOGFN("[VR] Last 5 s, single-pass draws: both eyes {}, each eye {} (instanced), left eye only {} ({} with layers differing "
            "between the targets). Constants uploaded per eye: vertex {}, pixel {}.",
            s.bothEyesDraws, s.eachEyeDraws, s.leftEyeDraws, s.layerMismatchDraws, s.perEyeVertexConstants, s.perEyePixelConstants);
    }

    if (s.viewLockedCsd != 0 || s.viewLocked2D != 0 || s.viewLockedBuffer2D != 0 || s.viewLockedOrtho != 0)
    {
        LOGFN("[VR] Last 5 s, back-buffer draws left over the whole view: untextured CSD covering the canvas {}, 2D full screen {}, "
            "2D from vertex buffers {}, camera draws not perspective {} (last: {}, shader {:016X}, viewport {:.0f},{:.0f} {:.0f}x{:.0f} "
            "of {}x{}, projection {:.4f} {:.4f} {:.4f} {:.4f}).",
            s.viewLockedCsd, s.viewLocked2D, s.viewLockedBuffer2D, s.viewLockedOrtho,
            s.lastViewLockedKind != nullptr ? s.lastViewLockedKind : "-", s.lastViewLockedShader,
            s.lastViewLockedViewport[0], s.lastViewLockedViewport[1], s.lastViewLockedViewport[2], s.lastViewLockedViewport[3],
            s.lastViewLockedTarget[0], s.lastViewLockedTarget[1],
            s.lastViewLockedProjection[0], s.lastViewLockedProjection[1], s.lastViewLockedProjection[2], s.lastViewLockedProjection[3]);
    }

    if (s.reflectionMapReads != 0 || s.reflectionMapDraws != 0 || s.reflectionPerEye != 0)
    {
        LOGFN("[VR] Last 5 s, reflections: per eye {} ({} flipped, {} with a clip plane). Reflection map read by {} draws "
            "({} not resolved, {} from their own target), {}x{}; draws into it {} ({} without camera constants, {} not a mirrored camera).",
            s.reflectionPerEye, s.reflectionFlipped, s.reflectionOblique, s.reflectionMapReads,
            s.reflectionMapUnresolved, s.reflectionMapSelf, s.reflectionMapWidth, s.reflectionMapHeight,
            s.reflectionMapDraws, s.reflectionMapNoCamera, s.reflectionMapNotMirrored);
    }

    if (s.reflectionSampleTaken)
    {
        auto formatMatrix = [](const float* m)
            {
                std::string text;
                for (size_t i = 0; i < 16; i++)
                {
                    if (i != 0)
                        text += (i % 4 == 0) ? " | " : " ";

                    text += fmt::format("{:.4f}", m[i]);
                }

                return text;
            };

        const VRReflection& r = s.reflectionSample;

        LOGFN("[VR] Reflection map draw: shader {:016X}, decomposed {} (lens {:.3f}/{:.3f}, offset {:.3f}/{:.3f}, flipped {}, clip plane {}, depth {:.4f}/{:.4f}). "
            "Projection [{}]. View [{}]. Main camera: projection [{}], view [{}].",
            s.reflectionSampleShader, s.reflectionSampleDecomposed, r.lensX, r.lensY, r.offsetX, r.offsetY, r.flipped, r.oblique,
            r.depthScale, r.depthOffset, formatMatrix(s.reflectionSampleProjection), formatMatrix(s.reflectionSampleView),
            s.mainSampleTaken ? formatMatrix(s.mainSampleProjection) : std::string("-"),
            s.mainSampleTaken ? formatMatrix(s.mainSampleView) : std::string("-"));
    }

    LogVRSurfaceUses();

    g_vrStatistics = {};
}

// The camera as the eye of the current pass sees it.
struct VRCameraPatch
{
    // The eye's frustum terms for elements [0], [5], [8] and [9], mirrored like the
    // game's projection when that flips an axis (a reflection camera may).
    float projectionTerms[4];
    XrMatrix projection;
    XrMatrix view;
    XrMatrix eyeView;
    float eyePosition[3];
    float eyeForward[3];
    float gameForward[3];
};

// The last camera patch computed for each eye, with the game's projection and
// view it came from, and the views it was computed with (g_vrViewsGeneration):
// draws through the same camera reuse it.
struct VRCameraPatchCache
{
    bool valid;
    uint32_t generation;
    bool result;
    uint32_t projection[16];
    uint32_t view[16];
    uint32_t viewCount;
    VRCameraPatch patch;
};

static VRCameraPatchCache g_vrCameraPatchCache[2];

static bool ComputeVRCameraPatchUncached(VRCameraPatch& patch, uint32_t eyeIndex);

static bool ComputeVRCameraPatch(VRCameraPatch& patch, uint32_t eyeIndex)
{
    GuestShader* vertexShader = g_pipelineState.vertexShader;
    VRCameraPatchCache& cache = g_vrCameraPatchCache[eyeIndex];

    const uint32_t* projection = g_vertexShaderConstants + vertexShader->vrProjectionRegister * 4;
    const uint32_t* view = g_vertexShaderConstants + vertexShader->vrViewRegister * 4;
    uint32_t viewCount = vertexShader->vrViewRegisterCount * 4;

    if (!cache.valid || cache.generation != g_vrViewsGeneration || cache.viewCount != viewCount ||
        memcmp(cache.projection, projection, sizeof(cache.projection)) != 0 ||
        memcmp(cache.view, view, viewCount * sizeof(uint32_t)) != 0)
    {
        cache.valid = true;
        cache.generation = g_vrViewsGeneration;
        cache.viewCount = viewCount;
        memcpy(cache.projection, projection, sizeof(cache.projection));
        memcpy(cache.view, view, viewCount * sizeof(uint32_t));
        cache.result = ComputeVRCameraPatchUncached(cache.patch, eyeIndex);
    }

    patch = cache.patch;
    return cache.result;
}

static bool ComputeVRCameraPatchUncached(VRCameraPatch& patch, uint32_t eyeIndex)
{
    GuestShader* vertexShader = g_pipelineState.vertexShader;
    const VREyeState& eye = g_vrEyes[eyeIndex];

    patch.projection = LoadGuestMatrix(g_vertexShaderConstants, vertexShader->vrProjectionRegister);

    float signX = patch.projection.m[0] < 0.0f ? -1.0f : 1.0f;
    float signY = patch.projection.m[5] < 0.0f ? -1.0f : 1.0f;
    patch.projectionTerms[0] = signX * eye.projectionTerms[0];
    patch.projectionTerms[1] = signY * eye.projectionTerms[1];
    patch.projectionTerms[2] = signX * eye.projectionTerms[2];
    patch.projectionTerms[3] = signY * eye.projectionTerms[3];

    patch.projection.m[0] = patch.projectionTerms[0];
    patch.projection.m[5] = patch.projectionTerms[1];
    patch.projection.m[8] = patch.projectionTerms[2];
    patch.projection.m[9] = patch.projectionTerms[3];

    patch.view = LoadGuestMatrix(g_vertexShaderConstants, vertexShader->vrViewRegister);
    patch.eyeView = XrMatrixMultiply(patch.view, eye.eyeFromView);

    XrMatrix inverseView, inverseEyeView;
    if (!XrMatrixInvert(patch.view, inverseView) || !XrMatrixInvert(patch.eyeView, inverseEyeView))
        return false;

    // The camera's origin and -Z axis in the world: the inverse's last and third rows.
    for (size_t i = 0; i < 3; i++)
    {
        patch.eyePosition[i] = inverseEyeView.m[12 + i];
        patch.eyeForward[i] = -inverseEyeView.m[8 + i];
        patch.gameForward[i] = -inverseView.m[8 + i];
    }

    return true;
}

static void WriteVRVector(float* constants, int16_t reg, const float value[3])
{
    constants[reg * 4 + 0] = value[0];
    constants[reg * 4 + 1] = value[1];
    constants[reg * 4 + 2] = value[2];
}

// The game's g_EyeDirection may point either way along the view axis; keep its sign.
static void WriteVREyeDirection(float* constants, const uint32_t* guestConstants, int16_t reg, const VRCameraPatch& patch)
{
    float original[3] =
    {
        LoadGuestConstant(guestConstants, reg * 4 + 0),
        LoadGuestConstant(guestConstants, reg * 4 + 1),
        LoadGuestConstant(guestConstants, reg * 4 + 2)
    };

    float sign = (original[0] * patch.gameForward[0] + original[1] * patch.gameForward[1] + original[2] * patch.gameForward[2]) < 0.0f ? -1.0f : 1.0f;
    float direction[3] = { patch.eyeForward[0] * sign, patch.eyeForward[1] * sign, patch.eyeForward[2] * sign };
    WriteVRVector(constants, reg, direction);
}

// Rewrites the uploaded (little-endian) vertex shader constants for the eye.
static void PatchVRVertexShaderConstants(float* constants, const VRCameraPatch& patch)
{
    GuestShader* shader = g_pipelineState.vertexShader;

    // Only the frustum terms change; the depth terms stay the game's, so its
    // near/far planes and the reverse Z the shaders apply on top still hold.
    float* projection = constants + shader->vrProjectionRegister * 4;
    projection[0] = patch.projectionTerms[0];
    projection[5] = patch.projectionTerms[1];
    projection[8] = patch.projectionTerms[2];
    projection[9] = patch.projectionTerms[3];

    memcpy(constants + shader->vrViewRegister * 4, patch.eyeView.m, shader->vrViewRegisterCount * 4 * sizeof(float));

    if (shader->vrEyePositionRegister >= 0)
        WriteVRVector(constants, shader->vrEyePositionRegister, patch.eyePosition);

    if (shader->vrEyeDirectionRegister >= 0)
        WriteVREyeDirection(constants, g_vertexShaderConstants, shader->vrEyeDirectionRegister, patch);
}

// Rewrites the uploaded pixel shader constants for the eye. The view and the
// inverse projection are only rewritten when the game's values are the ones
// the vertex shader's camera implies, so other matrices in those registers stay.
static void PatchVRPixelShaderConstants(float* constants, const VRCameraPatch& patch)
{
    GuestShader* shader = g_pipelineState.pixelShader;
    GuestShader* vertexShader = g_pipelineState.vertexShader;

    if (shader->vrEyePositionRegister >= 0)
        WriteVRVector(constants, shader->vrEyePositionRegister, patch.eyePosition);

    if (shader->vrEyeDirectionRegister >= 0)
        WriteVREyeDirection(constants, g_pixelShaderConstants, shader->vrEyeDirectionRegister, patch);

    if (shader->vrViewRegister >= 0)
    {
        size_t count = shader->vrViewRegisterCount * 4;
        if (memcmp(&g_pixelShaderConstants[shader->vrViewRegister * 4], &g_vertexShaderConstants[vertexShader->vrViewRegister * 4], count * sizeof(uint32_t)) == 0)
            memcpy(constants + shader->vrViewRegister * 4, patch.eyeView.m, count * sizeof(float));
    }

    if (shader->vrInverseProjectionRegister >= 0)
    {
        float gameProjectionX = LoadGuestConstant(g_vertexShaderConstants, vertexShader->vrProjectionRegister * 4);
        float gameInverseProjectionX = LoadGuestConstant(g_pixelShaderConstants, shader->vrInverseProjectionRegister * 4);

        XrMatrix inverseProjection;
        if (std::abs(gameProjectionX * gameInverseProjectionX - 1.0f) < 0.01f && XrMatrixInvert(patch.projection, inverseProjection))
            memcpy(constants + shader->vrInverseProjectionRegister * 4, inverseProjection.m, sizeof(inverseProjection.m));
    }
}

// Rewrites the uploaded projection of a reflection draw (see VRReflection) for
// the eye. The eye sits in the mirrored main view where it sits in the main
// view, so world to clip becomes R * eye * P(eye), with the game's image flip
// after it, and either the lens's depth terms or, with a clip plane at the water,
// the game's own depth column. The view register stays the game's (the shader's
// lighting reads it), so the projection written is view^-1 * world to clip.
static void PatchVRReflectionProjection(float* constants, uint32_t eyeIndex)
{
    GuestShader* shader = g_pipelineState.vertexShader;

    XrMatrix view, gameProjection, gameWorldToClip, inverseView;
    VRReflection reflection;
    if (!GetVRWorldToClip(shader, view, gameProjection, gameWorldToClip) ||
        !DecomposeVRReflection(gameWorldToClip, reflection) ||
        !XrMatrixInvert(view, inverseView))
    {
        return;
    }

    const VREyeState& eye = g_vrEyes[eyeIndex];

    XrMatrix eyeProjection{};
    eyeProjection.m[0] = eye.projectionTerms[0];
    eyeProjection.m[5] = eye.projectionTerms[1];
    eyeProjection.m[8] = eye.projectionTerms[2];
    eyeProjection.m[9] = eye.projectionTerms[3];
    eyeProjection.m[10] = reflection.depthScale;
    eyeProjection.m[11] = -1.0f;
    eyeProjection.m[14] = reflection.depthOffset;

    XrMatrix worldToClip = XrMatrixMultiply(XrMatrixMultiply(reflection.mirroredView, eye.eyeFromView), eyeProjection);

    for (size_t row = 0; row < 4; row++)
    {
        if (reflection.flipped)
            worldToClip.m[row * 4 + 1] = -worldToClip.m[row * 4 + 1];

        if (reflection.oblique)
            worldToClip.m[row * 4 + 2] = reflection.depthColumn[row];
    }

    XrMatrix projection = XrMatrixMultiply(inverseView, worldToClip);
    memcpy(constants + shader->vrProjectionRegister * 4, projection.m, sizeof(projection.m));
}

// The boost passes' pixel shaders (see CreateShader): full-screen distortions
// centred on a point the game projects with its own camera, which neither
// matches the eyes nor sits well in a headset. Unless [VR] VRBoostFilter asks
// for them, their parameters (c150-c152: g_Param, g_Param2, g_Param3) are set
// so that each pass outputs its source unchanged: no displacement, no blur, and
// the brightness ring's angle at +90 degrees, where its cosine is zero.
enum VRBoostShader : uint8_t
{
    VR_BOOST_SHADER_NONE,
    VR_BOOST_SHADER_FX_BOOST_BLUR,
    VR_BOOST_SHADER_BOOST_BLUR,
    VR_BOOST_SHADER_BOOST_BLUR_2D
};

static bool ShouldNeutraliseVRBoostPass()
{
    GuestShader* shader = g_pipelineState.pixelShader;
    return g_vrPass >= 0 && shader != nullptr && shader->vrBoostShader != VR_BOOST_SHADER_NONE && !Config::VRBoostFilter;
}

static void NeutraliseVRBoostPass(float* constants, bool count)
{
    float* param = constants + 150 * 4;
    float* param2 = constants + 151 * 4;
    float* param3 = constants + 152 * 4;

    switch (g_pipelineState.pixelShader->vrBoostShader)
    {
    case VR_BOOST_SHADER_FX_BOOST_BLUR:
        // Ring angle: (distance - g_Param2.w) / g_Param3.x, clamped to 1. Displacement: g_Param3.y.
        param2[3] = -1.0e4f;
        param3[0] = 1.0f;
        param3[1] = 0.0f;
        break;

    case VR_BOOST_SHADER_BOOST_BLUR:
        // Blur step g_Param.x, blend g_Param.z, ring as above, displacement g_Param3.y.
        param[0] = 0.0f;
        param[2] = 0.0f;
        param2[3] = -1.0e4f;
        param3[0] = 1.0f;
        param3[1] = 0.0f;
        break;

    case VR_BOOST_SHADER_BOOST_BLUR_2D:
        // Blend g_Param.y. Ring: (distance - g_Param3.x) / (distance * g_Param3.y). Displacement: g_Param3.z.
        param[1] = 0.0f;
        param3[0] = -1.0e4f;
        param3[1] = 1.0f;
        param3[2] = 0.0f;
        break;
    }

    if (count)
        ++g_vrStatistics.boostPassesNeutralised;
}

// The persistent bloom pass (Bloom_BrightPassPersistent) outputs this frame's
// bright pass (sampDif0) plus its own previous output (sampDif1) times
// g_BloomStar_Param1.z (c150.z). In VR the previous output is the other eye's,
// so each eye would carry a ghost of the other eye's glow, one that also trails
// behind head turns. Unless [VR] VRBloomPersistence asks for it, sampDif1 reads
// this frame's bright pass instead, weighted by k / (1 - k): the glow a steady
// scene builds up over frames, without the history. Flat frames are drawn once
// and keep it. The weight is capped (k at
// 0.75), as the history also smooths the flicker of small bright spots.
static bool ShouldReplaceVRBloomFeedback()
{
    GuestShader* shader = g_pipelineState.pixelShader;
    return g_vrPass >= 0 && shader != nullptr && shader->vrPersistentBloomShader && !Config::VRBloomPersistence && IsVRFrameImmersive();
}

static void NoteVRBloomFeedback(GuestSurface* renderTarget)
{
    VRDrawStatistics& s = g_vrStatistics;
    ++s.bloomFeedbackReplaced;
    s.lastBloomPersistence = LoadGuestConstant(g_pixelShaderConstants, 150 * 4 + 2);

    GuestTexture* history = g_textures[1];
    if (history != nullptr && history->vrResolvedFrom == renderTarget)
        ++s.bloomHistoryFromOwnTarget;
}

// Both write uploaded constants and read the originals: upload memory is slow to read.
static void ReplaceVRBloomFeedbackWeight(float* constants)
{
    float persistence = std::clamp(LoadGuestConstant(g_pixelShaderConstants, 150 * 4 + 2), 0.0f, 0.75f);
    constants[150 * 4 + 2] = persistence / (1.0f - persistence);
}

static void ReplaceVRBloomFeedbackTexture(SharedConstants* constants)
{
    constants->texture2DIndices[1] = g_sharedConstants.texture2DIndices[0];
    constants->texture3DIndices[1] = g_sharedConstants.texture3DIndices[0];
    constants->textureCubeIndices[1] = g_sharedConstants.textureCubeIndices[0];
    constants->samplerIndices[1] = g_sharedConstants.samplerIndices[0];
}

// The light shaft pass (LightShaftFilter) blurs the sky towards the sun's (or
// the moon's) position on screen, g_LightShaftSunParam[0].xy (c166), in texture
// coordinates the game projects with its own camera. For the eye, that position
// is turned back into a direction in the game camera's view (through the main
// lens), turned with the eye and projected with the eye's lens. With the light
// behind the eye the shafts are turned off: g_LightShaftSunParam[1].x (c167)
// weighs the samples.
static bool ShouldPatchVRLightShafts()
{
    GuestShader* shader = g_pipelineState.pixelShader;
    if (g_vrPass < 0 || shader == nullptr || !shader->vrLightShaftShader || !g_vrMainLensValid)
        return false;

    if (!g_vrViewsLatched)
        LatchVRViews();

    return g_vrViews.valid;
}

static void NoteVRLightShafts()
{
    VRDrawStatistics& s = g_vrStatistics;
    ++s.lightShaftPasses;
    s.lastLightShaftGame[0] = LoadGuestConstant(g_pixelShaderConstants, 166 * 4 + 0);
    s.lastLightShaftGame[1] = LoadGuestConstant(g_pixelShaderConstants, 166 * 4 + 1);
}

static void PatchVRLightShafts(float* constants, uint32_t eyeIndex, bool count)
{
    float* sun = constants + 166 * 4;
    float* weight = constants + 167 * 4;

    // Texture coordinates to the game camera's view: a direction, so no translation.
    float gameDirection[3] =
    {
        (LoadGuestConstant(g_pixelShaderConstants, 166 * 4 + 0) * 2.0f - 1.0f) / g_vrMainLensX,
        (1.0f - LoadGuestConstant(g_pixelShaderConstants, 166 * 4 + 1) * 2.0f) / g_vrMainLensY,
        -1.0f
    };

    const VREyeState& eye = g_vrEyes[eyeIndex];
    float direction[3];
    for (size_t column = 0; column < 3; column++)
    {
        direction[column] =
            gameDirection[0] * eye.eyeFromView.m[0 * 4 + column] +
            gameDirection[1] * eye.eyeFromView.m[1 * 4 + column] +
            gameDirection[2] * eye.eyeFromView.m[2 * 4 + column];
    }

    float clipW = -direction[2];
    if (clipW <= 1e-3f * sqrtf(DotVR3(direction, direction)))
    {
        weight[0] = 0.0f;

        if (count)
            ++g_vrStatistics.lightShaftsBehind;

        return;
    }

    float clipX = direction[0] * eye.projectionTerms[0] + direction[2] * eye.projectionTerms[2];
    float clipY = direction[1] * eye.projectionTerms[1] + direction[2] * eye.projectionTerms[3];
    sun[0] = 0.5f + 0.5f * clipX / clipW;
    sun[1] = 0.5f - 0.5f * clipY / clipW;

    if (count)
    {
        g_vrStatistics.lastLightShaftEye[0] = sun[0];
        g_vrStatistics.lastLightShaftEye[1] = sun[1];
    }
}

// Depth of field (AlternativeDOF, AlternativeDOF_IgnoreSky) blends each pixel
// towards blurred copies by weights saturate(blur * g_dofLerpScaleBias[0] +
// g_dofLerpScaleBias[1]) (c166, c167). In stereo frames the game focuses where
// its own camera looks, not where the eyes do; unless [VR] VRDepthOfField asks
// for it, the weights are zero and the pass outputs the sharp picture. Flat
// frames (cutscenes on the floating screen) keep it.
static bool ShouldNeutraliseVRDepthOfField()
{
    GuestShader* shader = g_pipelineState.pixelShader;
    return g_vrPass >= 0 && shader != nullptr && shader->vrDepthOfFieldShader && !Config::VRDepthOfField && IsVRFrameImmersive();
}

static void NeutraliseVRDepthOfField(float* constants, bool count)
{
    memset(constants + 166 * 4, 0, 8 * sizeof(float));

    if (count)
        ++g_vrStatistics.depthOfFieldNeutralised;
}

template<GuestRenderState TType>
static void SetRenderState(GuestDevice* device, uint32_t value)
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetRenderState;
    cmd.setRenderState.type = TType;
    cmd.setRenderState.value = value;
    g_renderQueue.enqueue(cmd);
}

static void SetRenderStateUnimplemented(GuestDevice* device, uint32_t value)
{
}

static void SetAlphaTestMode(bool enable)
{
    uint32_t specConstants = 0;
    bool enableAlphaToCoverage = false;

    if (enable)
    {
        enableAlphaToCoverage = Config::TransparencyAntiAliasing && g_renderTarget != nullptr && g_renderTarget->sampleCount != RenderSampleCount::COUNT_1;

        if (enableAlphaToCoverage)
            specConstants = SPEC_CONSTANT_ALPHA_TO_COVERAGE;
        else
            specConstants = SPEC_CONSTANT_ALPHA_TEST;
    }

    specConstants |= (g_pipelineState.specConstants & ~(SPEC_CONSTANT_ALPHA_TEST | SPEC_CONSTANT_ALPHA_TO_COVERAGE));

    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.enableAlphaToCoverage, enableAlphaToCoverage);
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants, specConstants);
}

static RenderBlend ConvertBlendMode(uint32_t blendMode)
{
    switch (blendMode)
    {
    case D3DBLEND_ZERO:
        return RenderBlend::ZERO;
    case D3DBLEND_ONE:
        return RenderBlend::ONE;
    case D3DBLEND_SRCCOLOR:
        return RenderBlend::SRC_COLOR;
    case D3DBLEND_INVSRCCOLOR:
        return RenderBlend::INV_SRC_COLOR;
    case D3DBLEND_SRCALPHA:
        return RenderBlend::SRC_ALPHA;
    case D3DBLEND_INVSRCALPHA:
        return RenderBlend::INV_SRC_ALPHA;
    case D3DBLEND_DESTCOLOR:
        return RenderBlend::DEST_COLOR;
    case D3DBLEND_INVDESTCOLOR:
        return RenderBlend::INV_DEST_COLOR;
    case D3DBLEND_DESTALPHA:
        return RenderBlend::DEST_ALPHA;
    case D3DBLEND_INVDESTALPHA:
        return RenderBlend::INV_DEST_ALPHA;
    default:
        assert(false && "Invalid blend mode");
        return RenderBlend::ZERO;
    }
}

static RenderBlendOperation ConvertBlendOp(uint32_t blendOp)
{
    switch (blendOp)
    {
    case D3DBLENDOP_ADD:
        return RenderBlendOperation::ADD;
    case D3DBLENDOP_SUBTRACT:
        return RenderBlendOperation::SUBTRACT;
    case D3DBLENDOP_REVSUBTRACT:
        return RenderBlendOperation::REV_SUBTRACT;
    case D3DBLENDOP_MIN:
        return RenderBlendOperation::MIN;
    case D3DBLENDOP_MAX:
        return RenderBlendOperation::MAX;
    default:
        assert(false && "Unknown blend operation");
        return RenderBlendOperation::ADD;
    }
}

static void ProcSetRenderState(const RenderCommand& cmd)
{
    uint32_t value = cmd.setRenderState.value;

    switch (cmd.setRenderState.type)
    {
    case D3DRS_ZENABLE:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zEnable, value != 0);
        g_dirtyStates.renderTargetAndDepthStencil |= g_dirtyStates.pipelineState;
        break;
    }
    case D3DRS_ZWRITEENABLE:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zWriteEnable, value != 0);
        break;
    }
    case D3DRS_ALPHATESTENABLE:
    {
        SetAlphaTestMode(value != 0);
        break;
    }
    case D3DRS_SRCBLEND:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.srcBlend, ConvertBlendMode(value));
        break;
    }
    case D3DRS_DESTBLEND:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.destBlend, ConvertBlendMode(value));
        break;
    }
    case D3DRS_CULLMODE:
    {
        RenderCullMode cullMode;

        switch (value) {
        case D3DCULL_NONE:
        case D3DCULL_NONE_2:
            cullMode = RenderCullMode::NONE;
            break;
        case D3DCULL_CW:
            cullMode = RenderCullMode::FRONT;
            break;
        case D3DCULL_CCW:
            cullMode = RenderCullMode::BACK;
            break;
        default:
            assert(false && "Invalid cull mode");
            cullMode = RenderCullMode::NONE;
            break;
        }

        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.cullMode, cullMode);
        break;
    }
    case D3DRS_ZFUNC:
    {
        RenderComparisonFunction comparisonFunc;

        switch (value)
        {
        case D3DCMP_NEVER:
            comparisonFunc = RenderComparisonFunction::NEVER;
            break;
        case D3DCMP_LESS:
            comparisonFunc = RenderComparisonFunction::LESS;
            break;
        case D3DCMP_EQUAL:
            comparisonFunc = RenderComparisonFunction::EQUAL;
            break;
        case D3DCMP_LESSEQUAL:
            comparisonFunc = RenderComparisonFunction::LESS_EQUAL;
            break;
        case D3DCMP_GREATER:
            comparisonFunc = RenderComparisonFunction::GREATER;
            break;
        case D3DCMP_NOTEQUAL:
            comparisonFunc = RenderComparisonFunction::NOT_EQUAL;
            break;
        case D3DCMP_GREATEREQUAL:
            comparisonFunc = RenderComparisonFunction::GREATER_EQUAL;
            break;
        case D3DCMP_ALWAYS:
            comparisonFunc = RenderComparisonFunction::ALWAYS;
            break;
        default:
            assert(false && "Unknown comparison function");
            comparisonFunc = RenderComparisonFunction::NEVER;
            break;
        }

        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zFunc, comparisonFunc);
        break;
    }
    case D3DRS_ALPHAREF:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_sharedConstants.alphaThreshold, float(value) / 256.0f);
        break;
    }
    case D3DRS_ALPHABLENDENABLE:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.alphaBlendEnable, value != 0);
        break;
    }
    case D3DRS_BLENDOP:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.blendOp, ConvertBlendOp(value));
        break;
    }
    case D3DRS_SCISSORTESTENABLE:
    {
        SetDirtyValue(g_dirtyStates.scissorRect, g_scissorTestEnable, value != 0);
        break;
    }
    case D3DRS_SLOPESCALEDEPTHBIAS:
    {
        if (g_capabilities.dynamicDepthBias)
            SetDirtyValue(g_dirtyStates.depthBias, g_slopeScaledDepthBias, *reinterpret_cast<float*>(&value));
        else 
            SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.slopeScaledDepthBias, *reinterpret_cast<float*>(&value));

        break;
    }
    case D3DRS_DEPTHBIAS:
    {
        if (g_capabilities.dynamicDepthBias)
            SetDirtyValue(g_dirtyStates.depthBias, g_depthBias, int32_t(*reinterpret_cast<float*>(&value) * (1 << 24)));
        else
            SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthBias, int32_t(*reinterpret_cast<float*>(&value)* (1 << 24)));

        break;
    }
    case D3DRS_SRCBLENDALPHA:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.srcBlendAlpha, ConvertBlendMode(value));
        break;
    }
    case D3DRS_DESTBLENDALPHA:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.destBlendAlpha, ConvertBlendMode(value));
        break;
    }
    case D3DRS_BLENDOPALPHA:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.blendOpAlpha, ConvertBlendOp(value));
        break;
    }
    case D3DRS_COLORWRITEENABLE:
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.colorWriteEnable, value);
        g_dirtyStates.renderTargetAndDepthStencil |= g_dirtyStates.pipelineState;
        break;
    }
    }
}

static const std::pair<GuestRenderState, PPCFunc*> g_setRenderStateFunctions[] =
{
    { D3DRS_ZENABLE, HostToGuestFunction<SetRenderState<D3DRS_ZENABLE>> },
    { D3DRS_ZWRITEENABLE, HostToGuestFunction<SetRenderState<D3DRS_ZWRITEENABLE>> },
    { D3DRS_ALPHATESTENABLE, HostToGuestFunction<SetRenderState<D3DRS_ALPHATESTENABLE>> },
    { D3DRS_SRCBLEND, HostToGuestFunction<SetRenderState<D3DRS_SRCBLEND>> },
    { D3DRS_DESTBLEND, HostToGuestFunction<SetRenderState<D3DRS_DESTBLEND>> },
    { D3DRS_CULLMODE, HostToGuestFunction<SetRenderState<D3DRS_CULLMODE>> },
    { D3DRS_ZFUNC, HostToGuestFunction<SetRenderState<D3DRS_ZFUNC>> },
    { D3DRS_ALPHAREF, HostToGuestFunction<SetRenderState<D3DRS_ALPHAREF>> },
    { D3DRS_ALPHABLENDENABLE, HostToGuestFunction<SetRenderState<D3DRS_ALPHABLENDENABLE>> },
    { D3DRS_BLENDOP, HostToGuestFunction<SetRenderState<D3DRS_BLENDOP>> },
    { D3DRS_SCISSORTESTENABLE, HostToGuestFunction<SetRenderState<D3DRS_SCISSORTESTENABLE>> },
    { D3DRS_SLOPESCALEDEPTHBIAS, HostToGuestFunction<SetRenderState<D3DRS_SLOPESCALEDEPTHBIAS>> },
    { D3DRS_DEPTHBIAS, HostToGuestFunction<SetRenderState<D3DRS_DEPTHBIAS>> },
    { D3DRS_SRCBLENDALPHA, HostToGuestFunction<SetRenderState<D3DRS_SRCBLENDALPHA>> },
    { D3DRS_DESTBLENDALPHA, HostToGuestFunction<SetRenderState<D3DRS_DESTBLENDALPHA>> },
    { D3DRS_BLENDOPALPHA, HostToGuestFunction<SetRenderState<D3DRS_BLENDOPALPHA>> },
    { D3DRS_COLORWRITEENABLE, HostToGuestFunction<SetRenderState<D3DRS_COLORWRITEENABLE>> }
};

static std::unique_ptr<RenderShader> g_copyShader;

static std::unique_ptr<RenderShader> g_copyColorShader;
static ankerl::unordered_dense::map<RenderFormat, std::unique_ptr<RenderPipeline>> g_copyColorPipelines;
static std::unique_ptr<RenderPipeline> g_copyDepthPipeline;

static std::unique_ptr<RenderShader> g_resolveMsaaColorShaders[3];
static ankerl::unordered_dense::map<RenderFormat, std::array<std::unique_ptr<RenderPipeline>, 3>> g_resolveMsaaColorPipelines;
static std::unique_ptr<RenderPipeline> g_resolveMsaaDepthPipelines[3];

enum
{
    GAUSSIAN_BLUR_3X3,
    GAUSSIAN_BLUR_5X5,
    GAUSSIAN_BLUR_7X7,
    GAUSSIAN_BLUR_9X9,
    GAUSSIAN_BLUR_COUNT
};

static std::unique_ptr<GuestShader> g_gaussianBlurShaders[GAUSSIAN_BLUR_COUNT];

static std::unique_ptr<GuestShader> g_csdFilterShader;
static GuestShader* g_csdShader;

static std::unique_ptr<GuestShader> g_enhancedMotionBlurShader;

#ifdef UNLEASHED_RECOMP_D3D12

// VR single-pass stereo uses every host shader's VR_STEREO variant.
#define CREATE_SHADER_VARIANT(NAME) \
    g_device->createShader( \
        g_vulkan ? g_##NAME##_spirv : g_##NAME##_dxil, \
        g_vulkan ? sizeof(g_##NAME##_spirv) : sizeof(g_##NAME##_dxil), \
        "main", \
        g_vulkan ? RenderShaderFormat::SPIRV : RenderShaderFormat::DXIL)

#else

#define CREATE_SHADER_VARIANT(NAME) \
    g_device->createShader(g_##NAME##_spirv, sizeof(g_##NAME##_spirv), "main", RenderShaderFormat::SPIRV)

#endif

#define CREATE_SHADER(NAME) \
    (g_vrSinglePass ? CREATE_SHADER_VARIANT(NAME##_stereo) : CREATE_SHADER_VARIANT(NAME))

#ifdef _WIN32
static bool DetectWine()
{
    HMODULE dllHandle = GetModuleHandle("ntdll.dll");
    return dllHandle != nullptr && GetProcAddress(dllHandle, "wine_get_version") != nullptr;
}
#endif

static constexpr size_t TEXTURE_DESCRIPTOR_SIZE = 65536;
static constexpr size_t SAMPLER_DESCRIPTOR_SIZE = 1024;

static std::unique_ptr<GuestTexture> g_imFontTexture;
static std::unique_ptr<RenderPipelineLayout> g_imPipelineLayout;
static std::unique_ptr<RenderPipeline> g_imPipeline;
static std::unique_ptr<RenderPipeline> g_imAdditivePipeline;

template<typename T>
static void ExecuteCopyCommandList(const T& function)
{
    std::lock_guard lock(g_copyMutex);

    g_copyCommandList->begin();
    function();
    g_copyCommandList->end();
    g_copyQueue->executeCommandLists(g_copyCommandList.get(), g_copyCommandFence.get());
    g_copyQueue->waitForCommandFence(g_copyCommandFence.get());
}

static constexpr uint32_t PITCH_ALIGNMENT = 0x100;
static constexpr uint32_t PLACEMENT_ALIGNMENT = 0x200;

struct ImGuiPushConstants
{
    ImVec2 boundsMin{};
    ImVec2 boundsMax{};
    ImU32 gradientTopLeft{};
    ImU32 gradientTopRight{};
    ImU32 gradientBottomRight{};
    ImU32 gradientBottomLeft{};
    uint32_t shaderModifier{};
    uint32_t texture2DDescriptorIndex{};
    ImVec2 displaySize{};
    ImVec2 inverseDisplaySize{};
    ImVec2 origin{ 0.0f, 0.0f };
    ImVec2 scale{ 1.0f, 1.0f };
    ImVec2 proceduralOrigin{ 0.0f, 0.0f };
    float outline{};
};

extern ImFontBuilderIO g_fontBuilderIO;

// VR single-pass stereo: every shader reads 2D textures as arrays (see
// shader_common.h), so their views are array views. The texture's own view
// dimension (GuestTexture::viewDimension) stays TEXTURE_2D.
static RenderTextureViewDimension GetShaderViewDimension(RenderTextureViewDimension dimension)
{
    return (g_vrSinglePass && dimension == RenderTextureViewDimension::TEXTURE_2D) ? RenderTextureViewDimension::TEXTURE_2D_ARRAY : dimension;
}

// Points a descriptor at a 2D texture of one mip level that has no view of its
// own, through an array view kept in `view` in single-pass stereo.
static void SetTexture2DDescriptor(uint32_t descriptorIndex, RenderTexture* texture, RenderFormat format, std::unique_ptr<RenderTextureView>& view)
{
    view.reset();

    if (g_vrSinglePass)
    {
        RenderTextureViewDesc viewDesc;
        viewDesc.format = format;
        viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D_ARRAY;
        viewDesc.mipLevels = 1;
        view = texture->createTextureView(viewDesc);
    }

    g_textureDescriptorSet->setTexture(descriptorIndex, texture, RenderTextureLayout::SHADER_READ, view.get());
}

static void CreateImGuiBackend()
{
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

#ifdef ENABLE_IM_FONT_ATLAS_SNAPSHOT
    IM_DELETE(io.Fonts);
    io.Fonts = ImFontAtlasSnapshot::Load();
#else
    io.Fonts->AddFontDefault();
    ImFontAtlasSnapshot::GenerateGlyphRanges();
#endif

    InitImGuiUtils();
    AchievementMenu::Init();
    AchievementOverlay::Init();
    ButtonGuide::Init();
    MessageWindow::Init();
    OptionsMenu::Init();
    InstallerWizard::Init();

    ImGui_ImplSDL2_InitForOther(GameWindow::s_pWindow);

#ifdef ENABLE_IM_FONT_ATLAS_SNAPSHOT
    g_imFontTexture = LoadTexture(
        decompressZstd(g_im_font_atlas_texture, g_im_font_atlas_texture_uncompressed_size).get(), g_im_font_atlas_texture_uncompressed_size);
#else
    io.Fonts->FontBuilderIO = &g_fontBuilderIO;
    io.Fonts->Build();

    g_imFontTexture = std::make_unique<GuestTexture>(ResourceType::Texture);

    uint8_t* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    RenderTextureDesc textureDesc;
    textureDesc.dimension = RenderTextureDimension::TEXTURE_2D;
    textureDesc.width = width;
    textureDesc.height = height;
    textureDesc.depth = 1;
    textureDesc.mipLevels = 1;
    textureDesc.arraySize = 1;
    textureDesc.format = RenderFormat::R8G8B8A8_UNORM;

    g_imFontTexture->textureHolder = g_device->createTexture(textureDesc);
    g_imFontTexture->texture = g_imFontTexture->textureHolder.get();

    uint32_t rowPitch = (width * 4 + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
    uint32_t slicePitch = (rowPitch * height + PLACEMENT_ALIGNMENT - 1) & ~(PLACEMENT_ALIGNMENT - 1);
    auto uploadBuffer = g_device->createBuffer(RenderBufferDesc::UploadBuffer(slicePitch));
    uint8_t* mappedMemory = reinterpret_cast<uint8_t*>(uploadBuffer->map());

    if (rowPitch == (width * 4))
    {
        memcpy(mappedMemory, pixels, slicePitch);
    }
    else
    {
        for (size_t i = 0; i < height; i++)
        {
            memcpy(mappedMemory, pixels, width * 4);
            pixels += width * 4;
            mappedMemory += rowPitch;
        }
    }

    uploadBuffer->unmap();

    ExecuteCopyCommandList([&]
        {
            g_copyCommandList->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(g_imFontTexture->texture, RenderTextureLayout::COPY_DEST));

            g_copyCommandList->copyTextureRegion(
                RenderTextureCopyLocation::Subresource(g_imFontTexture->texture, 0),
                RenderTextureCopyLocation::PlacedFootprint(uploadBuffer.get(), RenderFormat::R8G8B8A8_UNORM, width, height, 1, rowPitch / 4, 0));
        });

    g_imFontTexture->layout = RenderTextureLayout::COPY_DEST;

    RenderTextureViewDesc textureViewDesc;
    textureViewDesc.format = textureDesc.format;
    textureViewDesc.dimension = GetShaderViewDimension(RenderTextureViewDimension::TEXTURE_2D);
    textureViewDesc.mipLevels = 1;
    g_imFontTexture->textureView = g_imFontTexture->texture->createTextureView(textureViewDesc);

    g_imFontTexture->descriptorIndex = g_textureDescriptorAllocator.allocate();
    g_textureDescriptorSet->setTexture(g_imFontTexture->descriptorIndex, g_imFontTexture->texture, RenderTextureLayout::SHADER_READ, g_imFontTexture->textureView.get());
#endif

    io.Fonts->SetTexID(g_imFontTexture.get());

    RenderPipelineLayoutBuilder pipelineLayoutBuilder;
    pipelineLayoutBuilder.begin(false, true);

    RenderDescriptorSetBuilder descriptorSetBuilder;
    descriptorSetBuilder.begin();
    descriptorSetBuilder.addTexture(0, TEXTURE_DESCRIPTOR_SIZE);
    descriptorSetBuilder.end(true, TEXTURE_DESCRIPTOR_SIZE);
    pipelineLayoutBuilder.addDescriptorSet(descriptorSetBuilder);

    descriptorSetBuilder.begin();
    descriptorSetBuilder.addSampler(0, SAMPLER_DESCRIPTOR_SIZE);
    descriptorSetBuilder.end(true, SAMPLER_DESCRIPTOR_SIZE);
    pipelineLayoutBuilder.addDescriptorSet(descriptorSetBuilder);

    pipelineLayoutBuilder.addPushConstant(0, 2, sizeof(ImGuiPushConstants), RenderShaderStageFlag::VERTEX | RenderShaderStageFlag::PIXEL);

    pipelineLayoutBuilder.end();
    g_imPipelineLayout = pipelineLayoutBuilder.create(g_device.get());

    auto vertexShader = CREATE_SHADER(imgui_vs);
    auto pixelShader = CREATE_SHADER(imgui_ps);

    RenderInputElement inputElements[3];
    inputElements[0] = RenderInputElement("POSITION", 0, 0, RenderFormat::R32G32_FLOAT, 0, offsetof(ImDrawVert, pos));
    inputElements[1] = RenderInputElement("TEXCOORD", 0, 1, RenderFormat::R32G32_FLOAT, 0, offsetof(ImDrawVert, uv));
    inputElements[2] = RenderInputElement("COLOR", 0, 2, RenderFormat::R8G8B8A8_UNORM, 0, offsetof(ImDrawVert, col));

    RenderInputSlot inputSlot(0, sizeof(ImDrawVert));

    RenderGraphicsPipelineDesc pipelineDesc;
    pipelineDesc.pipelineLayout = g_imPipelineLayout.get();
    pipelineDesc.vertexShader = vertexShader.get();
    pipelineDesc.pixelShader = pixelShader.get();
    pipelineDesc.renderTargetFormat[0] = BACKBUFFER_FORMAT;
    pipelineDesc.renderTargetBlend[0] = RenderBlendDesc::AlphaBlend();
    pipelineDesc.renderTargetCount = 1;
    pipelineDesc.inputElements = inputElements;
    pipelineDesc.inputElementsCount = std::size(inputElements);
    pipelineDesc.inputSlots = &inputSlot;
    pipelineDesc.inputSlotsCount = 1;
    pipelineDesc.viewportCount = GetVRViewportCount();
    g_imPipeline = g_device->createGraphicsPipeline(pipelineDesc);

    pipelineDesc.renderTargetBlend[0].dstBlend = RenderBlend::ONE;
    pipelineDesc.viewportCount = GetVRViewportCount();
    g_imAdditivePipeline = g_device->createGraphicsPipeline(pipelineDesc);

#ifndef ENABLE_IM_FONT_ATLAS_SNAPSHOT
    ImFontAtlasSnapshot snapshot;
    snapshot.Snap();

    FILE* file = fopen("im_font_atlas.bin", "wb");
    if (file)
    {
        fwrite(snapshot.data.data(), 1, snapshot.data.size(), file);
        fclose(file);
    }

    ddspp::Header header;
    ddspp::HeaderDXT10 headerDX10;
    ddspp::encode_header(ddspp::R8G8B8A8_UNORM, width, height, 1, ddspp::Texture2D, 1, 1, header, headerDX10);

    file = fopen("im_font_atlas.dds", "wb");
    if (file)
    {
        fwrite(&ddspp::DDS_MAGIC, 4, 1, file);
        fwrite(&header, sizeof(header), 1, file);
        fwrite(&headerDX10, sizeof(headerDX10), 1, file);
        fwrite(pixels, 4, width * height, file);
        fclose(file);
    }
#endif
}

static void CheckSwapChain()
{
    // In VR the headset paces the game; the window is only a mirror and must never block on vsync.
    g_swapChain->setVsyncEnabled(Config::VSync && !VR::IsActive());
    g_swapChainValid &= !g_swapChain->needsResize();

    if (!g_swapChainValid)
    {
        Video::WaitForGPU();
        g_backBuffer->framebuffers.clear();
        g_swapChainValid = g_swapChain->resize();

        // In VR the game's resolution does not follow the window.
        g_needsResize = g_swapChainValid && !VR::IsActive();
    }

    if (g_swapChainValid)
    {
        g_swapChainAcquireProfiler.Begin();
        g_swapChainValid = g_swapChain->acquireTexture(g_acquireSemaphores[g_frame].get(), &g_backBufferIndex);
        g_swapChainAcquireProfiler.End();
    }

    if (g_needsResize)
        Video::ComputeViewportDimensions();

    g_backBuffer->width = Video::s_viewportWidth;
    g_backBuffer->height = Video::s_viewportHeight;
}

static void BeginCommandList()
{
    g_renderTarget = g_backBuffer;
    g_depthStencil = nullptr;
    g_framebuffer = nullptr;

    g_pipelineState.renderTargetFormat = BACKBUFFER_FORMAT;
    g_pipelineState.depthStencilFormat = RenderFormat::UNKNOWN;

    // In VR the game always renders into the intermediary texture, which is then
    // copied to the headset, also while the window is minimised.
    if (g_swapChainValid || VR::IsActive())
    {
        uint32_t width = Video::s_viewportWidth;
        uint32_t height = Video::s_viewportHeight;

        bool usingIntermediaryTexture = VR::IsActive() || (width != g_swapChain->getWidth()) || (height != g_swapChain->getHeight()) ||
            Config::XboxColorCorrection || (abs(Config::Brightness - 0.5f) > 0.001f);

        if (usingIntermediaryTexture)
        {
            if (g_intermediaryBackBufferTextureWidth != width ||
                g_intermediaryBackBufferTextureHeight != height)
            {
                if (g_intermediaryBackBufferTextureDescriptorIndex == NULL)
                    g_intermediaryBackBufferTextureDescriptorIndex = g_textureDescriptorAllocator.allocate();

                Video::WaitForGPU(); // Fine to wait for GPU, this'll only happen during resize.

                // Single-pass stereo: a layer per eye.
                RenderTextureDesc textureDesc = RenderTextureDesc::Texture2D(width, height, 1, BACKBUFFER_FORMAT, RenderTextureFlag::RENDER_TARGET);
                textureDesc.arraySize = GetVRViewportCount();

                g_intermediaryBackBufferTextureView.reset();
                g_intermediaryBackBufferTexture = g_device->createTexture(textureDesc);
                SetTexture2DDescriptor(g_intermediaryBackBufferTextureDescriptorIndex, g_intermediaryBackBufferTexture.get(), BACKBUFFER_FORMAT, g_intermediaryBackBufferTextureView);

                g_intermediaryBackBufferTextureWidth = width;
                g_intermediaryBackBufferTextureHeight = height;

                g_backBuffer->framebuffers.clear();
            }

            g_backBuffer->texture = g_intermediaryBackBufferTexture.get();
            g_backBuffer->layerCount = GetVRViewportCount();
        }
        else
        {
            g_backBuffer->texture = g_swapChain->getTexture(g_backBufferIndex);
            g_backBuffer->layerCount = 1;
        }

        // VR: the output texture has the headset images' size, the game's picture
        // being stretched into it; the UI texture has the back buffer's size and
        // format, so ImGui's pipelines draw into it.
        uint32_t outputWidth = VR::GetRenderWidth();
        uint32_t outputHeight = VR::GetRenderHeight();

        if (VR::IsActive() && (g_vrOutputTextureWidth != outputWidth || g_vrOutputTextureHeight != outputHeight))
        {
            if (g_vrOutputTextureDescriptorIndex == NULL)
                g_vrOutputTextureDescriptorIndex = g_textureDescriptorAllocator.allocate();

            Video::WaitForGPU(); // Only when the headset's resolution changes.

            g_vrOutputFramebuffer.reset();
            g_vrOutputTextureView.reset();
            g_vrOutputTexture = g_device->createTexture(RenderTextureDesc::Texture2D(outputWidth, outputHeight, 1, VR::GetImageFormat(), RenderTextureFlag::RENDER_TARGET));
            SetTexture2DDescriptor(g_vrOutputTextureDescriptorIndex, g_vrOutputTexture.get(), VR::GetImageFormat(), g_vrOutputTextureView);

            RenderFramebufferDesc framebufferDesc;
            const RenderTexture* colorAttachment = g_vrOutputTexture.get();
            framebufferDesc.colorAttachments = &colorAttachment;
            framebufferDesc.colorAttachmentsCount = 1;
            g_vrOutputFramebuffer = g_device->createFramebuffer(framebufferDesc);

            g_vrOutputTextureWidth = outputWidth;
            g_vrOutputTextureHeight = outputHeight;
        }

        if (VR::IsActive() && (g_vrUiTextureWidth != width || g_vrUiTextureHeight != height))
        {
            if (g_vrUiTextureDescriptorIndex == NULL)
                g_vrUiTextureDescriptorIndex = g_textureDescriptorAllocator.allocate();

            Video::WaitForGPU(); // Only when the screen resolution changes.

            g_vrUiFramebuffer.reset();
            g_vrUiTextureView.reset();
            g_vrUiTexture = g_device->createTexture(RenderTextureDesc::Texture2D(width, height, 1, BACKBUFFER_FORMAT, RenderTextureFlag::RENDER_TARGET));
            SetTexture2DDescriptor(g_vrUiTextureDescriptorIndex, g_vrUiTexture.get(), BACKBUFFER_FORMAT, g_vrUiTextureView);

            RenderFramebufferDesc framebufferDesc;
            const RenderTexture* colorAttachment = g_vrUiTexture.get();
            framebufferDesc.colorAttachments = &colorAttachment;
            framebufferDesc.colorAttachmentsCount = 1;
            g_vrUiFramebuffer = g_device->createFramebuffer(framebufferDesc);

            g_vrUiTextureWidth = width;
            g_vrUiTextureHeight = height;
        }
    }
    else
    {
        g_backBuffer->texture = g_backBuffer->textureHolder.get();
        g_backBuffer->layerCount = 1;
    }

    g_backBuffer->layout = RenderTextureLayout::UNKNOWN;

    for (size_t i = 0; i < 16; i++)
    {
        g_sharedConstants.texture2DIndices[i] = TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D;
        g_sharedConstants.texture3DIndices[i] = TEXTURE_DESCRIPTOR_NULL_TEXTURE_3D;
        g_sharedConstants.textureCubeIndices[i] = TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE;
    }

    memset(g_textures, 0, sizeof(g_textures));

    if (Config::GITextureFiltering == EGITextureFiltering::Bicubic)
        g_pipelineState.specConstants |= SPEC_CONSTANT_BICUBIC_GI_FILTER;
    else
        g_pipelineState.specConstants &= ~SPEC_CONSTANT_BICUBIC_GI_FILTER;

    auto& commandList = g_commandLists[g_frame];

    commandList->begin();
    commandList->resetQueryPool(g_queryPools[g_frame].get(), 0, NUM_QUERIES);
    commandList->writeTimestamp(g_queryPools[g_frame].get(), 0);

    // VR: the frame's GPU segments (see GpuSegmentKind), and no shading rate image.
    g_gpuSegmentKind = -1;
    g_boundShadingRateImage = nullptr;

    // Vulkan: the shading rate attachment starts at the full rate everywhere,
    // in the layout the render passes expect it in.
    if (g_vrShadingRateAttachment != nullptr && !g_vrShadingRateAttachmentReady)
    {
        uint32_t size = g_vrShadingRateAttachmentSize;
        uint32_t pitch = (size + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
        auto allocation = g_uploadAllocators[g_frame].allocate(pitch * size, PLACEMENT_ALIGNMENT);
        memset(allocation.memory, 0, pitch * size);

        commandList->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(g_vrShadingRateAttachment.get(), RenderTextureLayout::COPY_DEST));

        for (uint32_t layer = 0; layer < g_vrShadingRateAttachmentLayers; layer++)
        {
            commandList->copyTextureRegion(
                RenderTextureCopyLocation::Subresource(g_vrShadingRateAttachment.get(), 0, layer),
                RenderTextureCopyLocation::PlacedFootprint(allocation.buffer, RenderFormat::R8_UINT, size, size, 1, pitch, allocation.offset));
        }

        commandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(g_vrShadingRateAttachment.get(), RenderTextureLayout::SHADING_RATE_SOURCE));

        g_vrShadingRateAttachmentReady = true;
    }

    if (VR::IsActive())
    {
        commandList->resetQueryPool(g_segmentQueryPools[g_frame].get(), 0, NUM_SEGMENT_QUERIES);
        g_gpuSegmentCounts[g_frame] = 0;
    }
    else
    {
        g_gpuSegmentCounts[g_frame] = GPU_SEGMENTS_UNUSED;
    }
    commandList->setGraphicsPipelineLayout(g_pipelineLayout.get());
    commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 0);
    commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 1);
    commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 2);
    commandList->setGraphicsDescriptorSet(g_samplerDescriptorSet.get(), 3);

    if (VR::IsActive())
        BeginVRFrame();

    g_readyForCommands = true;
    g_readyForCommands.notify_one();
}

template<typename T>
static void ApplyLowEndDefault(ConfigDef<T> &configDef, T newDefault, bool &changed)
{
    if (configDef.IsDefaultValue() && !configDef.IsLoadedFromConfig)
    {
        configDef = newDefault;
        changed = true;
    }
    
    configDef.DefaultValue = newDefault;
}

static void ApplyLowEndDefaults()
{
    bool changed = false;

    ApplyLowEndDefault(Config::AntiAliasing, EAntiAliasing::MSAA2x, changed);
    ApplyLowEndDefault(Config::ShadowResolution, EShadowResolution::Original, changed);
    ApplyLowEndDefault(Config::TransparencyAntiAliasing, false, changed);
    ApplyLowEndDefault(Config::GITextureFiltering, EGITextureFiltering::Bilinear, changed);

    if (changed) 
    {
        Config::Save();
    }
}

bool Video::CreateHostDevice(const char *sdlVideoDriver, bool graphicsApiRetry)
{
    for (uint32_t i = 0; i < 16; i++)
        g_inputSlots[i].index = i;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    GameWindow::Init(sdlVideoDriver);

    // OpenXR must find the headset before the device exists: the runtime decides
    // the GPU, and for Vulkan the extensions plume enables. The session uses the
    // graphics API the game would pick (Vulkan under Wine or when chosen, D3D12
    // otherwise); a retry after a crash stays flat.
    bool vrVulkan = false;
#ifdef UNLEASHED_RECOMP_D3D12
    vrVulkan = DetectWine() || Config::GraphicsAPI == EGraphicsAPI::Vulkan;
#endif
    if (VR::s_requested && !graphicsApiRetry)
        VR::Init(vrVulkan);

#ifdef UNLEASHED_RECOMP_D3D12
    g_vulkan = DetectWine() || Config::GraphicsAPI == EGraphicsAPI::Vulkan;
#endif

    // Attempt to create the possible backends using a vector of function pointers. Whichever succeeds first will be the chosen API.
    using RenderInterfaceFunction = std::unique_ptr<RenderInterface>(void);
    std::vector<RenderInterfaceFunction *> interfaceFunctions;

#ifdef UNLEASHED_RECOMP_D3D12
    // Never silently move a VR session to Vulkan.
    bool allowVulkanRedirection = !VR::IsInitialized();

    if (graphicsApiRetry)
    {
        // If we are attempting to create again after a reboot due to a crash, swap the order.
        g_vulkan = !g_vulkan;

        // Don't allow redirection to Vulkan if we are retrying after a crash, 
        // so the user can at least boot the game with D3D12 if Vulkan fails to work.
        allowVulkanRedirection = false;
    }

    interfaceFunctions.push_back(g_vulkan ? CreateVulkanInterfaceWrapper : CreateD3D12Interface);
    interfaceFunctions.push_back(g_vulkan ? CreateD3D12Interface : CreateVulkanInterfaceWrapper);
#else
    interfaceFunctions.push_back(CreateVulkanInterfaceWrapper);
#endif

    for (size_t i = 0; i < interfaceFunctions.size(); i++)
    {
        RenderInterfaceFunction* interfaceFunction = interfaceFunctions[i];

#ifdef UNLEASHED_RECOMP_D3D12
        // Wrap the device creation in __try/__except to survive from driver crashes.
        __try
#endif
        {
            g_interface = interfaceFunction();
            if (g_interface == nullptr)
            {
                continue;
            }

            // The VR session's API on the GPU the runtime requires.
            std::string vrAdapterName;
            bool vulkanInterface = (interfaceFunction == CreateVulkanInterfaceWrapper);
            if (VR::IsInitialized() && vulkanInterface == vrVulkan)
                vrAdapterName = VR::GetAdapterName(g_interface.get());

            g_device = g_interface->createDevice(!vrAdapterName.empty() ? vrAdapterName : Config::GraphicsDevice.Value);
            if (g_device != nullptr)
            {
                const RenderDeviceDescription &deviceDescription = g_device->getDescription();
                
#ifdef UNLEASHED_RECOMP_D3D12
                if (interfaceFunction == CreateD3D12Interface)
                {
                    if (allowVulkanRedirection)
                    {
                        bool redirectToVulkan = false;

                        if (deviceDescription.vendor == RenderDeviceVendor::AMD)
                        {
                            // AMD Drivers before this version have a known issue where MSAA resolve targets will fail to work correctly.
                            // If no specific graphics API was selected, we silently destroy this one and move to the next option as it'll
                            // just work incorrectly otherwise and result in visual glitches and 3D rendering not working in general.
                            constexpr uint64_t MinimumAMDDriverVersion = 0x1F00005DC2005CULL; // 31.0.24002.92
                            if ((Config::GraphicsAPI == EGraphicsAPI::Auto) && (deviceDescription.driverVersion < MinimumAMDDriverVersion))
                                redirectToVulkan = true;
                        }
                        else if (deviceDescription.vendor == RenderDeviceVendor::INTEL)
                        {
                            // Intel drivers on D3D12 are extremely buggy, introducing various graphical glitches.
                            // We will redirect users to Vulkan until a workaround can be found.
                            if (Config::GraphicsAPI == EGraphicsAPI::Auto)
                                redirectToVulkan = true;
                        }

                        if (redirectToVulkan)
                        {
                            g_device.reset();
                            g_interface.reset();

                            // In case Vulkan fails to initialize, we will try D3D12 again afterwards, 
                            // just to get the game to boot. This only really happens in very old Intel GPU drivers.
                            if (!g_vulkan)
                            {
                                interfaceFunctions.push_back(CreateD3D12Interface);
                                allowVulkanRedirection = false;
                            }

                            continue;
                        }
                    }

                    // Hardware resolve seems to be completely bugged on Intel D3D12 drivers.
                    g_hardwareResolve = (deviceDescription.vendor != RenderDeviceVendor::INTEL);
                    g_hardwareDepthResolve = (deviceDescription.vendor != RenderDeviceVendor::INTEL);
                }

                g_vulkan = (interfaceFunction == CreateVulkanInterfaceWrapper);
#endif
                // Enable triangle strip workaround if we are on AMD, as there is a bug where
                // restart indices cause triangles to be culled incorrectly. Converting them to degenerate triangles fixes it.
                g_triangleStripWorkaround = (deviceDescription.vendor == RenderDeviceVendor::AMD);

                break;
            }
        }
#ifdef UNLEASHED_RECOMP_D3D12
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            if (graphicsApiRetry)
            {
                // If we were retrying, and this also failed, then we'll show the user neither of the graphics APIs succeeded.
                return false;
            }
            else
            {
                // If this is the first crash we ran into, reboot and try the other graphics API.
                os::process::StartProcess(os::process::GetExecutablePath(), { "--graphics-api-retry" });
                std::_Exit(0);
            }
        }
#endif
    }

    if (g_device == nullptr)
    {
        VR::Shutdown();
        return false;
    }

    if (VR::IsInitialized() && g_vulkan != vrVulkan)
    {
        LOGFN_WARNING("[VR] {} could not be used for the VR session. The game will run without VR.", vrVulkan ? "Vulkan" : "D3D12");
        VR::Shutdown();
    }

#ifdef UNLEASHED_RECOMP_D3D12
    if (graphicsApiRetry)
    {
        // If we managed to create a device after retrying it in a reboot, remember the one we picked.
        Config::GraphicsAPI = g_vulkan ? EGraphicsAPI::Vulkan : EGraphicsAPI::D3D12;
    }
#endif

    g_capabilities = g_device->getCapabilities();

    // VR on Vulkan: the shading rate attachment every framebuffer takes (see VRFoveation).
    if (g_capabilities.shadingRateAttachment && g_capabilities.shadingRateImageTileSize != 0)
    {
        uint32_t tileSize = g_capabilities.shadingRateImageTileSize;
        g_vrShadingRateAttachmentSize = (uint32_t(std::min<uint64_t>(g_capabilities.maxTextureSize, 16384)) + tileSize - 1) / tileSize;

        g_vrShadingRateAttachmentLayers = g_capabilities.shadingRateImageLayers ? 2 : 1;

        RenderTextureDesc attachmentDesc = RenderTextureDesc::Texture2D(
            g_vrShadingRateAttachmentSize, g_vrShadingRateAttachmentSize, 1, RenderFormat::R8_UINT, RenderTextureFlag::SHADING_RATE);
        attachmentDesc.arraySize = g_vrShadingRateAttachmentLayers;
        g_vrShadingRateAttachment = g_device->createTexture(attachmentDesc);

        g_device->setShadingRateAttachment(g_vrShadingRateAttachment.get());
    }

    LoadEmbeddedResources();

    constexpr uint64_t LowEndMemoryLimit = 2048ULL * 1024ULL * 1024ULL;
    RenderDeviceDescription deviceDescription = g_device->getDescription();
    bool lowEndType = deviceDescription.type != RenderDeviceType::UNKNOWN && deviceDescription.type != RenderDeviceType::DISCRETE;
    bool lowEndMemory = deviceDescription.dedicatedVideoMemory < LowEndMemoryLimit;
    bool lowEndUMA = deviceDescription.type == RenderDeviceType::UNKNOWN && g_capabilities.uma;
    if (lowEndType || lowEndMemory || lowEndUMA)
    {
        // Switch to low end defaults if a non-discrete GPU was detected or a low amount of VRAM was detected.
        // Checking for UMA on D3D12 seems to be a reliable way to detect integrated GPUs.
        ApplyLowEndDefaults();
    }

    const RenderSampleCounts colourSampleCount = g_device->getSampleCountsSupported(RenderFormat::R16G16B16A16_FLOAT);
    const RenderSampleCounts depthSampleCount  = g_device->getSampleCountsSupported(RenderFormat::D32_FLOAT);
    const RenderSampleCounts commonSampleCount = colourSampleCount & depthSampleCount;

    // Disable specific MSAA levels if they are not supported.
    if ((commonSampleCount & RenderSampleCount::COUNT_2) == 0)
        Config::AntiAliasing.InaccessibleValues.emplace(EAntiAliasing::MSAA2x);
    if ((commonSampleCount & RenderSampleCount::COUNT_4) == 0)
        Config::AntiAliasing.InaccessibleValues.emplace(EAntiAliasing::MSAA4x);
    if ((commonSampleCount & RenderSampleCount::COUNT_8) == 0)
        Config::AntiAliasing.InaccessibleValues.emplace(EAntiAliasing::MSAA8x);

    // Set Anti-Aliasing to nearest supported level.
    Config::AntiAliasing.SnapToNearestAccessibleValue(false);

    g_queue = g_device->createCommandQueue(RenderCommandListType::DIRECT);

    for (auto& commandList : g_commandLists)
        commandList = g_queue->createCommandList();

    for (auto& commandFence : g_commandFences)
        commandFence = g_device->createCommandFence();

    for (auto& queryPool : g_queryPools)
        queryPool = g_device->createQueryPool(NUM_QUERIES);

    for (auto& queryPool : g_segmentQueryPools)
        queryPool = g_device->createQueryPool(NUM_SEGMENT_QUERIES);

    g_copyQueue = g_device->createCommandQueue(RenderCommandListType::COPY);
    g_copyCommandList = g_copyQueue->createCommandList();
    g_copyCommandFence = g_device->createCommandFence();

    // The session uses the direct queue, so the runtime orders its reads of the
    // headset images after the render thread's copies into them.
    if (VR::IsInitialized() && !VR::CreateSession(g_device.get(), g_queue.get()))
    {
        LOGN_WARNING("[VR] The OpenXR session could not be created. The game will run without VR.");
        VR::Shutdown();
    }

    if (VR::IsInitialized())
    {
        LOGFN("[VR] Layer and viewport selection in vertex shaders: {}.",
            g_capabilities.vertexShaderLayerAndViewport ? "supported" : "not supported");

        g_vrSinglePass = Config::VRSinglePass && g_capabilities.vertexShaderLayerAndViewport;
        g_vrPartialConstantUploads = Config::VRPartialConstantUploads;

        LOGFN("[VR] Variable rate shading from an image: {} (tiles of {} pixels, 4x4 {}).",
            g_capabilities.shadingRateImage ? "supported" : "not supported", g_capabilities.shadingRateImageTileSize,
            g_capabilities.shadingRateImageCoarseRates ? "available" : "unavailable");

        if (g_vrSinglePass)
        {
            LOGN("[VR] Single-pass stereo: both eyes are drawn at once.");
            LoadStereoShaderCache();
        }
        else if (Config::VRSinglePass)
        {
            LOGN_WARNING("[VR] Single-pass stereo needs layer and viewport selection in vertex shaders; the eyes are drawn one after the other.");
        }
    }

    uint32_t bufferCount = 2;

    switch (Config::TripleBuffering)
    {
    case ETripleBuffering::Auto:
        if (g_vulkan)
        {
            // Defaulting to 3 is fine if presentWait as supported, as the maximum frame latency allowed is only 1.
            bufferCount = g_device->getCapabilities().presentWait ? 3 : 2;
        }
        else
        {
            // Defaulting to 3 is fine on D3D12 thanks to flip discard model.
            bufferCount = 3;
        }

        break;
    case ETripleBuffering::On:
        bufferCount = 3;
        break;
    case ETripleBuffering::Off:
        bufferCount = 2;
        break;
    }

    g_swapChain = g_queue->createSwapChain(GameWindow::s_renderWindow, bufferCount, BACKBUFFER_FORMAT, Config::MaxFrameLatency);
    g_swapChain->setVsyncEnabled(Config::VSync);
    g_swapChainValid = !g_swapChain->needsResize();

    for (auto& acquireSemaphore : g_acquireSemaphores)
        acquireSemaphore = g_device->createCommandSemaphore();
    
    for (auto& renderSemaphore : g_renderSemaphores)
        renderSemaphore = g_device->createCommandSemaphore();

    RenderPipelineLayoutBuilder pipelineLayoutBuilder;
    pipelineLayoutBuilder.begin(false, true);
    
    RenderDescriptorSetBuilder descriptorSetBuilder;
    descriptorSetBuilder.begin();
    descriptorSetBuilder.addTexture(0, TEXTURE_DESCRIPTOR_SIZE);
    descriptorSetBuilder.end(true, TEXTURE_DESCRIPTOR_SIZE);
    
    g_textureDescriptorSet = descriptorSetBuilder.create(g_device.get());
    
    for (size_t i = 0; i < TEXTURE_DESCRIPTOR_NULL_COUNT; i++)
    {
        auto& texture = g_blankTextures[i];
        auto& textureView = g_blankTextureViews[i];

        RenderTextureDesc desc;
        desc.width = 1;
        desc.height = 1;
        desc.depth = 1;
        desc.mipLevels = 1;
        desc.format = RenderFormat::R8_UNORM;

        RenderTextureViewDesc viewDesc;
        viewDesc.format = desc.format;
        viewDesc.componentMapping = RenderComponentMapping(RenderSwizzle::ZERO, RenderSwizzle::ZERO, RenderSwizzle::ZERO, RenderSwizzle::ZERO);
        viewDesc.mipLevels = 1;

        switch (i)
        {
        case TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D:
            desc.dimension = RenderTextureDimension::TEXTURE_2D;
            desc.arraySize = 1;
            viewDesc.dimension = GetShaderViewDimension(RenderTextureViewDimension::TEXTURE_2D);
            break;

        case TEXTURE_DESCRIPTOR_NULL_TEXTURE_3D:
            desc.dimension = RenderTextureDimension::TEXTURE_3D;
            desc.arraySize = 1;
            viewDesc.dimension = RenderTextureViewDimension::TEXTURE_3D;
            break;

        case TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE:
            desc.dimension = RenderTextureDimension::TEXTURE_2D;
            desc.arraySize = 6;
            desc.flags = RenderTextureFlag::CUBE;
            viewDesc.dimension = RenderTextureViewDimension::TEXTURE_CUBE;
            break;

        default:
            assert(false && "Unknown null descriptor dimension");
            break;
        }

        texture = g_device->createTexture(desc);
        textureView = texture->createTextureView(viewDesc);

        g_textureDescriptorSet->setTexture(i, texture.get(), RenderTextureLayout::SHADER_READ, textureView.get());
    }

    pipelineLayoutBuilder.addDescriptorSet(descriptorSetBuilder);
    pipelineLayoutBuilder.addDescriptorSet(descriptorSetBuilder);
    pipelineLayoutBuilder.addDescriptorSet(descriptorSetBuilder);
    
    descriptorSetBuilder.begin();
    descriptorSetBuilder.addSampler(0, SAMPLER_DESCRIPTOR_SIZE);
    descriptorSetBuilder.end(true, SAMPLER_DESCRIPTOR_SIZE);
    
    g_samplerDescriptorSet = descriptorSetBuilder.create(g_device.get());
    auto& [descriptorIndex, sampler] = g_samplerStates[XXH3_64bits(&g_samplerDescs[0], sizeof(RenderSamplerDesc))];
    descriptorIndex = 1;
    sampler = g_device->createSampler(g_samplerDescs[0]);
    g_samplerDescriptorSet->setSampler(0, sampler.get());

    pipelineLayoutBuilder.addDescriptorSet(descriptorSetBuilder);

    if (g_vulkan)
    {
        pipelineLayoutBuilder.addPushConstant(0, 4, 24, RenderShaderStageFlag::VERTEX | RenderShaderStageFlag::PIXEL);
    }
    else
    {
        pipelineLayoutBuilder.addRootDescriptor(0, 4, RenderRootDescriptorType::CONSTANT_BUFFER);
        pipelineLayoutBuilder.addRootDescriptor(1, 4, RenderRootDescriptorType::CONSTANT_BUFFER);
        pipelineLayoutBuilder.addRootDescriptor(2, 4, RenderRootDescriptorType::CONSTANT_BUFFER);
        pipelineLayoutBuilder.addPushConstant(3, 4, 4, RenderShaderStageFlag::PIXEL); // For copy/resolve shaders.
    }
    pipelineLayoutBuilder.end();
    
    g_pipelineLayout = pipelineLayoutBuilder.create(g_device.get());

    g_copyShader = CREATE_SHADER(copy_vs);
    g_copyColorShader = CREATE_SHADER(copy_color_ps);
    auto copyDepthShader = CREATE_SHADER(copy_depth_ps);

    RenderGraphicsPipelineDesc desc;
    desc.pipelineLayout = g_pipelineLayout.get();
    desc.vertexShader = g_copyShader.get();
    desc.pixelShader = copyDepthShader.get();
    desc.depthFunction = RenderComparisonFunction::ALWAYS;
    desc.depthEnabled = true;
    desc.depthWriteEnabled = true;
    desc.depthTargetFormat = RenderFormat::D32_FLOAT;
    desc.viewportCount = GetVRViewportCount();
    g_copyDepthPipeline = g_device->createGraphicsPipeline(desc);

    g_resolveMsaaColorShaders[0] = CREATE_SHADER(resolve_msaa_color_2x);
    g_resolveMsaaColorShaders[1] = CREATE_SHADER(resolve_msaa_color_4x);
    g_resolveMsaaColorShaders[2] = CREATE_SHADER(resolve_msaa_color_8x);

    for (size_t i = 0; i < std::size(g_resolveMsaaDepthPipelines); i++)
    {
        std::unique_ptr<RenderShader> pixelShader;
        switch (i)
        {
        case 0:
            pixelShader = CREATE_SHADER(resolve_msaa_depth_2x);
            break;
        case 1:
            pixelShader = CREATE_SHADER(resolve_msaa_depth_4x);
            break;
        case 2:
            pixelShader = CREATE_SHADER(resolve_msaa_depth_8x);
            break;
        }

        desc = {};
        desc.pipelineLayout = g_pipelineLayout.get();
        desc.vertexShader = g_copyShader.get();
        desc.pixelShader = pixelShader.get();
        desc.depthFunction = RenderComparisonFunction::ALWAYS;
        desc.depthEnabled = true;
        desc.depthWriteEnabled = true;
        desc.depthTargetFormat = RenderFormat::D32_FLOAT;
        desc.viewportCount = GetVRViewportCount();
        g_resolveMsaaDepthPipelines[i] = g_device->createGraphicsPipeline(desc);
    }

    for (auto& shader : g_gaussianBlurShaders)
        shader = std::make_unique<GuestShader>(ResourceType::PixelShader);

    g_gaussianBlurShaders[GAUSSIAN_BLUR_3X3]->shader = CREATE_SHADER(gaussian_blur_3x3);
    g_gaussianBlurShaders[GAUSSIAN_BLUR_5X5]->shader = CREATE_SHADER(gaussian_blur_5x5);
    g_gaussianBlurShaders[GAUSSIAN_BLUR_7X7]->shader = CREATE_SHADER(gaussian_blur_7x7);
    g_gaussianBlurShaders[GAUSSIAN_BLUR_9X9]->shader = CREATE_SHADER(gaussian_blur_9x9);

    g_csdFilterShader = std::make_unique<GuestShader>(ResourceType::PixelShader);
    g_csdFilterShader->shader = CREATE_SHADER(csd_filter_ps);

    g_enhancedMotionBlurShader = std::make_unique<GuestShader>(ResourceType::PixelShader);
    g_enhancedMotionBlurShader->shader = CREATE_SHADER(enhanced_motion_blur_ps);

    CreateImGuiBackend();

    auto gammaCorrectionShader = CREATE_SHADER(gamma_correction_ps);

    desc = {};
    desc.pipelineLayout = g_pipelineLayout.get();
    desc.vertexShader = g_copyShader.get();
    desc.pixelShader = gammaCorrectionShader.get();
    desc.renderTargetFormat[0] = BACKBUFFER_FORMAT;
    desc.renderTargetBlend[0] = RenderBlendDesc::Copy();
    desc.renderTargetCount = 1;
    desc.viewportCount = GetVRViewportCount();
    g_gammaCorrectionPipeline = g_device->createGraphicsPipeline(desc);

    if (VR::IsActive())
    {
        auto outputShader = CREATE_SHADER(xr_output_ps);

        desc = {};
        desc.pipelineLayout = g_pipelineLayout.get();
        desc.vertexShader = g_copyShader.get();
        desc.pixelShader = outputShader.get();
        desc.renderTargetFormat[0] = VR::GetImageFormat();
        desc.renderTargetBlend[0] = RenderBlendDesc::Copy();
        desc.renderTargetCount = 1;
        desc.viewportCount = GetVRViewportCount();
        g_vrOutputPipeline = g_device->createGraphicsPipeline(desc);

        auto mirrorShader = CREATE_SHADER(xr_blit_ps);

        desc = {};
        desc.pipelineLayout = g_pipelineLayout.get();
        desc.vertexShader = g_copyShader.get();
        desc.pixelShader = mirrorShader.get();
        desc.renderTargetFormat[0] = BACKBUFFER_FORMAT;
        desc.renderTargetBlend[0] = RenderBlendDesc::Copy();
        desc.renderTargetCount = 1;
        desc.viewportCount = GetVRViewportCount();
        g_vrMirrorPipeline = g_device->createGraphicsPipeline(desc);

        // The UI texture holds premultiplied colours.
        auto uiVertexShader = CREATE_SHADER(xr_ui_vs);
        auto uiPixelShader = CREATE_SHADER(xr_ui_ps);

        desc.vertexShader = uiVertexShader.get();
        desc.pixelShader = uiPixelShader.get();
        desc.renderTargetBlend[0] = RenderBlendDesc::AlphaBlend();
        desc.renderTargetBlend[0].srcBlend = RenderBlend::ONE;
        desc.viewportCount = GetVRViewportCount();
        g_vrUiCompositePipeline = g_device->createGraphicsPipeline(desc);
    }

    // NOTE: We initially allocate this on host memory to make the installer work, even if the 4 GB memory allocation fails.
    g_backBufferHolder = std::make_unique<GuestSurface>(ResourceType::RenderTarget);

    g_backBuffer = g_backBufferHolder.get();
    g_backBuffer->width = 1280;
    g_backBuffer->height = 720;
    g_backBuffer->format = BACKBUFFER_FORMAT;
    g_backBuffer->textureHolder = g_device->createTexture(RenderTextureDesc::Texture2D(1, 1, 1, BACKBUFFER_FORMAT, RenderTextureFlag::RENDER_TARGET));

    Video::ComputeViewportDimensions();
    CheckSwapChain();
    BeginCommandList();

    RenderTextureBarrier blankTextureBarriers[TEXTURE_DESCRIPTOR_NULL_COUNT];
    for (size_t i = 0; i < TEXTURE_DESCRIPTOR_NULL_COUNT; i++)
        blankTextureBarriers[i] = RenderTextureBarrier(g_blankTextures[i].get(), RenderTextureLayout::SHADER_READ);

    g_commandLists[g_frame]->barriers(RenderBarrierStage::NONE, blankTextureBarriers, std::size(blankTextureBarriers));

    return true;
}

static uint32_t g_waitForGPUCount = 0;

void Video::WaitForGPU()
{
    g_waitForGPUCount++;

    // Wait for all queued frames to finish.
    for (size_t i = 0; i < NUM_FRAMES; i++)
    {
        if (g_commandListStates[i])
        {
            g_queue->waitForCommandFence(g_commandFences[i].get());
            g_commandListStates[i] = false;
        }
    }

    // Execute an empty command list and wait for it to end to guarantee that any remaining presentation has finished.
    g_commandLists[0]->begin();
    g_commandLists[0]->end();
    g_queue->executeCommandLists(g_commandLists[0].get(), g_commandFences[0].get());
    g_queue->waitForCommandFence(g_commandFences[0].get());
}

static uint32_t CreateDevice(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, be<uint32_t>* a6)
{
    g_xdbfTextureCache = std::unordered_map<uint16_t, GuestTexture *>();

    for (auto &achievement : g_xdbfWrapper.GetAchievements(XDBF_LANGUAGE_ENGLISH))
    {
        // huh?
        if (!achievement.pImageBuffer || !achievement.ImageBufferSize)
            continue;

        g_xdbfTextureCache[achievement.ID] =
            LoadTexture((uint8_t *)achievement.pImageBuffer, achievement.ImageBufferSize).release();
    }

    // Move backbuffer to guest memory.
    assert(!g_memory.IsInMemoryRange(g_backBuffer) && g_backBufferHolder != nullptr);
    g_backBuffer = g_userHeap.AllocPhysical<GuestSurface>(std::move(*g_backBufferHolder));

    // Check for stale reference. BeginCommandList() gets called before CreateDevice() which is where the assignment happens.
    if (g_renderTarget == g_backBufferHolder.get()) g_renderTarget = g_backBuffer;
    if (g_depthStencil == g_backBufferHolder.get()) g_depthStencil = g_backBuffer;

    // Free the host backbuffer.
    g_backBufferHolder = nullptr;

    auto device = g_userHeap.AllocPhysical<GuestDevice>();
    memset(device, 0, sizeof(*device));

    // Append render state functions to the end of guest function table.
    uint32_t functionOffset = PPC_CODE_BASE + PPC_CODE_SIZE;
    g_memory.InsertFunction(functionOffset, HostToGuestFunction<SetRenderStateUnimplemented>);

    for (size_t i = 0; i < std::size(device->setRenderStateFunctions); i++)
        device->setRenderStateFunctions[i] = functionOffset;

    for (auto& [state, function] : g_setRenderStateFunctions)
    {
        functionOffset += 4;
        g_memory.InsertFunction(functionOffset, function);
        device->setRenderStateFunctions[state / 4] = functionOffset;
    }

    for (size_t i = 0; i < std::size(device->setSamplerStateFunctions); i++)
        device->setSamplerStateFunctions[i] = *reinterpret_cast<uint32_t*>(g_memory.Translate(0x8330F3DC + i * 0xC));

    device->viewport.width = 1280.0f;
    device->viewport.height = 720.0f;
    device->viewport.maxZ = 1.0f;

    *a6 = g_memory.MapVirtual(device);

    return 0;
}

static void DestructResource(GuestResource* resource) 
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::DestructResource;
    cmd.destructResource.resource = resource;
    g_renderQueue.enqueue(cmd);
}

static void ProcDestructResource(const RenderCommand& cmd)
{
    const auto& args = cmd.destructResource;
    g_tempResources[g_frame].push_back(args.resource);

    // VR: the reflection map's surface is only compared by address; forget it
    // before the address can be reused.
    if (g_vrReflectionSurfaceSeen != nullptr && args.resource == g_vrReflectionSurfaceSeen)
        g_vrReflectionSurfaceSeen = nullptr;

    if (args.resource->type == ResourceType::RenderTarget || args.resource->type == ResourceType::DepthStencil)
        g_vrSurfaceUses.erase(static_cast<const GuestSurface*>(args.resource));
}

static uint32_t ComputeTexturePitch(GuestTexture* texture)
{
    return (texture->width * RenderFormatSize(texture->format) + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
}

static void LockTextureRect(GuestTexture* texture, uint32_t, GuestLockedRect* lockedRect) 
{
    uint32_t pitch = ComputeTexturePitch(texture);
    uint32_t slicePitch = pitch * texture->height;

    if (texture->mappedMemory == nullptr)
        texture->mappedMemory = g_userHeap.AllocPhysical(slicePitch, 0x10);

    lockedRect->pitch = pitch;
    lockedRect->bits = g_memory.MapVirtual(texture->mappedMemory);
}

static void UnlockTextureRect(GuestTexture* texture) 
{
    assert(std::this_thread::get_id() == g_presentThreadId);

    RenderCommand cmd;
    cmd.type = RenderCommandType::UnlockTextureRect;
    cmd.unlockTextureRect.texture = texture;
    g_renderQueue.enqueue(cmd);
}

static void ProcUnlockTextureRect(const RenderCommand& cmd)
{
    const auto& args = cmd.unlockTextureRect;

    AddBarrier(args.texture, RenderTextureLayout::COPY_DEST);
    FlushBarriers();

    uint32_t pitch = ComputeTexturePitch(args.texture);
    uint32_t slicePitch = pitch * args.texture->height;

    auto allocation = g_uploadAllocators[g_frame].allocate(slicePitch, PLACEMENT_ALIGNMENT);
    memcpy(allocation.memory, args.texture->mappedMemory, slicePitch);

    // Every layer: both eyes read it.
    for (uint32_t layer = 0; layer < args.texture->layerCount; layer++)
    {
        g_commandLists[g_frame]->copyTextureRegion(
            RenderTextureCopyLocation::Subresource(args.texture->texture, 0, layer),
            RenderTextureCopyLocation::PlacedFootprint(allocation.buffer, args.texture->format, args.texture->width, args.texture->height, 1, pitch / RenderFormatSize(args.texture->format), allocation.offset));
    }

    RecordVRUpload({ nullptr, nullptr, args.texture, allocation, pitch });
}

static void* LockBuffer(GuestBuffer* buffer, uint32_t flags)
{
    buffer->lockedReadOnly = (flags & 0x10) != 0;

    if (buffer->mappedMemory == nullptr)
        buffer->mappedMemory = g_userHeap.AllocPhysical(buffer->dataSize, 0x10);

    return buffer->mappedMemory;
}

static void* LockVertexBuffer(GuestBuffer* buffer, uint32_t, uint32_t, uint32_t flags)
{
    return LockBuffer(buffer, flags);
}

static std::atomic<uint32_t> g_bufferUploadCount = 0;

// Returns the upload buffer the command list copies from, when it does.
template<typename T>
static RenderBuffer* UnlockBuffer(GuestBuffer* buffer, bool useCopyQueue)
{
    RenderBuffer* commandListUploadBuffer = nullptr;

    auto copyBuffer = [&](T* dest)
        {
            auto src = reinterpret_cast<const T*>(buffer->mappedMemory);

            for (size_t i = 0; i < buffer->dataSize; i += sizeof(T))
            {
                *dest = ByteSwap(*src);
                ++dest;
                ++src;
            }
        };

    if (useCopyQueue && g_capabilities.gpuUploadHeap)
    {
        copyBuffer(reinterpret_cast<T*>(buffer->buffer->map()));
        buffer->buffer->unmap();
    }
    else
    {
        auto uploadBuffer = g_device->createBuffer(RenderBufferDesc::UploadBuffer(buffer->dataSize));
        copyBuffer(reinterpret_cast<T*>(uploadBuffer->map()));
        uploadBuffer->unmap();

        if (useCopyQueue)
        {
            ExecuteCopyCommandList([&]
                {
                    g_copyCommandList->copyBufferRegion(buffer->buffer->at(0), uploadBuffer->at(0), buffer->dataSize);
                });
        }
        else
        {
            auto& commandList = g_commandLists[g_frame];

            commandList->barriers(RenderBarrierStage::COPY, RenderBufferBarrier(buffer->buffer.get(), RenderBufferAccess::WRITE));
            commandList->copyBufferRegion(buffer->buffer->at(0), uploadBuffer->at(0), buffer->dataSize);
            commandList->barriers(RenderBarrierStage::GRAPHICS, RenderBufferBarrier(buffer->buffer.get(), RenderBufferAccess::READ));

            commandListUploadBuffer = uploadBuffer.get();
            g_tempBuffers[g_frame].emplace_back(std::move(uploadBuffer));
        }
    }

    g_bufferUploadCount++;

    return commandListUploadBuffer;
}

template<typename T>
static void UnlockBuffer(GuestBuffer* buffer)
{
    if (!buffer->lockedReadOnly)
    {
        if (std::this_thread::get_id() == g_presentThreadId)
        {
            RenderCommand cmd;
            cmd.type = (sizeof(T) == 2) ? RenderCommandType::UnlockBuffer16 : RenderCommandType::UnlockBuffer32;
            cmd.unlockBuffer.buffer = buffer;
            g_renderQueue.enqueue(cmd);
        }
        else
        {
            UnlockBuffer<T>(buffer, true);
        }
    }
}

static void ProcUnlockBuffer16(const RenderCommand& cmd)
{
    if (auto uploadBuffer = UnlockBuffer<uint16_t>(cmd.unlockBuffer.buffer, false))
        RecordVRUpload({ cmd.unlockBuffer.buffer, uploadBuffer, nullptr, {}, 0 });
}

static void ProcUnlockBuffer32(const RenderCommand& cmd)
{
    if (auto uploadBuffer = UnlockBuffer<uint32_t>(cmd.unlockBuffer.buffer, false))
        RecordVRUpload({ cmd.unlockBuffer.buffer, uploadBuffer, nullptr, {}, 0 });
}

static void ProcVRReplayUpload(const RenderCommand& cmd)
{
    const VRUploadRecord& record = g_vrUploadRecords[cmd.vrReplayUpload.recordIndex];
    auto& commandList = g_commandLists[g_frame];

    if (record.buffer != nullptr)
    {
        commandList->barriers(RenderBarrierStage::COPY, RenderBufferBarrier(record.buffer->buffer.get(), RenderBufferAccess::WRITE));
        commandList->copyBufferRegion(record.buffer->buffer->at(0), record.uploadBuffer->at(0), record.buffer->dataSize);
        commandList->barriers(RenderBarrierStage::GRAPHICS, RenderBufferBarrier(record.buffer->buffer.get(), RenderBufferAccess::READ));
    }
    else
    {
        GuestTexture* texture = record.texture;

        AddBarrier(texture, RenderTextureLayout::COPY_DEST);
        FlushBarriers();

        commandList->copyTextureRegion(
            RenderTextureCopyLocation::Subresource(texture->texture, 0),
            RenderTextureCopyLocation::PlacedFootprint(record.allocation.buffer, texture->format, texture->width, texture->height, 1, record.pitch / RenderFormatSize(texture->format), record.allocation.offset));
    }
}

static void UnlockVertexBuffer(GuestBuffer* buffer)
{
    UnlockBuffer<uint32_t>(buffer);
}

static void GetVertexBufferDesc(GuestBuffer* buffer, GuestBufferDesc* desc) 
{
    desc->size = buffer->dataSize;
}

static void* LockIndexBuffer(GuestBuffer* buffer, uint32_t, uint32_t, uint32_t flags) 
{
    return LockBuffer(buffer, flags);
}

static void UnlockIndexBuffer(GuestBuffer* buffer) 
{
    if (buffer->guestFormat == D3DFMT_INDEX32)
        UnlockBuffer<uint32_t>(buffer);
    else
        UnlockBuffer<uint16_t>(buffer);
}

static void GetIndexBufferDesc(GuestBuffer* buffer, GuestBufferDesc* desc)
{
    desc->format = buffer->guestFormat;
    desc->size = buffer->dataSize;
}

static void GetSurfaceDesc(GuestSurface* surface, GuestSurfaceDesc* desc) 
{
    desc->width = surface->width;
    desc->height = surface->height;
}

static void GetVertexDeclaration(GuestVertexDeclaration* vertexDeclaration, GuestVertexElement* vertexElements, be<uint32_t>* count) 
{
    memcpy(vertexElements, vertexDeclaration->vertexElements.get(), vertexDeclaration->vertexElementCount * sizeof(GuestVertexElement));
    *count = vertexDeclaration->vertexElementCount;
}

static uint32_t HashVertexDeclaration(uint32_t vertexDeclaration) 
{
    // Vertex declarations are cached on host side, so the pointer itself can be used.
    return vertexDeclaration;
}

static const char *DeviceTypeName(RenderDeviceType type)
{
    switch (type) 
    {
    case RenderDeviceType::INTEGRATED:
        return "Integrated";
    case RenderDeviceType::DISCRETE:
        return "Discrete";
    case RenderDeviceType::VIRTUAL:
        return "Virtual";
    case RenderDeviceType::CPU:
        return "CPU";
    default:
        return "Unknown";
    }
}

static void DrawProfiler()
{
    bool toggleProfiler = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F1] != 0;

    if (!g_profilerWasToggled && toggleProfiler)
    {
        g_profilerVisible = !g_profilerVisible;

        GameWindow::SetFullscreenCursorVisibility(App::s_isInit ? g_profilerVisible : true);
    }

    g_profilerWasToggled = toggleProfiler;

    if (!g_profilerVisible)
        return;

    ImFont* font = ImFontAtlasSnapshot::GetFont("FOT-SeuratPro-M.otf");
    float defaultScale = font->Scale;
    font->Scale = ImGui::GetDefaultFont()->FontSize / font->FontSize;
    ImGui::PushFont(font);

    if (ImGui::Begin("Profiler", &g_profilerVisible))
    {
        g_applicationValues[g_profilerValueIndex] = App::s_deltaTime * 1000.0;

        const double applicationAvg = std::accumulate(g_applicationValues, g_applicationValues + PROFILER_VALUE_COUNT, 0.0) / PROFILER_VALUE_COUNT;
        double gpuFrameAvg = g_gpuFrameProfiler.UpdateAndReturnAverage();
        double presentAvg = g_presentProfiler.UpdateAndReturnAverage();
        double updateDirectorAvg = g_updateDirectorProfiler.UpdateAndReturnAverage();
        double renderDirectorAvg = g_renderDirectorProfiler.UpdateAndReturnAverage();
        double frameFenceAvg = g_frameFenceProfiler.UpdateAndReturnAverage();
        double presentWaitAvg = g_presentWaitProfiler.UpdateAndReturnAverage();
        double swapChainAcquireAvg = g_swapChainAcquireProfiler.UpdateAndReturnAverage();

        if (ImPlot::BeginPlot("Frame Time"))
        {
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 20.0);
            ImPlot::SetupAxis(ImAxis_Y1, "ms", ImPlotAxisFlags_None);
            ImPlot::PlotLine<double>("Application", g_applicationValues, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
            ImPlot::PlotLine<double>("GPU Frame", g_gpuFrameProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
            ImPlot::PlotLine<double>("Present", g_presentProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
            ImPlot::PlotLine<double>("Update Director", g_updateDirectorProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
            ImPlot::PlotLine<double>("Render Director", g_renderDirectorProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
            ImPlot::PlotLine<double>("Frame Fence", g_frameFenceProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
            ImPlot::PlotLine<double>("Present Wait", g_presentWaitProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
            ImPlot::PlotLine<double>("Swap Chain Acquire", g_swapChainAcquireProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
            ImPlot::EndPlot();
        }

        g_profilerValueIndex = (g_profilerValueIndex + 1) % PROFILER_VALUE_COUNT;

        ImGui::Text("Current Application: %g ms (%g FPS)", App::s_deltaTime * 1000.0, 1.0 / App::s_deltaTime);
        ImGui::Text("Current GPU Frame: %g ms (%g FPS)", g_gpuFrameProfiler.value.load(), 1000.0 / g_gpuFrameProfiler.value.load());
        ImGui::Text("Current Present: %g ms (%g FPS)", g_presentProfiler.value.load(), 1000.0 / g_presentProfiler.value.load());
        ImGui::Text("Current Update Director: %g ms (%g FPS)", g_updateDirectorProfiler.value.load(), 1000.0 / g_updateDirectorProfiler.value.load());
        ImGui::Text("Current Render Director: %g ms (%g FPS)", g_renderDirectorProfiler.value.load(), 1000.0 / g_renderDirectorProfiler.value.load());
        ImGui::Text("Current Frame Fence: %g ms", g_frameFenceProfiler.value.load());
        ImGui::Text("Current Present Wait: %g ms", g_presentWaitProfiler.value.load());
        ImGui::Text("Current Swap Chain Acquire: %g ms", g_swapChainAcquireProfiler.value.load());

        ImGui::NewLine();

        ImGui::Text("Average Application: %g ms (%g FPS)", applicationAvg, 1000.0 / applicationAvg);
        ImGui::Text("Average GPU Frame: %g ms (%g FPS)", gpuFrameAvg, 1000.0 / gpuFrameAvg);
        ImGui::Text("Average Present: %g ms (%g FPS)", presentAvg, 1000.0 / presentAvg);
        ImGui::Text("Average Update Director: %g ms (%g FPS)", updateDirectorAvg, 1000.0 / updateDirectorAvg);
        ImGui::Text("Average Render Director: %g ms (%g FPS)", renderDirectorAvg, 1000.0 / renderDirectorAvg);
        ImGui::Text("Average Frame Fence: %g ms", frameFenceAvg);
        ImGui::Text("Average Present Wait: %g ms", presentWaitAvg);
        ImGui::Text("Average Swap Chain Acquire: %g ms", swapChainAcquireAvg);

        ImGui::NewLine();

        if (g_userHeap.heap != nullptr && g_userHeap.physicalHeap != nullptr)
        {
            O1HeapDiagnostics diagnostics, physicalDiagnostics;
            {
                std::lock_guard lock(g_userHeap.mutex);
                diagnostics = o1heapGetDiagnostics(g_userHeap.heap);
            }
            {
                std::lock_guard lock(g_userHeap.physicalMutex);
                physicalDiagnostics = o1heapGetDiagnostics(g_userHeap.physicalHeap);
            }

            ImGui::Text("Heap Allocated: %d MB", int32_t(diagnostics.allocated / (1024 * 1024)));
            ImGui::Text("Physical Heap Allocated: %d MB", int32_t(physicalDiagnostics.allocated / (1024 * 1024)));
        }

        ImGui::Text("GPU Waits: %d", int32_t(g_waitForGPUCount));
        ImGui::Text("Buffer Uploads: %d", int32_t(g_bufferUploadCount));
        ImGui::NewLine();

        ImGui::Text("Present Wait: %s", g_capabilities.presentWait ? "Supported" : "Unsupported");
        ImGui::Text("Triangle Fan: %s", g_capabilities.triangleFan ? "Supported" : "Unsupported");
        ImGui::Text("Dynamic Depth Bias: %s", g_capabilities.dynamicDepthBias ? "Supported" : "Unsupported");
        ImGui::Text("Triangle Strip Workaround: %s", g_triangleStripWorkaround ? "Enabled" : "Disabled");
        ImGui::Text("Hardware Resolve: %s", g_hardwareResolve ? "Enabled" : "Disabled");
        ImGui::Text("Hardware Depth Resolve: %s", g_hardwareDepthResolve ? "Enabled" : "Disabled");
        ImGui::NewLine();

        ImGui::Text("API: %s", g_vulkan ? "Vulkan" : "D3D12");
        ImGui::Text("Device: %s", g_device->getDescription().name.c_str());
        ImGui::Text("Device Type: %s", DeviceTypeName(g_device->getDescription().type));
        ImGui::Text("VRAM: %.2f MiB", (double)(g_device->getDescription().dedicatedVideoMemory) / (1024.0 * 1024.0));
        ImGui::Text("UMA: %s", g_capabilities.uma ? "Supported" : "Unsupported");
        ImGui::Text("GPU Upload Heap: %s", g_capabilities.gpuUploadHeap ? "Supported" : "Unsupported");

        const char* sdlVideoDriver = SDL_GetCurrentVideoDriver();
        if (sdlVideoDriver != nullptr)
            ImGui::Text("SDL Video Driver: %s", sdlVideoDriver);

        ImGui::NewLine();
        ImGui::Checkbox("Show FPS", &Config::ShowFPS.Value);
        ImGui::NewLine();

        if (ImGui::TreeNode("Device Names"))
        {
            ImGui::Indent();

            uint32_t deviceIndex = 0;
            for (const std::string &deviceName : g_interface->getDeviceNames())
            {
                ImGui::Text("Option #%d: %s", deviceIndex++, deviceName.c_str());
            }

            ImGui::Unindent();
            ImGui::TreePop();
        }
    }
    ImGui::End();

    ImGui::PopFont();
    font->Scale = defaultScale;
}

static void DrawFPS()
{
    if (!Config::ShowFPS)
        return;

    double time = ImGui::GetTime();
    static double updateTime = time;
    static double fps = 0;
    static double totalDeltaTime = 0.0;
    static uint32_t totalDeltaCount = 0;

    totalDeltaTime += g_presentProfiler.value.load();
    totalDeltaCount++;

    if (time - updateTime >= 1.0f)
    {
        fps = 1000.0 / std::max(totalDeltaTime / double(totalDeltaCount), 1.0);
        updateTime = time;
        totalDeltaTime = 0.0;
        totalDeltaCount = 0;
    }

    auto drawList = ImGui::GetBackgroundDrawList();

    auto fmt = fmt::format("FPS: {:.2f}", fps);
    auto font = ImFontAtlasSnapshot::GetFont("FOT-SeuratPro-M.otf");
    auto fontSize = Scale(10);
    auto textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0, fmt.c_str());

    ImVec2 min = { Scale(40), Scale(30) };
    ImVec2 max = { min.x + std::max(Scale(75), textSize.x + Scale(10)), min.y + Scale(15) };
    ImVec2 textPos = { min.x + Scale(2), CENTRE_TEXT_VERT(min, max, textSize) + Scale(0.2f) };

    drawList->AddRectFilled(min, max, IM_COL32(0, 0, 0, 200));
    drawList->AddText(font, fontSize, textPos, IM_COL32_WHITE, fmt.c_str());
}

static void DrawImGui()
{
    ImGui_ImplSDL2_NewFrame();

    auto& io = ImGui::GetIO();
    io.DisplaySize = { float(Video::s_viewportWidth), float(Video::s_viewportHeight) };

    // ImGui doesn't know that we center the screen for specific aspect ratio
    // settings, which causes mouse events to not work correctly. To fix this, 
    // we can adjust the mouse events before ImGui processes them.
    uint32_t width = g_swapChain->getWidth();
    uint32_t height = g_swapChain->getHeight();
    float mousePosScaleX = float(width) / float(GameWindow::s_width);
    float mousePosScaleY = float(height) / float(GameWindow::s_height);
    float mousePosOffsetX = (width - Video::s_viewportWidth) / 2.0f;
    float mousePosOffsetY = (height - Video::s_viewportHeight) / 2.0f;
    for (int i = 0; i < io.Ctx->InputEventsQueue.Size; i++)
    {
        auto& e = io.Ctx->InputEventsQueue[i];
        if (e.Type == ImGuiInputEventType_MousePos)
        {
            if (e.MousePos.PosX != -FLT_MAX)
            {
                e.MousePos.PosX *= mousePosScaleX;
                e.MousePos.PosX -= mousePosOffsetX;
            }

            if (e.MousePos.PosY != -FLT_MAX)
            {
                e.MousePos.PosY *= mousePosScaleY;
                e.MousePos.PosY -= mousePosOffsetY;
            }
        }
    }

    ImGui::NewFrame();

    ResetImGuiCallbacks();

#ifdef ASYNC_PSO_DEBUG
    if (ImGui::Begin("Async PSO Stats"))
    {
        ImGui::Text("Pipelines Created In Render Thread: %d", g_pipelinesCreatedInRenderThread.load());
        ImGui::Text("Pipelines Created Asynchronously: %d", g_pipelinesCreatedAsynchronously.load());
        ImGui::Text("Pipelines Dropped: %d", g_pipelinesDropped.load());
        ImGui::Text("Pipelines Currently Compiling: %d", g_pipelinesCurrentlyCompiling.load());
        ImGui::Text("Compiling Pipeline Task Count: %d", g_compilingPipelineTaskCount.load());
        ImGui::Text("Pending Pipeline Task Count: %d", g_pendingPipelineTaskCount.load());

        std::lock_guard lock(g_debugMutex);
        ImGui::TextUnformatted(g_pipelineDebugText.c_str());
    }
    ImGui::End();
#endif

    AchievementMenu::Draw();
    OptionsMenu::Draw();
    AchievementOverlay::Draw();
    InstallerWizard::Draw();
    MessageWindow::Draw();
    ButtonGuide::Draw();
    Fader::Draw();
    BlackBar::Draw();

    assert(ImGui::GetBackgroundDrawList()->_ClipRectStack.Size == 1 && "Some clip rects were not removed from the stack!");

    DrawFPS();
    DrawProfiler();
    ImGui::Render();

    auto drawData = ImGui::GetDrawData();
    if (drawData->CmdListsCount != 0)
    {
        RenderCommand cmd;
        cmd.type = RenderCommandType::DrawImGui;
        g_renderQueue.enqueue(cmd);
    }
}

static void SetFramebuffer(GuestSurface *renderTarget, GuestSurface *depthStencil, bool settingForClear);

// VR: whether an ImGui triangle covers the whole display: all its corners lie
// on the display's corners (or just outside, for anti-aliasing fringes).
static bool IsImGuiFullDisplayTriangle(const ImDrawList* drawList, const ImDrawCmd& drawCmd, uint32_t firstIndex, ImVec2 displaySize)
{
    constexpr float Tolerance = 1.5f;

    for (uint32_t i = 0; i < 3; i++)
    {
        const ImDrawVert& vertex = drawList->VtxBuffer[drawCmd.VtxOffset + drawList->IdxBuffer[drawCmd.IdxOffset + firstIndex + i]];
        bool onEdgeX = vertex.pos.x <= Tolerance || vertex.pos.x >= displaySize.x - Tolerance;
        bool onEdgeY = vertex.pos.y <= Tolerance || vertex.pos.y >= displaySize.y - Tolerance;
        if (!onEdgeX || !onEdgeY)
            return false;
    }

    return true;
}

static void ProcDrawImGui(const RenderCommand& cmd)
{
    SetShadingRateImage(nullptr);

    // VR: in stereo frames the UI goes on the HUD panel. It is drawn once, in the
    // left eye's pass, into the UI texture that both eyes composite; what covers
    // the whole display (fades, dimmed backgrounds) is drawn into each eye.
    bool vrPanel = IsVRFrameImmersive() && g_vrUiTexture != nullptr;
    bool vrRightEye = vrPanel && g_vrPass == 1;

    // Make sure the backbuffer is the current target.
    AddBarrier(g_backBuffer, RenderTextureLayout::COLOR_WRITE);
    FlushBarriers();
    SetFramebuffer(g_backBuffer, nullptr, false);

    auto& commandList = g_commandLists[g_frame];
    auto pipeline = g_imPipeline.get();

    RenderFramebuffer* backBufferFramebuffer = g_framebuffer;
    bool panelBound = false;

    if (vrPanel && !vrRightEye)
    {
        commandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(g_vrUiTexture.get(), RenderTextureLayout::COLOR_WRITE));
        commandList->setFramebuffer(g_vrUiFramebuffer.get());
        commandList->clearColor(0, RenderColor(0.0f, 0.0f, 0.0f, 0.0f));
        commandList->setFramebuffer(backBufferFramebuffer);
    }

    auto bindTarget = [&](bool panel)
        {
            if (panel != panelBound)
            {
                commandList->setFramebuffer(panel ? g_vrUiFramebuffer.get() : backBufferFramebuffer);
                panelBound = panel;
            }
        };

    commandList->setGraphicsPipelineLayout(g_imPipelineLayout.get());
    commandList->setPipeline(pipeline);
    commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 0);
    commandList->setGraphicsDescriptorSet(g_samplerDescriptorSet.get(), 1);

    auto& drawData = *ImGui::GetDrawData();
    SetHostViewport(commandList.get(), RenderViewport(drawData.DisplayPos.x, drawData.DisplayPos.y, drawData.DisplaySize.x, drawData.DisplaySize.y));

    ImGuiPushConstants pushConstants{};
    pushConstants.displaySize = drawData.DisplaySize;
    pushConstants.inverseDisplaySize = { 1.0f / drawData.DisplaySize.x, 1.0f / drawData.DisplaySize.y };
    commandList->setGraphicsPushConstants(0, &pushConstants);

    size_t pushConstantRangeMin = ~0;
    size_t pushConstantRangeMax = 0;

    auto setPushConstants = [&](void* destination, const void* source, size_t size)
        {
            bool dirty = memcmp(destination, source, size) != 0;

            memcpy(destination, source, size);

            if (dirty)
            {
                size_t offset = reinterpret_cast<size_t>(destination) - reinterpret_cast<size_t>(&pushConstants);
                pushConstantRangeMin = std::min(pushConstantRangeMin, offset);
                pushConstantRangeMax = std::max(pushConstantRangeMax, offset + size);
            }
        };

    ImRect clipRect{};

    for (int i = 0; i < drawData.CmdListsCount; i++)
    {
        auto& drawList = drawData.CmdLists[i];

        auto vertexBufferAllocation = g_uploadAllocators[g_frame].allocate<false>(drawList->VtxBuffer.Data, drawList->VtxBuffer.Size * sizeof(ImDrawVert), alignof(ImDrawVert));
        auto indexBufferAllocation = g_uploadAllocators[g_frame].allocate<false>(drawList->IdxBuffer.Data, drawList->IdxBuffer.Size * sizeof(uint16_t), alignof(uint16_t));

        const RenderVertexBufferView vertexBufferView(vertexBufferAllocation.buffer->at(vertexBufferAllocation.offset), drawList->VtxBuffer.Size * sizeof(ImDrawVert));
        const RenderInputSlot inputSlot(0, sizeof(ImDrawVert));
        commandList->setVertexBuffers(0, &vertexBufferView, 1, &inputSlot);

        const RenderIndexBufferView indexBufferView(indexBufferAllocation.buffer->at(indexBufferAllocation.offset), drawList->IdxBuffer.Size * sizeof(uint16_t), RenderFormat::R16_UINT);
        commandList->setIndexBuffer(&indexBufferView);

        for (int j = 0; j < drawList->CmdBuffer.Size; j++)
        {
            auto& drawCmd = drawList->CmdBuffer[j];
            if (drawCmd.UserCallback != nullptr)
            {
                auto callbackData = reinterpret_cast<const ImGuiCallbackData*>(drawCmd.UserCallbackData);

                switch (static_cast<ImGuiCallback>(reinterpret_cast<size_t>(drawCmd.UserCallback)))
                {
                case ImGuiCallback::SetGradient:
                    setPushConstants(&pushConstants.boundsMin, &callbackData->setGradient, sizeof(callbackData->setGradient));
                    break;       
                case ImGuiCallback::SetShaderModifier:
                    setPushConstants(&pushConstants.shaderModifier, &callbackData->setShaderModifier, sizeof(callbackData->setShaderModifier));
                    break;
                case ImGuiCallback::SetOrigin:
                    setPushConstants(&pushConstants.origin, &callbackData->setOrigin, sizeof(callbackData->setOrigin));
                    break;
                case ImGuiCallback::SetScale:
                    setPushConstants(&pushConstants.scale, &callbackData->setScale, sizeof(callbackData->setScale));
                    break;       
                case ImGuiCallback::SetMarqueeFade:
                    setPushConstants(&pushConstants.boundsMin, &callbackData->setMarqueeFade, sizeof(callbackData->setMarqueeFade));
                    break;
                case ImGuiCallback::SetOutline:
                    setPushConstants(&pushConstants.outline, &callbackData->setOutline, sizeof(callbackData->setOutline));
                    break;
                case ImGuiCallback::SetProceduralOrigin:
                    setPushConstants(&pushConstants.proceduralOrigin, &callbackData->setProceduralOrigin, sizeof(callbackData->setProceduralOrigin));
                    break;
                case ImGuiCallback::SetAdditive:
                {
                    auto pipelineToSet = callbackData->setAdditive.enabled ? g_imAdditivePipeline.get() : g_imPipeline.get();
                    if (pipeline != pipelineToSet)
                    {
                        commandList->setPipeline(pipelineToSet);
                        pipeline = pipelineToSet;
                    }
                    break;
                }
                default:
                    assert(false && "Unknown ImGui callback type.");
                    break;
                }
            }
            else
            {
                if (drawCmd.ClipRect.z <= drawCmd.ClipRect.x || drawCmd.ClipRect.w <= drawCmd.ClipRect.y)
                    continue;

                auto texture = reinterpret_cast<GuestTexture*>(drawCmd.TextureId);
                uint32_t descriptorIndex = TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D;
                if (texture != nullptr)
                {
                    if (texture->layout != RenderTextureLayout::SHADER_READ)
                    {
                        commandList->barriers(RenderBarrierStage::GRAPHICS | RenderBarrierStage::COPY,
                            RenderTextureBarrier(texture->texture, RenderTextureLayout::SHADER_READ));

                        texture->layout = RenderTextureLayout::SHADER_READ;
                    }

                    descriptorIndex = texture->descriptorIndex;

                    if (texture == g_imFontTexture.get())
                        descriptorIndex |= 0x80000000;

                    setPushConstants(&pushConstants.texture2DDescriptorIndex, &descriptorIndex, sizeof(descriptorIndex));
                }

                if (pushConstantRangeMin < pushConstantRangeMax)
                {
                    commandList->setGraphicsPushConstants(0, reinterpret_cast<const uint8_t*>(&pushConstants) + pushConstantRangeMin, pushConstantRangeMin, pushConstantRangeMax - pushConstantRangeMin);
                    pushConstantRangeMin = ~0;
                    pushConstantRangeMax = 0;
                }

                if (memcmp(&clipRect, &drawCmd.ClipRect, sizeof(clipRect)) != 0)
                {
                    SetHostScissor(commandList.get(), RenderRect(int32_t(drawCmd.ClipRect.x), int32_t(drawCmd.ClipRect.y), int32_t(drawCmd.ClipRect.z), int32_t(drawCmd.ClipRect.w)));
                    clipRect = drawCmd.ClipRect;
                }

                // The back buffer gets an instance per layer (single-pass stereo).
                if (!vrPanel)
                {
                    commandList->drawIndexedInstanced(drawCmd.ElemCount, g_backBuffer->layerCount, drawCmd.IdxOffset, drawCmd.VtxOffset, 0);
                    continue;
                }

                // Split the command into runs of full-display triangles (into the eye)
                // and the rest (onto the panel, drawn in the left eye's pass only).
                uint32_t runStart = 0;
                while (runStart < drawCmd.ElemCount)
                {
                    bool fullDisplay = IsImGuiFullDisplayTriangle(drawList, drawCmd, runStart, drawData.DisplaySize);
                    uint32_t runEnd = runStart + 3;

                    while (runEnd < drawCmd.ElemCount && IsImGuiFullDisplayTriangle(drawList, drawCmd, runEnd, drawData.DisplaySize) == fullDisplay)
                        runEnd += 3;

                    if (fullDisplay || !vrRightEye)
                    {
                        bindTarget(!fullDisplay);
                        commandList->drawIndexedInstanced(runEnd - runStart, fullDisplay ? g_backBuffer->layerCount : 1, drawCmd.IdxOffset + runStart, drawCmd.VtxOffset, 0);

                        if (!fullDisplay)
                            g_vrUiUsed = true;
                    }

                    runStart = runEnd;
                }
            }
        }
    }

    bindTarget(false);
}

// We have to check for this to properly handle the following situation:
// 1. Wait on swap chain.
// 2. Create loading thread.
// 3. Loading thread also waits on swap chain.
// 4. Loading thread presents and quits.
// 5. After the loading thread quits, application also presents.
static bool g_pendingWaitOnSwapChain = true;

void Video::WaitOnSwapChain()
{
    if (g_pendingWaitOnSwapChain)
    {
        if (VR::IsActive())
        {
            // xrWaitFrame paces the game; the poses located here belong to this frame.
            g_presentWaitProfiler.Begin();
            VR::BeginFrame();
            g_presentWaitProfiler.End();
        }
        else if (g_swapChainValid)
        {
            g_presentWaitProfiler.Begin();
            g_swapChain->wait();
            g_presentWaitProfiler.End();
        }

        g_pendingWaitOnSwapChain = false;
    }
}

static bool g_shouldPrecompilePipelines;
static std::atomic<bool> g_executedCommandList;

void Video::Present() 
{
    g_readyForCommands = false;

    RenderCommand cmd;
    cmd.type = RenderCommandType::ExecutePendingStretchRectCommands;
    g_renderQueue.enqueue(cmd);

    DrawImGui();

    if (VR::IsActive())
    {
        // Frames presented without CApplication::Update (the installer, or the
        // main thread after the loading thread presented) begin their XR frame here.
        VR::BeginFrame();
        VR::AcquireImages(g_vrImages);
    }

    cmd.type = RenderCommandType::ExecuteCommandList;
    g_renderQueue.enqueue(cmd);

    // All the shaders are available at this point. We can precompile embedded PSOs then.
    if (g_shouldPrecompilePipelines)
    {
        EnqueuePipelineTask(PipelineTaskType::PrecompilePipelines, {});
        g_shouldPrecompilePipelines = false;
    }

    g_executeWaitProfiler.Begin();
    g_executedCommandList.wait(false);
    g_executeWaitProfiler.End();
    g_executedCommandList = false;

    // The copy into the headset image has been submitted; hand it to the compositor.
    bool pacedByVR = false;
    if (VR::IsActive())
    {
        pacedByVR = VR::EndFrame(g_vrFrameResult);
        g_vrImages = {};
    }

    if (g_swapChainValid)
    {
        if (g_pendingWaitOnSwapChain && !VR::IsActive())
        {
            g_presentWaitProfiler.Begin();
            g_swapChain->wait(); // Never gonna happen outside loading threads as explained above.
            g_presentWaitProfiler.End();
        }

        RenderCommandSemaphore* signalSemaphores[] = { g_renderSemaphores[g_frame].get() };
        g_swapChainValid = g_swapChain->present(g_backBufferIndex, signalSemaphores, std::size(signalSemaphores));
    }

    g_pendingWaitOnSwapChain = true;

    g_frame = g_nextFrame;
    g_nextFrame = (g_frame + 1) % NUM_FRAMES;

    if (g_commandListStates[g_frame])
    {
        g_frameFenceProfiler.Begin();
        g_queue->waitForCommandFence(g_commandFences[g_frame].get());
        g_frameFenceProfiler.End();
        g_commandListStates[g_frame] = false;

        // Update the GPU profiler with the results from the timestamps of the frame.
        g_queryPools[g_frame]->queryResults();
        const uint64_t *frameTimestamps = g_queryPools[g_frame]->getResults();
        g_gpuFrameProfiler.Set(double(frameTimestamps[1] - frameTimestamps[0]) / 1000000.0);

        // VR: the frame's GPU segments, each lasting until the next one or the
        // frame's end; the time before the first one counts as flat work.
        uint32_t segmentCount = g_gpuSegmentCounts[g_frame];
        if (segmentCount != GPU_SEGMENTS_UNUSED && segmentCount != 0 && segmentCount <= NUM_SEGMENT_QUERIES)
        {
            g_segmentQueryPools[g_frame]->queryResults();
            const uint64_t* segmentTimestamps = g_segmentQueryPools[g_frame]->getResults();
            double segmentMs[GPU_SEGMENT_KIND_COUNT] = {};
            bool valid = segmentTimestamps[0] >= frameTimestamps[0];

            if (valid)
                segmentMs[GPU_SEGMENT_FLAT] += double(segmentTimestamps[0] - frameTimestamps[0]) / 1000000.0;

            for (uint32_t i = 0; i < segmentCount && valid; i++)
            {
                uint64_t start = segmentTimestamps[i];
                uint64_t end = (i + 1 < segmentCount) ? segmentTimestamps[i + 1] : frameTimestamps[1];
                valid = end >= start && (end - start) < 1000000000ull;

                if (valid)
                    segmentMs[g_gpuSegmentKinds[g_frame][i]] += double(end - start) / 1000000.0;
            }

            if (valid)
            {
                std::lock_guard lock(g_gpuSegmentMutex);

                for (size_t i = 0; i < GPU_SEGMENT_KIND_COUNT; i++)
                    g_gpuSegmentMs[i] += segmentMs[i];

                ++g_gpuSegmentFrames;
                g_gpuSegmentTotal += segmentCount;

                int8_t testState = g_gpuSegmentTestStates[g_frame];
                if (testState >= 0)
                {
                    for (size_t i = 0; i < GPU_SEGMENT_KIND_COUNT; i++)
                        g_gpuSegmentTestMs[testState][i] += segmentMs[i];

                    ++g_gpuSegmentTestFrames[testState];
                }
            }
        }
    }

    // VR: a new headset resolution replaces the headset's images once the GPU
    // is done with the old ones (the render thread waits for BeginCommandList),
    // and the game's screen follows.
    if (VR::IsActive() && VR::IsRenderScalePending())
    {
        for (size_t i = 0; i < NUM_FRAMES; i++)
        {
            if (g_commandListStates[i])
            {
                g_queue->waitForCommandFence(g_commandFences[i].get());
                g_commandListStates[i] = false;
            }
        }

        if (VR::ApplyRenderScale())
        {
            g_needsResize = true;
            Video::ComputeViewportDimensions();
        }
    }

    g_dirtyStates = DirtyStates(true);
    g_uploadAllocators[g_frame].reset();
    g_intermediaryUploadAllocator.reset();
    g_triangleFanIndexData.reset();
    g_quadIndexData.reset();

    CheckSwapChain();

    cmd.type = RenderCommandType::BeginCommandList;
    g_renderQueue.enqueue(cmd);

    // While the headset paces the frames, the desktop limiter would only add latency.
    if (!pacedByVR && Config::FPS >= FPS_MIN && Config::FPS < FPS_MAX)
    {
        using namespace std::chrono_literals;

        static std::chrono::steady_clock::time_point s_next;

        auto now = std::chrono::steady_clock::now();

        if (now < s_next)
        {
            std::this_thread::sleep_for(std::chrono::floor<std::chrono::milliseconds>(s_next - now - 2ms));

            while ((now = std::chrono::steady_clock::now()) < s_next)
                std::this_thread::yield();
        }
        else
        {
            s_next = now;
        }

        s_next += 1000000000ns / Config::FPS;
    }

    g_presentProfiler.Reset();
}

void Video::StartPipelinePrecompilation()
{
    g_shouldPrecompilePipelines = true;
}

static void SetRootDescriptor(const UploadAllocation& allocation, size_t index)
{
    auto& commandList = g_commandLists[g_frame];

    if (g_vulkan)
        commandList->setGraphicsPushConstants(0, &allocation.deviceAddress, 8 * index, 8);
    else
        commandList->setGraphicsRootDescriptor(allocation.buffer->at(allocation.offset), index);
}

struct GammaCorrectionConstants
{
    float gammaR;
    float gammaG;
    float gammaB;
    uint32_t textureDescriptorIndex;

    int32_t viewportOffsetX;
    int32_t viewportOffsetY;
    int32_t viewportWidth;
    int32_t viewportHeight;
};

static void ComputeGammaCorrection(GammaCorrectionConstants& constants)
{
    if (Config::XboxColorCorrection)
    {
        constants.gammaR = 1.2f;
        constants.gammaG = 1.17f;
        constants.gammaB = 0.98f;
    }
    else
    {
        constants.gammaR = 1.0f;
        constants.gammaG = 1.0f;
        constants.gammaB = 1.0f;
    }

    float offset = (Config::Brightness - 0.5f) * 1.2f;

    constants.gammaR = 1.0f / std::clamp(constants.gammaR + offset, 0.1f, 4.0f);
    constants.gammaG = 1.0f / std::clamp(constants.gammaG + offset, 0.1f, 4.0f);
    constants.gammaB = 1.0f / std::clamp(constants.gammaB + offset, 0.1f, 4.0f);
}

// VR: stretches the game's picture (the given layer of it in single-pass
// stereo) over the output texture, gamma-corrected, and copies it into a
// headset image (when there is one).
static void OutputVRImage(RenderTexture* target, uint32_t layer)
{
    auto& commandList = g_commandLists[g_frame];

    GammaCorrectionConstants gamma;
    ComputeGammaCorrection(gamma);

    struct
    {
        float gamma[3];
        uint32_t textureDescriptorIndex;
        float inverseOutputSize[2];
        uint32_t layer;
    } constants;

    constants.gamma[0] = gamma.gammaR;
    constants.gamma[1] = gamma.gammaG;
    constants.gamma[2] = gamma.gammaB;
    constants.textureDescriptorIndex = g_intermediaryBackBufferTextureDescriptorIndex;
    constants.inverseOutputSize[0] = 1.0f / float(g_vrOutputTextureWidth);
    constants.inverseOutputSize[1] = 1.0f / float(g_vrOutputTextureHeight);
    constants.layer = layer;

    RenderTextureBarrier gammaBarriers[] =
    {
        RenderTextureBarrier(g_intermediaryBackBufferTexture.get(), RenderTextureLayout::SHADER_READ),
        RenderTextureBarrier(g_vrOutputTexture.get(), RenderTextureLayout::COLOR_WRITE)
    };

    commandList->barriers(RenderBarrierStage::GRAPHICS, gammaBarriers, std::size(gammaBarriers));
    commandList->setGraphicsPipelineLayout(g_pipelineLayout.get());
    commandList->setPipeline(g_vrOutputPipeline.get());
    commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 0);
    commandList->setGraphicsDescriptorSet(g_samplerDescriptorSet.get(), 3);
    SetRootDescriptor(g_uploadAllocators[g_frame].allocate<false>(&constants, sizeof(constants), 0x100), 2);
    commandList->setFramebuffer(g_vrOutputFramebuffer.get());
    SetHostViewport(commandList.get(), RenderViewport(0.0f, 0.0f, float(g_vrOutputTextureWidth), float(g_vrOutputTextureHeight)));
    SetHostScissor(commandList.get(), RenderRect(0, 0, g_vrOutputTextureWidth, g_vrOutputTextureHeight));
    commandList->drawInstanced(6, 1, 0, 0);

    if (target != nullptr)
    {
        // The swapchain image is the sRGB sibling of the output texture's format,
        // so the compositor decodes the gamma-encoded bytes as such.
        RenderTextureBarrier copyBarriers[] =
        {
            RenderTextureBarrier(g_vrOutputTexture.get(), RenderTextureLayout::COPY_SOURCE),
            RenderTextureBarrier(target, RenderTextureLayout::COPY_DEST)
        };

        commandList->barriers(RenderBarrierStage::COPY, copyBarriers, std::size(copyBarriers));
        commandList->copyTexture(target, g_vrOutputTexture.get());

        // OpenXR requires the image back in the render target state on release.
        commandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(target, RenderTextureLayout::COLOR_WRITE));
    }

    // The intermediary was transitioned outside the back buffer's layout tracking.
    g_backBuffer->layout = RenderTextureLayout::UNKNOWN;
}

// VR: scales the output texture into the window. Follows OutputVRImage, whose
// pipeline layout and descriptors it uses.
static void MirrorVROutput()
{
    if (!g_swapChainValid)
        return;

    auto& commandList = g_commandLists[g_frame];
    auto swapChainTexture = g_swapChain->getTexture(g_backBufferIndex);
    uint32_t windowWidth = g_swapChain->getWidth();
    uint32_t windowHeight = g_swapChain->getHeight();

    struct
    {
        uint32_t textureDescriptorIndex;
        uint32_t padding[3];
        float destinationRect[4];
    } mirrorConstants = {};

    // Fit the screen into the window, keeping its aspect ratio.
    float scale = std::min(float(windowWidth) / float(g_vrOutputTextureWidth), float(windowHeight) / float(g_vrOutputTextureHeight));
    float mirrorWidth = float(g_vrOutputTextureWidth) * scale;
    float mirrorHeight = float(g_vrOutputTextureHeight) * scale;
    mirrorConstants.textureDescriptorIndex = g_vrOutputTextureDescriptorIndex;
    mirrorConstants.destinationRect[0] = (float(windowWidth) - mirrorWidth) * 0.5f;
    mirrorConstants.destinationRect[1] = (float(windowHeight) - mirrorHeight) * 0.5f;
    mirrorConstants.destinationRect[2] = mirrorWidth;
    mirrorConstants.destinationRect[3] = mirrorHeight;

    auto& framebuffer = g_backBuffer->framebuffers[swapChainTexture];
    if (!framebuffer)
    {
        RenderFramebufferDesc desc;
        desc.colorAttachments = const_cast<const RenderTexture**>(&swapChainTexture);
        desc.colorAttachmentsCount = 1;
        framebuffer = g_device->createFramebuffer(desc);
    }

    RenderTextureBarrier mirrorBarriers[] =
    {
        RenderTextureBarrier(g_vrOutputTexture.get(), RenderTextureLayout::SHADER_READ),
        RenderTextureBarrier(swapChainTexture, RenderTextureLayout::COLOR_WRITE)
    };

    commandList->barriers(RenderBarrierStage::GRAPHICS, mirrorBarriers, std::size(mirrorBarriers));
    commandList->setPipeline(g_vrMirrorPipeline.get());
    SetRootDescriptor(g_uploadAllocators[g_frame].allocate<false>(&mirrorConstants, sizeof(mirrorConstants), 0x100), 2);
    commandList->setFramebuffer(framebuffer.get());
    SetHostViewport(commandList.get(), RenderViewport(0.0f, 0.0f, float(windowWidth), float(windowHeight)));
    SetHostScissor(commandList.get(), RenderRect(0, 0, windowWidth, windowHeight));
    commandList->drawInstanced(6, 1, 0, 0);
    commandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(swapChainTexture, RenderTextureLayout::PRESENT));
}

// VR: puts the UI ImGui drew this frame on the HUD panel of the eye being drawn,
// or in single-pass stereo of both eyes (an instance each).
static void CompositeVRUi()
{
    if (!g_vrUiUsed)
        return;

    auto& commandList = g_commandLists[g_frame];

    AddBarrier(g_backBuffer, RenderTextureLayout::COLOR_WRITE);
    FlushBarriers();
    SetFramebuffer(g_backBuffer, nullptr, true);

    commandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(g_vrUiTexture.get(), RenderTextureLayout::SHADER_READ));

    struct
    {
        float panelMatrices[2][16];
        uint32_t textureDescriptorIndex;
    } constants = {};

    for (uint32_t eye = 0; eye < g_backBuffer->layerCount; eye++)
        memcpy(constants.panelMatrices[eye], g_vrEyes[g_vrSinglePass ? eye : g_vrPass].hudMatrix.m, sizeof(constants.panelMatrices[eye]));

    constants.textureDescriptorIndex = g_vrUiTextureDescriptorIndex;

    commandList->setGraphicsPipelineLayout(g_pipelineLayout.get());
    commandList->setPipeline(g_vrUiCompositePipeline.get());
    commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 0);
    commandList->setGraphicsDescriptorSet(g_samplerDescriptorSet.get(), 3);
    SetRootDescriptor(g_uploadAllocators[g_frame].allocate<false>(&constants, sizeof(constants), 0x100), 2);
    SetHostViewport(commandList.get(), RenderViewport(0.0f, 0.0f, float(g_backBuffer->width), float(g_backBuffer->height)));
    SetHostScissor(commandList.get(), RenderRect(0, 0, g_backBuffer->width, g_backBuffer->height));
    commandList->drawInstanced(6, g_backBuffer->layerCount, 0, 0);
}

static void ProcessRenderCommand(const RenderCommand& cmd);

// VR: the end of a frame. A frame that drew through the game camera is shown
// in stereo: the left eye's picture (drawn live) is output, then the frame is
// replayed for the right eye; in single-pass stereo each eye's picture is a
// layer of the back buffer. Any other frame goes to the floating screen.
static void ExecuteVROutput()
{
    SetShadingRateImage(nullptr);
    g_vrRecording = false;
    BeginGpuSegment(GPU_SEGMENT_OUTPUT);

    bool immersive = g_vrPass == 0 && g_vrViews.valid && g_vrCameraDrawCount > 0 &&
        g_vrImages.eyes[0] != nullptr && g_vrImages.eyes[1] != nullptr;

    if (immersive && g_vrSinglePass)
    {
        CompositeVRUi();
        OutputVRImage(g_vrImages.eyes[0], 0);
        MirrorVROutput();
        OutputVRImage(g_vrImages.eyes[1], g_backBuffer->layerCount - 1);

        g_vrFrameResult.mode = VRFrameMode::Immersive;
        g_vrFrameResult.views = g_vrViews;
    }
    else if (immersive)
    {
        CompositeVRUi();
        OutputVRImage(g_vrImages.eyes[0], 0);
        MirrorVROutput();

        RestoreVRFrameStart(g_backBuffer);
        g_vrPass = 1;

        for (const RenderCommand& recordedCommand : g_vrRecordedCommands)
            ProcessRenderCommand(recordedCommand);

        CompositeVRUi();
        OutputVRImage(g_vrImages.eyes[1], 0);

        g_vrFrameResult.mode = VRFrameMode::Immersive;
        g_vrFrameResult.views = g_vrViews;
    }
    else
    {
        OutputVRImage(g_vrImages.screen, 0);
        MirrorVROutput();

        g_vrFrameResult.mode = g_vrImages.screen != nullptr ? VRFrameMode::Flat : VRFrameMode::None;
        g_vrFrameResult.views = {};
        g_vrFrameResult.screenAspectRatio = float(Video::s_viewportWidth) / float(std::max(1u, Video::s_viewportHeight));
    }

    ++(immersive ? g_vrStatistics.immersiveFrames : g_vrStatistics.flatFrames);
    g_vrLastFrameImmersive = immersive;

    // Timings: the render thread's own, and the game thread's latest (written by
    // that thread; its waits for the render thread and the GPU are the last
    // frame's).
    auto now = std::chrono::steady_clock::now();
    double frameSpanMs = std::chrono::duration<double, std::milli>(now - g_vrFrameStartTime).count();
    double busyMs = g_renderThreadBusyMs + std::chrono::duration<double, std::milli>(now - g_renderThreadBatchStart).count();
    g_renderThreadBusyMs = 0.0;
    g_renderThreadBatchStart = now;

    double gameFrameMs = g_presentProfiler.value;
    double gameUpdateMs = g_updateDirectorProfiler.value;
    double gameRenderMs = g_renderDirectorProfiler.value;
    double gpuFrameMs = g_gpuFrameProfiler.value;

    VRDrawStatistics& t = g_vrStatistics;
    ++t.timedFrames;
    t.gameFrameMs += gameFrameMs;
    t.gameFrameMaxMs = std::max(t.gameFrameMaxMs, gameFrameMs);
    t.gameUpdateMs += gameUpdateMs;
    t.gameRenderMs += gameRenderMs;
    t.gameWorkMaxMs = std::max(t.gameWorkMaxMs, gameUpdateMs + gameRenderMs);
    t.headsetWaitMs += g_presentWaitProfiler.value;
    t.renderThreadWaitMs += g_executeWaitProfiler.value;
    t.gpuFenceWaitMs += g_frameFenceProfiler.value;
    t.frameSpanMs += frameSpanMs;
    t.frameSpanMaxMs = std::max(t.frameSpanMaxMs, frameSpanMs);
    t.renderThreadBusyMs += busyMs;
    t.renderThreadBusyMaxMs = std::max(t.renderThreadBusyMaxMs, busyMs);
    t.gpuFrameMs += gpuFrameMs;
    t.gpuFrameMaxMs = std::max(t.gpuFrameMaxMs, gpuFrameMs);

    LogVRStatistics();

    g_vrPass = -1;
    g_vrRecordedCommands.clear();
    g_vrUploadRecords.clear();
}

static void ProcExecuteCommandList(const RenderCommand& cmd)
{
    if (VR::IsActive() && g_vrOutputTexture != nullptr && g_backBuffer->texture == g_intermediaryBackBufferTexture.get())
    {
        ExecuteVROutput();
    }
    else if (g_swapChainValid)
    {
        auto swapChainTexture = g_swapChain->getTexture(g_backBufferIndex);
        if (g_backBuffer->texture == g_intermediaryBackBufferTexture.get())
        {
            GammaCorrectionConstants constants;
            ComputeGammaCorrection(constants);
            constants.textureDescriptorIndex = g_intermediaryBackBufferTextureDescriptorIndex;

            constants.viewportOffsetX = (int32_t(g_swapChain->getWidth()) - int32_t(Video::s_viewportWidth)) / 2;
            constants.viewportOffsetY = (int32_t(g_swapChain->getHeight()) - int32_t(Video::s_viewportHeight)) / 2;
            constants.viewportWidth = Video::s_viewportWidth;
            constants.viewportHeight = Video::s_viewportHeight;

            auto &framebuffer = g_backBuffer->framebuffers[swapChainTexture];
            if (!framebuffer)
            {
                RenderFramebufferDesc desc;
                desc.colorAttachments = const_cast<const RenderTexture **>(&swapChainTexture);
                desc.colorAttachmentsCount = 1;
                framebuffer = g_device->createFramebuffer(desc);
            }

            RenderTextureBarrier srcBarriers[] =
            {
                RenderTextureBarrier(g_intermediaryBackBufferTexture.get(), RenderTextureLayout::SHADER_READ),
                RenderTextureBarrier(swapChainTexture, RenderTextureLayout::COLOR_WRITE)
            };

            auto &commandList = g_commandLists[g_frame];
            commandList->barriers(RenderBarrierStage::GRAPHICS, srcBarriers, std::size(srcBarriers));
            commandList->setGraphicsPipelineLayout(g_pipelineLayout.get());
            commandList->setPipeline(g_gammaCorrectionPipeline.get());
            commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 0);
            SetRootDescriptor(g_uploadAllocators[g_frame].allocate<false>(&constants, sizeof(constants), 0x100), 2);
            commandList->setFramebuffer(framebuffer.get());
            SetHostViewport(commandList.get(), RenderViewport(0.0f, 0.0f, g_swapChain->getWidth(), g_swapChain->getHeight()));
            SetHostScissor(commandList.get(), RenderRect(0, 0, g_swapChain->getWidth(), g_swapChain->getHeight()));
            commandList->drawInstanced(6, 1, 0, 0);
            commandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(swapChainTexture, RenderTextureLayout::PRESENT));
        }
        else
        {
            AddBarrier(g_backBuffer, RenderTextureLayout::PRESENT);
            FlushBarriers();
        }
    }

    auto &commandList = g_commandLists[g_frame];
    commandList->writeTimestamp(g_queryPools[g_frame].get(), 1);

    // Vulkan reads the pool back whole: every query must have been written.
    if (g_vulkan && g_gpuSegmentCounts[g_frame] != GPU_SEGMENTS_UNUSED)
    {
        for (uint32_t i = g_gpuSegmentCounts[g_frame]; i < NUM_SEGMENT_QUERIES; i++)
            commandList->writeTimestamp(g_segmentQueryPools[g_frame].get(), i);
    }

    commandList->end();

    if (g_swapChainValid)
    {
        const RenderCommandList *commandLists[] = { commandList.get() };
        RenderCommandSemaphore *waitSemaphores[] = { g_acquireSemaphores[g_frame].get() };
        RenderCommandSemaphore *signalSemaphores[] = { g_renderSemaphores[g_frame].get() };

        g_queue->executeCommandLists(
            commandLists, std::size(commandLists),
            waitSemaphores, std::size(waitSemaphores),
            signalSemaphores, std::size(signalSemaphores),
            g_commandFences[g_frame].get());
    }
    else
    {
        g_queue->executeCommandLists(commandList.get(), g_commandFences[g_frame].get());
    }

    g_commandListStates[g_frame] = true;

    g_executedCommandList = true;
    g_executedCommandList.notify_one();
}

static void ProcBeginCommandList(const RenderCommand& cmd)
{
    DestructTempResources();
    BeginCommandList();
}

static GuestSurface* GetBackBuffer() 
{
    g_backBuffer->AddRef();
    return g_backBuffer;
}

void Video::ComputeViewportDimensions()
{
    if (VR::IsActive())
    {
        // The game draws for a screen of the configured shape (4:3 for 4:3 and
        // Original 4:3, 16:9 otherwise) with as many pixels as an eye's image: its
        // 2D (HUD, menus, movies) is laid out as on that screen, and the HUD panel
        // and the floating screen take its shape. The eyes' images are stretched
        // from it, each eye's lens being in the projection it is drawn with. The
        // window only mirrors the headset.
        bool narrow = Config::AspectRatio == EAspectRatio::Narrow || Config::AspectRatio == EAspectRatio::OriginalNarrow;
        double aspectRatio = narrow ? 4.0 / 3.0 : 16.0 / 9.0;
        double pixels = double(VR::GetRenderWidth()) * double(VR::GetRenderHeight());

        s_viewportHeight = std::max(2u, uint32_t(round(sqrt(pixels / aspectRatio))) & ~1u);
        s_viewportWidth = std::max(2u, uint32_t(round(s_viewportHeight * aspectRatio)) & ~1u);
        AspectRatioPatches::ComputeOffsets();
        return;
    }

    uint32_t width = g_swapChain->getWidth();
    uint32_t height = g_swapChain->getHeight();
    float aspectRatio = float(width) / float(height);

    switch (Config::AspectRatio)
    {
    case EAspectRatio::Wide:
    {
        if (aspectRatio > WIDE_ASPECT_RATIO)
        {
            s_viewportWidth = height * 16 / 9;
            s_viewportHeight = height;
        }
        else
        {
            s_viewportWidth = width;
            s_viewportHeight = width * 9 / 16;
        }

        break;
    }

    case EAspectRatio::Narrow:
    case EAspectRatio::OriginalNarrow:
    {
        if (aspectRatio > NARROW_ASPECT_RATIO)
        {
            s_viewportWidth = height * 4 / 3;
            s_viewportHeight = height;
        }
        else
        {
            s_viewportWidth = width;
            s_viewportHeight = width * 3 / 4;
        }

        break;
    }

    default:
        s_viewportWidth = width;
        s_viewportHeight = height;
        break;
    }

    AspectRatioPatches::ComputeOffsets();
}

static RenderFormat ConvertFormat(uint32_t format)
{
    switch (format)
    {
    case D3DFMT_A16B16G16R16F:
    case D3DFMT_A16B16G16R16F_2:
        return RenderFormat::R16G16B16A16_FLOAT;
    case D3DFMT_A8B8G8R8:
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
        return RenderFormat::R8G8B8A8_UNORM;
    case D3DFMT_D24FS8:
    case D3DFMT_D24S8:
        return RenderFormat::D32_FLOAT;
    case D3DFMT_G16R16F:
    case D3DFMT_G16R16F_2:
        return RenderFormat::R16G16_FLOAT;
    case D3DFMT_INDEX16:
        return RenderFormat::R16_UINT;
    case D3DFMT_INDEX32:
        return RenderFormat::R32_UINT;
    case D3DFMT_L8:
    case D3DFMT_L8_2:
        return RenderFormat::R8_UNORM;
    default:
        assert(false && "Unknown format");
        return RenderFormat::R16G16B16A16_FLOAT;
    }
}

static GuestTexture* CreateTexture(uint32_t width, uint32_t height, uint32_t depth, uint32_t levels, uint32_t usage, uint32_t format, uint32_t pool, uint32_t type) 
{
    const auto texture = g_userHeap.AllocPhysical<GuestTexture>(type == 17 ? ResourceType::VolumeTexture : ResourceType::Texture);

    RenderTextureDesc desc;
    desc.dimension = texture->type == ResourceType::VolumeTexture ? RenderTextureDimension::TEXTURE_3D : RenderTextureDimension::TEXTURE_2D;
    desc.width = width;
    desc.height = height;
    desc.depth = depth;
    desc.mipLevels = levels;
    desc.arraySize = 1;
    desc.format = ConvertFormat(format);

    if (desc.format == RenderFormat::D32_FLOAT)
        desc.flags = RenderTextureFlag::DEPTH_TARGET;
    else if (usage != 0)
        desc.flags = RenderTextureFlag::RENDER_TARGET;
    else
        desc.flags = RenderTextureFlag::NONE;

    // VR single-pass stereo: a texture the game resolves into has the layers of
    // a surface of its size.
    if (desc.dimension == RenderTextureDimension::TEXTURE_2D && (desc.flags & (RenderTextureFlag::RENDER_TARGET | RenderTextureFlag::DEPTH_TARGET)) != 0)
        desc.arraySize = GetVRLayerCount(width, height);

    texture->textureHolder = g_device->createTexture(desc);
    texture->texture = texture->textureHolder.get();

    RenderTextureViewDimension viewDimension = texture->type == ResourceType::VolumeTexture ? RenderTextureViewDimension::TEXTURE_3D : RenderTextureViewDimension::TEXTURE_2D;

    RenderTextureViewDesc viewDesc;
    viewDesc.format = desc.format;
    viewDesc.dimension = GetShaderViewDimension(viewDimension);
    viewDesc.mipLevels = levels;

    switch (format)
    {
    case D3DFMT_D24FS8:
    case D3DFMT_D24S8:
    case D3DFMT_L8:
    case D3DFMT_L8_2:
        viewDesc.componentMapping = RenderComponentMapping(RenderSwizzle::R, RenderSwizzle::R, RenderSwizzle::R, RenderSwizzle::ONE);
        break;

    case D3DFMT_X8R8G8B8:
        viewDesc.componentMapping = RenderComponentMapping(RenderSwizzle::G, RenderSwizzle::B, RenderSwizzle::A, RenderSwizzle::ONE);
        break;
    }

    texture->textureView = texture->texture->createTextureView(viewDesc);

    texture->width = width;
    texture->height = height;
    texture->depth = depth;
    texture->format = desc.format;
    texture->layerCount = desc.arraySize;
    texture->viewDimension = viewDimension;
    texture->descriptorIndex = g_textureDescriptorAllocator.allocate();

    g_textureDescriptorSet->setTexture(texture->descriptorIndex, texture->texture, RenderTextureLayout::SHADER_READ, texture->textureView.get());
   
#ifdef _DEBUG 
    texture->texture->setName(fmt::format("Texture {:X}", g_memory.MapVirtual(texture)));
#endif

    return texture;
}

static RenderHeapType GetBufferHeapType()
{
    return g_capabilities.gpuUploadHeap ? RenderHeapType::GPU_UPLOAD : RenderHeapType::DEFAULT;
}

static GuestBuffer* CreateVertexBuffer(uint32_t length) 
{
    auto buffer = g_userHeap.AllocPhysical<GuestBuffer>(ResourceType::VertexBuffer);
    buffer->buffer = g_device->createBuffer(RenderBufferDesc::VertexBuffer(length, GetBufferHeapType(), RenderBufferFlag::INDEX));
    buffer->dataSize = length;
#ifdef _DEBUG 
    buffer->buffer->setName(fmt::format("Vertex Buffer {:X}", g_memory.MapVirtual(buffer)));
#endif
    return buffer;
}

static GuestBuffer* CreateIndexBuffer(uint32_t length, uint32_t, uint32_t format)
{
    auto buffer = g_userHeap.AllocPhysical<GuestBuffer>(ResourceType::IndexBuffer);
    buffer->buffer = g_device->createBuffer(RenderBufferDesc::IndexBuffer(length, GetBufferHeapType()));
    buffer->dataSize = length;
    buffer->format = ConvertFormat(format);
    buffer->guestFormat = format;
#ifdef _DEBUG 
    buffer->buffer->setName(fmt::format("Index Buffer {:X}", g_memory.MapVirtual(buffer)));
#endif
    return buffer;
}

static GuestSurface* CreateSurface(uint32_t width, uint32_t height, uint32_t format, uint32_t multiSample) 
{
    RenderTextureDesc desc;
    desc.dimension = RenderTextureDimension::TEXTURE_2D;
    desc.width = width;
    desc.height = height;
    desc.depth = 1;
    desc.mipLevels = 1;
    desc.arraySize = GetVRLayerCount(width, height);
    desc.multisampling.sampleCount = multiSample != 0 && Config::AntiAliasing != EAntiAliasing::None ? int32_t(Config::AntiAliasing.Value) : RenderSampleCount::COUNT_1;
    desc.format = ConvertFormat(format);
    desc.flags = desc.format == RenderFormat::D32_FLOAT ? RenderTextureFlag::DEPTH_TARGET : RenderTextureFlag::RENDER_TARGET;

    auto surface = g_userHeap.AllocPhysical<GuestSurface>(desc.format == RenderFormat::D32_FLOAT ? 
        ResourceType::DepthStencil : ResourceType::RenderTarget);

    surface->textureHolder = g_device->createTexture(desc);
    surface->texture = surface->textureHolder.get();
    surface->width = width;
    surface->height = height;
    surface->format = desc.format;
    surface->guestFormat = format;
    surface->layerCount = desc.arraySize;
    surface->sampleCount = desc.multisampling.sampleCount;

    RenderTextureViewDesc viewDesc;
    viewDesc.dimension = GetShaderViewDimension(RenderTextureViewDimension::TEXTURE_2D);
    viewDesc.format = desc.format;
    viewDesc.mipLevels = 1;
    surface->textureView = surface->textureHolder->createTextureView(viewDesc);
    surface->descriptorIndex = g_textureDescriptorAllocator.allocate();
    g_textureDescriptorSet->setTexture(surface->descriptorIndex, surface->textureHolder.get(), RenderTextureLayout::SHADER_READ, surface->textureView.get());

#ifdef _DEBUG 
    surface->texture->setName(fmt::format("{} {:X}", desc.flags & RenderTextureFlag::RENDER_TARGET ? "Render Target" : "Depth Stencil", g_memory.MapVirtual(surface)));
#endif

    return surface;
}

static void FlushViewport()
{
    auto& commandList = g_commandLists[g_frame];

    if (g_dirtyStates.viewport)
    {
        auto viewport = g_viewport;

        if (viewport.minDepth > viewport.maxDepth)
            std::swap(viewport.minDepth, viewport.maxDepth);

        SetHostViewport(commandList.get(), viewport);

        g_dirtyStates.viewport = false;
    }

    // A rectangle per eye in single-pass stereo, each moved onto that eye's view
    // of the HUD panel with a draw put there.
    if (g_dirtyStates.scissorRect)
    {
        RenderRect scissorRects[2];
        uint32_t scissorCount = GetVRViewportCount();

        for (uint32_t i = 0; i < scissorCount; i++)
        {
            if (g_scissorTestEnable && g_vrScissorOnPanel)
            {
                scissorRects[i] = GetVRPanelScissorRect(g_vrSinglePass ? i : uint32_t(g_vrPass));
            }
            else
            {
                scissorRects[i] = g_scissorTestEnable ? g_scissorRect : RenderRect(
                    g_viewport.x,
                    g_viewport.y,
                    g_viewport.x + g_viewport.width,
                    g_viewport.y + g_viewport.height);
            }
        }

        commandList->setScissors(scissorRects, scissorCount);

        g_dirtyStates.scissorRect = false;
    }
}

static void StretchRect(GuestDevice* device, uint32_t flags, uint32_t, GuestTexture* texture)
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::StretchRect;
    cmd.stretchRect.flags = flags;
    cmd.stretchRect.texture = texture;
    g_renderQueue.enqueue(cmd);
}

static void SetTextureInRenderThread(uint32_t index, GuestTexture* texture);
static void SetSurface(uint32_t index, GuestSurface* surface);

static void ProcStretchRect(const RenderCommand& cmd)
{
    const auto& args = cmd.stretchRect;

    const bool isDepthStencil = (args.flags & 0x4) != 0;
    const auto surface = isDepthStencil ? g_depthStencil : g_renderTarget;

    // VR: a shadow map texture the left eye's pass copied into keeps that copy.
    // One still reading its surface (the copy dropped at the end of the frame,
    // depth being transient) is pointed at it again: the surface still holds
    // what the left eye's pass drew into it last.
    if (g_vrPass == 1 && Config::VRReuseShadows && IsVRShadowMap(surface) && args.texture->vrResolveCopied)
    {
        ++g_vrStatistics.shadowWorkReused;
        return;
    }

    if (g_vrPass == 0)
        NoteVRSurfaceResolve(surface, args.texture);

    // Erase previous pending command so it doesn't cause the texture to be overriden.
    if (args.texture->sourceSurface != nullptr)
        args.texture->sourceSurface->destinationTextures.erase(args.texture);

    args.texture->sourceSurface = surface;
    args.texture->vrResolvedFrom = surface;
    args.texture->vrResolveCopied = false;
    surface->destinationTextures.emplace(args.texture);

    // If the texture is assigned to any slots, set it again. This'll also push the barrier.
    for (uint32_t i = 0; i < std::size(g_textures); i++)
    {
        if (g_textures[i] == args.texture)
        {
            // Set the original texture for MSAA textures as they always get resolved.
            if (surface->sampleCount != RenderSampleCount::COUNT_1)
            {
                SetTextureInRenderThread(i, args.texture);
                g_pendingMsaaResolves.emplace(surface);
            }
            else
            {
                SetSurface(i, surface);
            }
        }
    }

    // Remember to clear later.
    g_pendingSurfaceCopies.emplace(surface);
}

static void SetDefaultViewport(GuestDevice* device, GuestSurface* surface)
{
    if (surface != nullptr)
    {
        RenderCommand cmd;
        cmd.type = RenderCommandType::SetViewport;
        cmd.setViewport.x = 0.0f;
        cmd.setViewport.y = 0.0f;
        cmd.setViewport.width = float(surface->width);
        cmd.setViewport.height = float(surface->height);
        cmd.setViewport.minDepth = 0.0f;
        cmd.setViewport.maxDepth = 1.0f;
        g_renderQueue.enqueue(cmd);

        device->viewport.x = 0.0f;
        device->viewport.y = 0.0f;
        device->viewport.width = float(surface->width);
        device->viewport.height = float(surface->height);
        device->viewport.minZ = 0.0f;
        device->viewport.maxZ = 1.0f;
    }
}

static void SetRenderTarget(GuestDevice* device, uint32_t index, GuestSurface* renderTarget) 
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetRenderTarget;
    cmd.setRenderTarget.renderTarget = renderTarget;
    g_renderQueue.enqueue(cmd);

    SetDefaultViewport(device, renderTarget);
}

static void ProcSetRenderTarget(const RenderCommand& cmd)
{
    const auto& args = cmd.setRenderTarget;

    SetDirtyValue(g_dirtyStates.renderTargetAndDepthStencil, g_renderTarget, args.renderTarget);
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.renderTargetFormat, args.renderTarget != nullptr ? args.renderTarget->format : RenderFormat::UNKNOWN);
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.sampleCount, args.renderTarget != nullptr ? args.renderTarget->sampleCount : RenderSampleCount::COUNT_1);

    // When alpha to coverage is enabled, update the alpha test mode as it's dependent on sample count.
    SetAlphaTestMode((g_pipelineState.specConstants & (SPEC_CONSTANT_ALPHA_TEST | SPEC_CONSTANT_ALPHA_TO_COVERAGE)) != 0);
}

static void SetDepthStencilSurface(GuestDevice* device, GuestSurface* depthStencil) 
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetDepthStencilSurface;
    cmd.setDepthStencilSurface.depthStencil = depthStencil;
    g_renderQueue.enqueue(cmd);

    SetDefaultViewport(device, depthStencil);
}

static void ProcSetDepthStencilSurface(const RenderCommand& cmd)
{
    const auto& args = cmd.setDepthStencilSurface;

    SetDirtyValue(g_dirtyStates.renderTargetAndDepthStencil, g_depthStencil, args.depthStencil);
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthStencilFormat, args.depthStencil != nullptr ? args.depthStencil->format : RenderFormat::UNKNOWN);
}

static bool PopulateBarriersForStretchRect(GuestSurface* renderTarget, GuestSurface* depthStencil)
{
    bool addedAny = false;

    for (const auto surface : { renderTarget, depthStencil })
    {
        if (surface != nullptr && !surface->destinationTextures.empty())
        {
            const bool multiSampling = surface->sampleCount != RenderSampleCount::COUNT_1;

            RenderTextureLayout srcLayout;
            RenderTextureLayout dstLayout;
            bool shaderResolve = true;

            if (multiSampling && g_hardwareResolve)
            {
                // Hardware depth resolve is only supported on D3D12 when programmable sample positions are available.
                bool hardwareDepthResolveAvailable = g_hardwareDepthResolve && !g_vulkan && g_capabilities.sampleLocations;

                if (surface->format != RenderFormat::D32_FLOAT || hardwareDepthResolveAvailable)
                {
                    srcLayout = RenderTextureLayout::RESOLVE_SOURCE;
                    dstLayout = RenderTextureLayout::RESOLVE_DEST;
                    shaderResolve = false;
                }
            }
            
            if (shaderResolve)
            {
                srcLayout = RenderTextureLayout::SHADER_READ;
                dstLayout = (surface->format == RenderFormat::D32_FLOAT ? RenderTextureLayout::DEPTH_WRITE : RenderTextureLayout::COLOR_WRITE);
            }

            AddBarrier(surface, srcLayout);

            for (const auto texture : surface->destinationTextures)
                AddBarrier(texture, dstLayout);

            addedAny = true;
        }
    }

    return addedAny;
}

static void ExecutePendingStretchRectCommands(GuestSurface* renderTarget, GuestSurface* depthStencil)
{
    auto& commandList = g_commandLists[g_frame];

    for (const auto surface : { renderTarget, depthStencil })
    {
        if (surface != nullptr && !surface->destinationTextures.empty())
        {
            const bool multiSampling = surface->sampleCount != RenderSampleCount::COUNT_1;

            for (const auto texture : surface->destinationTextures)
            {
                bool shaderResolve = true;

                if (multiSampling && g_hardwareResolve)
                {
                    bool hardwareDepthResolveAvailable = g_hardwareDepthResolve && !g_vulkan && g_capabilities.sampleLocations;

                    if (surface->format != RenderFormat::D32_FLOAT || hardwareDepthResolveAvailable)
                    {
                        if (surface->format == RenderFormat::D32_FLOAT)
                            commandList->resolveTextureRegion(texture->texture, 0, 0, surface->texture, nullptr, RenderResolveMode::MIN);
                        else
                            commandList->resolveTexture(texture->texture, surface->texture);

                        shaderResolve = false;
                    }
                }

                if (shaderResolve)
                {
                    RenderPipeline* pipeline = nullptr;

                    if (multiSampling)
                    {
                        uint32_t pipelineIndex = 0;

                        switch (surface->sampleCount)
                        {
                        case RenderSampleCount::COUNT_2:
                            pipelineIndex = 0;
                            break;
                        case RenderSampleCount::COUNT_4:
                            pipelineIndex = 1;
                            break;
                        case RenderSampleCount::COUNT_8:
                            pipelineIndex = 2;
                            break;
                        default:
                            assert(false && "Unsupported MSAA sample count");
                            break;
                        }

                        if (texture->format == RenderFormat::D32_FLOAT)
                        {
                            pipeline = g_resolveMsaaDepthPipelines[pipelineIndex].get();
                        }
                        else
                        {
                            auto& resolveMsaaColorPipeline = g_resolveMsaaColorPipelines[surface->format][pipelineIndex];
                            if (resolveMsaaColorPipeline == nullptr)
                            {
                                RenderGraphicsPipelineDesc desc;
                                desc.pipelineLayout = g_pipelineLayout.get();
                                desc.vertexShader = g_copyShader.get();
                                desc.pixelShader = g_resolveMsaaColorShaders[pipelineIndex].get();
                                desc.renderTargetFormat[0] = texture->format;
                                desc.renderTargetBlend[0] = RenderBlendDesc::Copy();
                                desc.renderTargetCount = 1;
                                desc.viewportCount = GetVRViewportCount();
                                resolveMsaaColorPipeline = g_device->createGraphicsPipeline(desc);
                            }

                            pipeline = resolveMsaaColorPipeline.get();
                        }
                    }
                    else
                    {
                        if (texture->format == RenderFormat::D32_FLOAT)
                        {
                            pipeline = g_copyDepthPipeline.get();
                        }
                        else
                        {
                            auto& copyColorPipeline = g_copyColorPipelines[surface->format];
                            if (copyColorPipeline == nullptr)
                            {
                                RenderGraphicsPipelineDesc desc;
                                desc.pipelineLayout = g_pipelineLayout.get();
                                desc.vertexShader = g_copyShader.get();
                                desc.pixelShader = g_copyColorShader.get();
                                desc.renderTargetFormat[0] = texture->format;
                                desc.renderTargetBlend[0] = RenderBlendDesc::Copy();
                                desc.renderTargetCount = 1;
                                desc.viewportCount = GetVRViewportCount();
                                copyColorPipeline = g_device->createGraphicsPipeline(desc);
                            }

                            pipeline = copyColorPipeline.get();
                        }
                    }

                    if (texture->framebuffer == nullptr)
                    {
                        if (texture->format == RenderFormat::D32_FLOAT)
                        {
                            RenderFramebufferDesc desc;
                            desc.depthAttachment = texture->texture;
                            texture->framebuffer = g_device->createFramebuffer(desc);
                        }
                        else
                        {
                            RenderFramebufferDesc desc;
                            desc.colorAttachments = const_cast<const RenderTexture**>(&texture->texture);
                            desc.colorAttachmentsCount = 1;
                            texture->framebuffer = g_device->createFramebuffer(desc);
                        }
                    }

                    if (g_framebuffer != texture->framebuffer.get())
                    {
                        commandList->setFramebuffer(texture->framebuffer.get());
                        g_framebuffer = texture->framebuffer.get();
                    }

                    // An instance per layer of the texture (single-pass stereo), at the full rate.
                    SetShadingRateImage(nullptr);
                    commandList->setPipeline(pipeline);
                    SetHostViewport(commandList.get(), RenderViewport(0.0f, 0.0f, float(texture->width), float(texture->height), 0.0f, 1.0f));
                    SetHostScissor(commandList.get(), RenderRect(0, 0, texture->width, texture->height));
                    commandList->setGraphicsPushConstants(0, &surface->descriptorIndex, 0, sizeof(uint32_t));
                    commandList->drawInstanced(6, texture->layerCount, 0, 0);

                    g_dirtyStates.renderTargetAndDepthStencil = true;
                    g_dirtyStates.viewport = true;
                    g_dirtyStates.pipelineState = true;
                    g_dirtyStates.scissorRect = true;

                    if (g_vulkan)
                    {
                        g_dirtyStates.vertexShaderConstants = true; // The push constant call invalidates vertex shader constants.
                        g_dirtyStates.depthBias = true; // Static depth bias in copy pipeline invalidates dynamic depth bias.
                    }
                }

                texture->sourceSurface = nullptr;
                texture->vrResolveCopied = true;

                // Check if any texture slots had this texture assigned, and make it point back at the original texture.
                for (uint32_t i = 0; i < std::size(g_textures); i++)
                {
                    if (g_textures[i] == texture)
                        SetTextureInRenderThread(i, texture);
                }
            }

            surface->destinationTextures.clear();
        }
    }
}

static void ProcExecutePendingStretchRectCommands(const RenderCommand& cmd)
{
    bool foundAny = false;

    for (const auto surface : g_pendingSurfaceCopies)
    {
        // Depth stencil textures in this game are guaranteed to be transient.
        if (surface->format != RenderFormat::D32_FLOAT)
            foundAny |= PopulateBarriersForStretchRect(surface, nullptr);
    }

    if (foundAny)
    {
        FlushBarriers();

        for (const auto surface : g_pendingSurfaceCopies)
        {
            if (surface->format != RenderFormat::D32_FLOAT)
                ExecutePendingStretchRectCommands(surface, nullptr);

            for (const auto texture : surface->destinationTextures)
                texture->sourceSurface = nullptr;

            surface->destinationTextures.clear();
        }
    }

    g_pendingSurfaceCopies.clear();
    g_pendingMsaaResolves.clear();
}

static void SetFramebuffer(GuestSurface* renderTarget, GuestSurface* depthStencil, bool settingForClear)
{
    if (settingForClear || g_dirtyStates.renderTargetAndDepthStencil)
    {
        GuestSurface* framebufferContainer = nullptr;
        RenderTexture* framebufferKey = nullptr;

        if (renderTarget != nullptr && depthStencil != nullptr)
        {
            framebufferContainer = depthStencil; // Backbuffer texture changes per frame so we can't use the depth stencil as the key.
            framebufferKey = renderTarget->texture;
        }
        else if (renderTarget != nullptr && depthStencil == nullptr)
        {
            framebufferContainer = renderTarget;
            framebufferKey = renderTarget->texture; // Backbuffer texture changes per frame so we can't assume nullptr for it.
        }
        else if (renderTarget == nullptr && depthStencil != nullptr)
        {
            framebufferContainer = depthStencil;
            framebufferKey = nullptr;
        }

        auto& commandList = g_commandLists[g_frame];

        if (framebufferContainer != nullptr)
        {
            auto& framebuffer = framebufferContainer->framebuffers[framebufferKey];

            if (framebuffer == nullptr)
            {
                RenderFramebufferDesc desc;

                if (renderTarget != nullptr)
                {
                    desc.colorAttachments = const_cast<const RenderTexture**>(&renderTarget->texture);
                    desc.colorAttachmentsCount = 1;
                }

                if (depthStencil != nullptr)
                    desc.depthAttachment = depthStencil->texture;

                framebuffer = g_device->createFramebuffer(desc);
            }

            if (g_framebuffer != framebuffer.get())
            {
                commandList->setFramebuffer(framebuffer.get());
                g_framebuffer = framebuffer.get();
            }
        }
        else if (g_framebuffer != nullptr)
        {
            commandList->setFramebuffer(nullptr);
            g_framebuffer = nullptr;
        }

        if (g_framebuffer != nullptr)
        {
            SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.halfPixelOffsetX, 1.0f / float(g_framebuffer->getWidth()));
            SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.halfPixelOffsetY, -1.0f / float(g_framebuffer->getHeight()));
        }

        g_dirtyStates.renderTargetAndDepthStencil = settingForClear;
    }
}

static void Clear(GuestDevice* device, uint32_t flags, uint32_t, be<float>* color, double z) 
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::Clear;
    cmd.clear.flags = flags;
    cmd.clear.color[0] = color[0];
    cmd.clear.color[1] = color[1];
    cmd.clear.color[2] = color[2];
    cmd.clear.color[3] = color[3];
    cmd.clear.z = float(z);
    g_renderQueue.enqueue(cmd);
}

// Builds the foveation images (see VRFoveation) for the scene's size and the
// eyes' lenses, when they changed. Called at the start of a frame's command list.
static void UpdateVRFoveation()
{
    VRFoveation& f = g_vrFoveation;
    g_vrRetiredFoveationImages[g_frame].clear();

    int32_t level = std::clamp(Config::VRFoveation.Value, 0, 3);
    if (level == 0 || !g_capabilities.shadingRateImage || g_capabilities.shadingRateImageTileSize == 0 || !g_vrEyesValid)
        return;

    int32_t sceneWidth, sceneHeight;
    GetVRSceneSize(sceneWidth, sceneHeight);
    if (sceneWidth <= 0 || sceneHeight <= 0)
        return;

    bool rebuild = !f.built || f.level != level || f.sceneWidth != uint32_t(sceneWidth) || f.sceneHeight != uint32_t(sceneHeight);
    for (uint32_t eye = 0; eye < 2 && !rebuild; eye++)
    {
        for (uint32_t i = 0; i < 4; i++)
            rebuild |= std::abs(f.terms[eye][i] - g_vrEyes[eye].projectionTerms[i]) > 1.0e-3f;
    }

    if (!rebuild)
        return;

    for (auto& image : f.images)
    {
        if (image != nullptr)
            g_vrRetiredFoveationImages[g_frame].push_back(std::move(image));
    }

    f.level = level;
    f.sceneWidth = uint32_t(sceneWidth);
    f.sceneHeight = uint32_t(sceneHeight);
    for (uint32_t eye = 0; eye < 2; eye++)
        memcpy(f.terms[eye], g_vrEyes[eye].projectionTerms, sizeof(f.terms[eye]));

    // The angle off each lens's axis (degrees) up to which tiles keep the full
    // rate, and up to which they are shaded at 2x2; beyond, 4x4 where available.
    static constexpr float FullRateAngles[] = { 0.0f, 35.0f, 27.0f, 20.0f };
    static constexpr float HalfRateAngles[] = { 0.0f, 90.0f, 40.0f, 32.0f };

    // D3D12_SHADING_RATE values (Vulkan's attachment texels encode sizes alike).
    constexpr uint8_t Rate1x1 = 0x0;
    constexpr uint8_t Rate2x2 = 0x5;
    constexpr uint8_t Rate4x4 = 0xA;
    const uint8_t rates[3] = { Rate1x1, Rate2x2, g_capabilities.shadingRateImageCoarseRates ? Rate4x4 : Rate2x2 };

    uint32_t tileSize = g_capabilities.shadingRateImageTileSize;
    uint32_t tilesX = (f.sceneWidth + tileSize - 1) / tileSize;
    uint32_t tilesY = (f.sceneHeight + tileSize - 1) / tileSize;

    bool attachment = g_capabilities.shadingRateAttachment;
    if (attachment && (g_vrShadingRateAttachment == nullptr || !g_vrShadingRateAttachmentReady ||
        tilesX > g_vrShadingRateAttachmentSize || tilesY > g_vrShadingRateAttachmentSize))
    {
        f.built = false;
        return;
    }

    f.built = true;
    f.tilesX = tilesX;
    f.tilesY = tilesY;

    float fullRateTangent = tanf(FullRateAngles[level] * 3.14159265f / 180.0f);
    float halfRateTangent = tanf(std::min(HalfRateAngles[level], 89.0f) * 3.14159265f / 180.0f);

    for (auto& pattern : f.patterns)
        pattern.assign(size_t(tilesX) * tilesY, Rate1x1);

    memset(f.tileCounts, 0, sizeof(f.tileCounts));

    for (uint32_t y = 0; y < tilesY; y++)
    {
        for (uint32_t x = 0; x < tilesX; x++)
        {
            // The tile's centre in normalised device coordinates.
            float ndcX = (std::min(float(x * tileSize) + 0.5f * tileSize, float(f.sceneWidth)) / float(f.sceneWidth)) * 2.0f - 1.0f;
            float ndcY = 1.0f - (std::min(float(y * tileSize) + 0.5f * tileSize, float(f.sceneHeight)) / float(f.sceneHeight)) * 2.0f;

            uint32_t rateIndices[2];
            for (uint32_t eye = 0; eye < 2; eye++)
            {
                // The tangent of the angle off the eye's lens axis.
                const float* terms = f.terms[eye];
                float tangentX = (ndcX + terms[2]) / terms[0];
                float tangentY = (ndcY + terms[3]) / terms[1];
                float tangent = sqrtf(tangentX * tangentX + tangentY * tangentY);

                rateIndices[eye] = tangent <= fullRateTangent ? 0 : (tangent <= halfRateTangent ? 1 : 2);
                f.patterns[eye][size_t(y) * tilesX + x] = rates[rateIndices[eye]];
            }

            f.patterns[2][size_t(y) * tilesX + x] = rates[std::min(rateIndices[0], rateIndices[1])];
            ++f.tileCounts[rates[rateIndices[0]] == Rate1x1 ? 0 : (rates[rateIndices[0]] == Rate2x2 ? 1 : 2)];
        }
    }

    if (attachment)
    {
        // Single-pass stereo: a layer per eye, or the shared pattern in the only
        // one. Frame replay: BeginVRFrame and RestoreVRFrameStart write the eye's.
        if (g_vrSinglePass)
        {
            if (g_vrShadingRateAttachmentLayers >= 2)
            {
                UploadVRFoveationPattern(g_vrShadingRateAttachment.get(), 0, 0);
                UploadVRFoveationPattern(g_vrShadingRateAttachment.get(), 1, 1);
            }
            else
            {
                UploadVRFoveationPattern(g_vrShadingRateAttachment.get(), 0, 2);
            }
        }
    }
    else if (g_vrSinglePass)
    {
        // A layer per eye, or the shared pattern where one layer serves both.
        bool layered = g_capabilities.shadingRateImageLayers;
        RenderTextureDesc desc = RenderTextureDesc::Texture2D(tilesX, tilesY, 1, RenderFormat::R8_UINT, RenderTextureFlag::NONE);
        desc.arraySize = layered ? 2 : 1;
        f.images[2] = g_device->createTexture(desc);

        for (uint32_t layer = 0; layer < desc.arraySize; layer++)
            UploadVRFoveationPattern(f.images[2].get(), layer, layered ? layer : 2);
    }
    else
    {
        for (uint32_t eye = 0; eye < 2; eye++)
        {
            f.images[eye] = g_device->createTexture(RenderTextureDesc::Texture2D(tilesX, tilesY, 1, RenderFormat::R8_UINT, RenderTextureFlag::NONE));
            UploadVRFoveationPattern(f.images[eye].get(), 0, eye);
        }
    }

    LOGFN("[VR] Foveated rendering (level {}): {}x{} tiles of {} pixels; each eye's image is {:.0f}% at full rate{}.",
        level, tilesX, tilesY, tileSize, 100.0 * f.tileCounts[0] / std::max(1u, tilesX * tilesY),
        (g_vrSinglePass && !g_capabilities.shadingRateImageLayers) ? " (one image for both eyes: the finer rate of the two)" : "");
}

// Copies a foveation pattern (an eye's, or 2 for the shared one) into a layer of
// a shading rate image, in the corner the scene's targets cover.
static void UploadVRFoveationPattern(RenderTexture* texture, uint32_t layer, uint32_t pattern)
{
    const VRFoveation& f = g_vrFoveation;
    if (texture == nullptr || f.tilesX == 0 || f.tilesY == 0 || f.patterns[pattern].size() != size_t(f.tilesX) * f.tilesY)
        return;

    uint32_t pitch = (f.tilesX + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
    auto allocation = g_uploadAllocators[g_frame].allocate(pitch * f.tilesY, PLACEMENT_ALIGNMENT);

    for (uint32_t y = 0; y < f.tilesY; y++)
        memcpy(allocation.memory + size_t(y) * pitch, f.patterns[pattern].data() + size_t(y) * f.tilesX, f.tilesX);

    auto& commandList = g_commandLists[g_frame];
    commandList->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(texture, RenderTextureLayout::COPY_DEST));
    commandList->copyTextureRegion(
        RenderTextureCopyLocation::Subresource(texture, 0, layer),
        RenderTextureCopyLocation::PlacedFootprint(allocation.buffer, RenderFormat::R8_UINT, f.tilesX, f.tilesY, 1, pitch, allocation.offset));
    commandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(texture, RenderTextureLayout::SHADING_RATE_SOURCE));
}

// VR: draws the hidden area mask (see g_vrHiddenAreaFrame) into the depth buffer
// just cleared to `clearZ`, at the opposite end of the depth range, for the eye
// being drawn or in single-pass stereo for each eye's layer.
static void DrawVRHiddenAreaMask(float clearZ)
{
    GuestSurface* depthStencil = g_depthStencil;

    int32_t sceneWidth, sceneHeight;
    GetVRSceneSize(sceneWidth, sceneHeight);

    // The scene's own depth buffers: of its size, and drawn through the game camera before.
    if (g_vrPass < 0 || !g_vrHiddenAreaFrame || depthStencil == nullptr || !depthStencil->vrPerspectiveDepth ||
        std::abs(int32_t(depthStencil->width) - sceneWidth) > 2 || std::abs(int32_t(depthStencil->height) - sceneHeight) > 2)
    {
        return;
    }

    uint32_t pipelineIndex;
    switch (depthStencil->sampleCount)
    {
    case RenderSampleCount::COUNT_1:
        pipelineIndex = 0;
        break;
    case RenderSampleCount::COUNT_2:
        pipelineIndex = 1;
        break;
    case RenderSampleCount::COUNT_4:
        pipelineIndex = 2;
        break;
    case RenderSampleCount::COUNT_8:
        pipelineIndex = 3;
        break;
    default:
        return;
    }

    // The D3D12 shader declares 4095 registers of two vertices.
    constexpr size_t MaxVertices = 4095 * 2;

    auto& commandList = g_commandLists[g_frame];
    bool bound = false;

    for (uint32_t layer = 0; layer < depthStencil->layerCount; layer++)
    {
        uint32_t eye = g_vrSinglePass ? layer : uint32_t(g_vrPass);
        const std::vector<float>& mesh = g_vrHiddenAreaMeshes[eye];
        size_t vertexCount = mesh.size() / 2;

        if (eye >= 2 || vertexCount < 3 || vertexCount > MaxVertices)
            continue;

        if (!bound)
        {
            // Compared the way the game's draws compare (a reversed depth range
            // is cleared to 0): a write against that direction may turn the
            // GPU's coarse depth culling off until the next clear.
            bool reversed = clearZ < 0.5f;
            auto& pipeline = g_vrMaskPipelines[pipelineIndex][reversed ? 1 : 0];
            if (pipeline == nullptr)
            {
                if (g_vrMaskShader == nullptr)
                    g_vrMaskShader = CREATE_SHADER(xr_mask_vs);

                RenderGraphicsPipelineDesc desc;
                desc.pipelineLayout = g_pipelineLayout.get();
                desc.vertexShader = g_vrMaskShader.get();
                desc.depthFunction = reversed ? RenderComparisonFunction::GREATER_EQUAL : RenderComparisonFunction::LESS_EQUAL;
                desc.depthEnabled = true;
                desc.depthWriteEnabled = true;
                desc.depthTargetFormat = RenderFormat::D32_FLOAT;
                desc.multisampling.sampleCount = depthStencil->sampleCount;
                desc.viewportCount = GetVRViewportCount();
                pipeline = g_device->createGraphicsPipeline(desc);
            }

            SetFramebuffer(nullptr, depthStencil, true);
            commandList->setPipeline(pipeline.get());
            SetHostViewport(commandList.get(), RenderViewport(0.0f, 0.0f, float(depthStencil->width), float(depthStencil->height), 0.0f, 1.0f));
            SetHostScissor(commandList.get(), RenderRect(0, 0, depthStencil->width, depthStencil->height));
            bound = true;
        }

        // The depth and layer, then the vertices in normalised device coordinates.
        const float* terms = g_vrEyes[eye].projectionTerms;
        auto allocation = g_uploadAllocators[g_frame].allocate(uint32_t(16 + vertexCount * 8), 0x100);
        auto header = reinterpret_cast<uint32_t*>(allocation.memory);
        auto vertices = reinterpret_cast<float*>(allocation.memory + 16);

        float depth = clearZ >= 0.5f ? 0.0f : 1.0f;
        memcpy(&header[0], &depth, sizeof(depth));
        header[1] = layer;
        header[2] = 0;
        header[3] = 0;

        // Computed apart from the upload: upload memory is not read back.
        float area = 0.0f;

        for (size_t i = 0; i + 2 < vertexCount; i += 3)
        {
            float x[3], y[3];
            for (size_t j = 0; j < 3; j++)
            {
                x[j] = mesh[(i + j) * 2 + 0] * terms[0] - terms[2];
                y[j] = mesh[(i + j) * 2 + 1] * terms[1] - terms[3];
                vertices[(i + j) * 2 + 0] = x[j];
                vertices[(i + j) * 2 + 1] = y[j];
            }

            // Clipped to the image only roughly: the mesh hugs its edges.
            area += 0.5f * std::abs((x[1] - x[0]) * (y[2] - y[0]) - (x[2] - x[0]) * (y[1] - y[0]));
        }

        SetRootDescriptor(allocation, 2);
        commandList->drawInstanced(uint32_t(vertexCount / 3 * 3), 1, 0, 0);

        if (g_vrPass == 0)
        {
            ++g_vrStatistics.hiddenAreaDraws;
            g_vrStatistics.hiddenAreaTriangles = uint32_t(vertexCount / 3);
            g_vrStatistics.hiddenAreaShare = area / 4.0f;
        }
    }

    if (bound)
    {
        g_dirtyStates.renderTargetAndDepthStencil = true;
        g_dirtyStates.viewport = true;
        g_dirtyStates.scissorRect = true;
        g_dirtyStates.pipelineState = true;
        g_dirtyStates.sharedConstants = true;

        if (g_capabilities.dynamicDepthBias)
            g_dirtyStates.depthBias = true;
    }
}

static void ProcClear(const RenderCommand& cmd)
{
    const auto& args = cmd.clear;

    if (g_vrPass == 0)
        NoteVRSurfaceClear(g_renderTarget, g_depthStencil, args.flags);

    if (ShouldReuseVRShadowWork())
        return;

    if (PopulateBarriersForStretchRect(g_renderTarget, g_depthStencil))
    {
        FlushBarriers();
        ExecutePendingStretchRectCommands(g_renderTarget, g_depthStencil);
    }

    AddBarrier(g_renderTarget, RenderTextureLayout::COLOR_WRITE);
    AddBarrier(g_depthStencil, RenderTextureLayout::DEPTH_WRITE);
    FlushBarriers();

    bool canClearInOnePass = (g_renderTarget == nullptr) || (g_depthStencil == nullptr) ||
        (g_renderTarget->width == g_depthStencil->width && g_renderTarget->height == g_depthStencil->height);

    if (canClearInOnePass)
        SetFramebuffer(g_renderTarget, g_depthStencil, true);

    auto& commandList = g_commandLists[g_frame];

    if (g_renderTarget != nullptr && (args.flags & D3DCLEAR_TARGET) != 0)
    {
        if (!canClearInOnePass)
            SetFramebuffer(g_renderTarget, nullptr, true);

        commandList->clearColor(0, RenderColor(args.color[0], args.color[1], args.color[2], args.color[3]));
    }

    if (g_depthStencil != nullptr && (args.flags & D3DCLEAR_ZBUFFER) != 0)
    {
        if (!canClearInOnePass)
            SetFramebuffer(nullptr, g_depthStencil, true);

        commandList->clearDepth(true, args.z);
        DrawVRHiddenAreaMask(args.z);
    }
}

static void SetViewport(GuestDevice* device, GuestViewport* viewport)
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetViewport;
    cmd.setViewport.x = viewport->x;
    cmd.setViewport.y = viewport->y;
    cmd.setViewport.width = viewport->width;
    cmd.setViewport.height = viewport->height;
    cmd.setViewport.minDepth = viewport->minZ;
    cmd.setViewport.maxDepth = viewport->maxZ;
    g_renderQueue.enqueue(cmd);

    device->viewport.x = float(viewport->x);
    device->viewport.y = float(viewport->y);
    device->viewport.width = float(viewport->width);
    device->viewport.height = float(viewport->height);
    device->viewport.minZ = viewport->minZ;
    device->viewport.maxZ = viewport->maxZ;
}

static void ProcSetViewport(const RenderCommand& cmd)
{
    const auto& args = cmd.setViewport;

    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.x, args.x);
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.y, args.y);
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.width, args.width);
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.height, args.height);
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.minDepth, args.minDepth);
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.maxDepth, args.maxDepth);
    
    uint32_t specConstants = g_pipelineState.specConstants;
    if (args.minDepth > args.maxDepth)
        specConstants |= SPEC_CONSTANT_REVERSE_Z;
    else 
        specConstants &= ~SPEC_CONSTANT_REVERSE_Z;

    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants, specConstants);

    g_dirtyStates.scissorRect |= g_dirtyStates.viewport;
}

static void SetTexture(GuestDevice* device, uint32_t index, GuestTexture* texture) 
{
    auto isPlayStation = Config::ControllerIcons == EControllerIcons::PlayStation;

    if (Config::ControllerIcons == EControllerIcons::Auto)
        isPlayStation = hid::g_inputDeviceController == hid::EInputDevice::PlayStation;

    if (isPlayStation && texture != nullptr && texture->patchedTexture != nullptr)
        texture = texture->patchedTexture.get();

    RenderCommand cmd;
    cmd.type = RenderCommandType::SetTexture;
    cmd.setTexture.index = index;
    cmd.setTexture.texture = texture;
    g_renderQueue.enqueue(cmd);
}

static void SetTextureInRenderThread(uint32_t index, GuestTexture* texture)
{
    AddBarrier(texture, RenderTextureLayout::SHADER_READ);

    auto viewDimension = texture != nullptr ? texture->viewDimension : RenderTextureViewDimension::UNKNOWN;

    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.texture2DIndices[index],
        viewDimension == RenderTextureViewDimension::TEXTURE_2D ? texture->descriptorIndex : TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D);

    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.texture3DIndices[index], texture != nullptr &&
        viewDimension == RenderTextureViewDimension::TEXTURE_3D ? texture->descriptorIndex : TEXTURE_DESCRIPTOR_NULL_TEXTURE_3D);

    // Check if there's a cubemap texture we recreated and assign it if it's valid. The shader will pick whichever is correct.
    if (viewDimension == RenderTextureViewDimension::TEXTURE_2D && texture->recreatedCubeMapTexture != nullptr)
    {
        texture = texture->recreatedCubeMapTexture.get();
        AddBarrier(texture, RenderTextureLayout::SHADER_READ);
        viewDimension = texture->viewDimension;
    }

    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.textureCubeIndices[index], texture != nullptr &&
        viewDimension == RenderTextureViewDimension::TEXTURE_CUBE ? texture->descriptorIndex : TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE);
}

static void SetSurface(uint32_t index, GuestSurface* surface)
{
    AddBarrier(surface, RenderTextureLayout::SHADER_READ);

    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.texture2DIndices[index], surface->descriptorIndex);
    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.texture3DIndices[index], uint32_t(TEXTURE_DESCRIPTOR_NULL_TEXTURE_3D));
    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.textureCubeIndices[index], uint32_t(TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE));
}

static void ProcSetTexture(const RenderCommand& cmd)
{
    const auto& args = cmd.setTexture;

    // If a pending copy operation is detected, set the source surface. The indices will be fixed later if flushing is necessary.
    bool shouldSetTexture = true;
    if (args.texture != nullptr && args.texture->sourceSurface != nullptr)
    {
        // MSAA surfaces need to be resolved and cannot be used directly.
        if (args.texture->sourceSurface->sampleCount != RenderSampleCount::COUNT_1)
        {
            g_pendingMsaaResolves.emplace(args.texture->sourceSurface);
        }
        else
        {
            SetSurface(args.index, args.texture->sourceSurface);
            shouldSetTexture = false;
        }
    }
    
    if (shouldSetTexture)
        SetTextureInRenderThread(args.index, args.texture);
    
    g_textures[args.index] = args.texture;
}

static void SetScissorRect(GuestDevice* device, GuestRect* rect)
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetScissorRect;
    cmd.setScissorRect.top = rect->top;
    cmd.setScissorRect.left = rect->left;
    cmd.setScissorRect.bottom = rect->bottom;
    cmd.setScissorRect.right = rect->right;
    g_renderQueue.enqueue(cmd);
}

static void ProcSetScissorRect(const RenderCommand& cmd)
{
    const auto& args = cmd.setScissorRect;

    SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.top, args.top);
    SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.left, args.left);
    SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.bottom, args.bottom);
    SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.right, args.right);
}

static uint32_t GetShaderDxilOffset(const ShaderCacheEntry* entry)
{
    return g_vrSinglePass ? entry->stereoDxilOffset : entry->dxilOffset;
}

static uint32_t GetShaderDxilSize(const ShaderCacheEntry* entry)
{
    return g_vrSinglePass ? entry->stereoDxilSize : entry->dxilSize;
}

static uint32_t GetShaderSpirvOffset(const ShaderCacheEntry* entry)
{
    return g_vrSinglePass ? entry->stereoSpirvOffset : entry->spirvOffset;
}

static uint32_t GetShaderSpirvSize(const ShaderCacheEntry* entry)
{
    return g_vrSinglePass ? entry->stereoSpirvSize : entry->spirvSize;
}

static RenderShader* GetOrLinkShader(GuestShader* guestShader, uint32_t specConstants)
{
    if (g_vulkan ||
        guestShader->shaderCacheEntry == nullptr || 
        guestShader->shaderCacheEntry->specConstantsMask == 0)
    {
        std::lock_guard lock(guestShader->mutex);

        if (guestShader->shader == nullptr)
        {
            assert(guestShader->shaderCacheEntry != nullptr);

            if (g_vulkan)
            {
                auto compressedSpirvData = g_shaderCache.get() + GetShaderSpirvOffset(guestShader->shaderCacheEntry);

                std::vector<uint8_t> decoded(smolv::GetDecodedBufferSize(compressedSpirvData, GetShaderSpirvSize(guestShader->shaderCacheEntry)));
                bool result = smolv::Decode(compressedSpirvData, GetShaderSpirvSize(guestShader->shaderCacheEntry), decoded.data(), decoded.size());
                assert(result);

                guestShader->shader = g_device->createShader(decoded.data(), decoded.size(), "main", RenderShaderFormat::SPIRV);
            }
            else
            {
                guestShader->shader = g_device->createShader(g_shaderCache.get() + GetShaderDxilOffset(guestShader->shaderCacheEntry), 
                    GetShaderDxilSize(guestShader->shaderCacheEntry), "main", RenderShaderFormat::DXIL);
            }
        }

        return guestShader->shader.get();
    }

    specConstants &= guestShader->shaderCacheEntry->specConstantsMask;

    RenderShader* shader;
    {
        std::lock_guard lock(guestShader->mutex);
        shader = guestShader->linkedShaders[specConstants].get();
    }

#ifdef UNLEASHED_RECOMP_D3D12
    if (shader == nullptr)
    {
        static Mutex g_compiledSpecConstantLibraryBlobMutex;
        static ankerl::unordered_dense::map<uint32_t, ComPtr<IDxcBlob>> g_compiledSpecConstantLibraryBlobs;

        thread_local ComPtr<IDxcCompiler3> s_dxcCompiler;
        thread_local ComPtr<IDxcLinker> s_dxcLinker;
        thread_local ComPtr<IDxcUtils> s_dxcUtils;

        wchar_t specConstantsLibName[0x100];
        swprintf_s(specConstantsLibName, L"SpecConstants_%d", specConstants);

        ComPtr<IDxcBlob> specConstantLibraryBlob;
        {
            std::lock_guard lock(g_compiledSpecConstantLibraryBlobMutex);
            specConstantLibraryBlob = g_compiledSpecConstantLibraryBlobs[specConstants];
        }

        if (specConstantLibraryBlob == nullptr)
        {
            if (s_dxcCompiler == nullptr)
            {
                HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(s_dxcCompiler.GetAddressOf()));
                assert(SUCCEEDED(hr) && s_dxcCompiler != nullptr);
            }

            char libraryHlsl[0x100];
            sprintf_s(libraryHlsl, "export uint g_SpecConstants() { return %d; }", specConstants);

            DxcBuffer buffer{};
            buffer.Ptr = libraryHlsl;
            buffer.Size = strlen(libraryHlsl);

            const wchar_t* args[1];
            args[0] = L"-T lib_6_3";

            ComPtr<IDxcResult> result;
            HRESULT hr = s_dxcCompiler->Compile(&buffer, args, std::size(args), nullptr, IID_PPV_ARGS(result.GetAddressOf()));
            assert(SUCCEEDED(hr) && result != nullptr);

            hr = result->GetResult(specConstantLibraryBlob.GetAddressOf());
            assert(SUCCEEDED(hr) && specConstantLibraryBlob != nullptr);

            std::lock_guard lock(g_compiledSpecConstantLibraryBlobMutex);
            g_compiledSpecConstantLibraryBlobs.emplace(specConstants, specConstantLibraryBlob);
        }

        if (s_dxcLinker == nullptr)
        {
            HRESULT hr = DxcCreateInstance(CLSID_DxcLinker, IID_PPV_ARGS(s_dxcLinker.GetAddressOf()));
            assert(SUCCEEDED(hr) && s_dxcLinker != nullptr);
        }

        s_dxcLinker->RegisterLibrary(specConstantsLibName, specConstantLibraryBlob.Get());

        wchar_t shaderLibName[0x100];
        swprintf_s(shaderLibName, L"Shader_%d", GetShaderDxilOffset(guestShader->shaderCacheEntry));

        ComPtr<IDxcBlobEncoding> shaderLibraryBlob;
        {
            std::lock_guard lock(guestShader->mutex);
            shaderLibraryBlob = guestShader->libraryBlob;
        }

        if (shaderLibraryBlob == nullptr)
        {
            if (s_dxcUtils == nullptr)
            {
                HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(s_dxcUtils.GetAddressOf()));
                assert(SUCCEEDED(hr) && s_dxcUtils != nullptr);
            }

            HRESULT hr = s_dxcUtils->CreateBlobFromPinned(
                g_shaderCache.get() + GetShaderDxilOffset(guestShader->shaderCacheEntry),
                GetShaderDxilSize(guestShader->shaderCacheEntry),
                DXC_CP_ACP,
                shaderLibraryBlob.GetAddressOf());

            assert(SUCCEEDED(hr) && shaderLibraryBlob != nullptr);

            std::lock_guard lock(guestShader->mutex);
            guestShader->libraryBlob = shaderLibraryBlob;
        }

        s_dxcLinker->RegisterLibrary(shaderLibName, shaderLibraryBlob.Get());

        const wchar_t* libraryNames[] = { specConstantsLibName, shaderLibName };

        ComPtr<IDxcOperationResult> result;
        HRESULT hr = s_dxcLinker->Link(L"main", guestShader->type == ResourceType::VertexShader ? L"vs_6_0" : L"ps_6_0",
            libraryNames, std::size(libraryNames), nullptr, 0, result.GetAddressOf());

        assert(SUCCEEDED(hr) && result != nullptr);

        ComPtr<IDxcBlob> blob;
        hr = result->GetResult(blob.GetAddressOf());
        assert(SUCCEEDED(hr) && blob != nullptr);

        {
            std::lock_guard lock(guestShader->mutex);

            auto& linkedShader = guestShader->linkedShaders[specConstants];
            if (linkedShader == nullptr)
            {
                linkedShader = g_device->createShader(blob->GetBufferPointer(), blob->GetBufferSize(), "main", RenderShaderFormat::DXIL);
                guestShader->shaderBlobs.push_back(std::move(blob));
            }

            shader = linkedShader.get();
        }        
    }
#endif

    return shader;
}

static void SanitizePipelineState(PipelineState& pipelineState)
{
    if (!pipelineState.zEnable)
    {
        pipelineState.zWriteEnable = false;
        pipelineState.zFunc = RenderComparisonFunction::LESS;
        pipelineState.slopeScaledDepthBias = 0.0f;
        pipelineState.depthBias = 0;
        pipelineState.depthStencilFormat = RenderFormat::UNKNOWN;
    }

    if (pipelineState.slopeScaledDepthBias == 0.0f)
        pipelineState.slopeScaledDepthBias = 0.0f; // Remove sign.

    if (!pipelineState.colorWriteEnable)
    {
        pipelineState.alphaBlendEnable = false;
        pipelineState.renderTargetFormat = RenderFormat::UNKNOWN;
    }

    if (!pipelineState.alphaBlendEnable)
    {
        pipelineState.srcBlend = RenderBlend::ONE;
        pipelineState.destBlend = RenderBlend::ZERO;
        pipelineState.blendOp = RenderBlendOperation::ADD;
        pipelineState.srcBlendAlpha = RenderBlend::ONE;
        pipelineState.destBlendAlpha = RenderBlend::ZERO;
        pipelineState.blendOpAlpha = RenderBlendOperation::ADD;
    }

    for (size_t i = 0; i < 16; i++)
    {
        if (!pipelineState.vertexDeclaration->vertexStreams[i])
            pipelineState.vertexStrides[i] = 0;
    }

    uint32_t specConstantsMask = 0;
    if (pipelineState.vertexShader->shaderCacheEntry != nullptr)
        specConstantsMask |= pipelineState.vertexShader->shaderCacheEntry->specConstantsMask;

    if (pipelineState.pixelShader != nullptr && pipelineState.pixelShader->shaderCacheEntry != nullptr)
        specConstantsMask |= pipelineState.pixelShader->shaderCacheEntry->specConstantsMask;

    pipelineState.specConstants &= specConstantsMask;
}

static std::unique_ptr<RenderPipeline> CreateGraphicsPipeline(const PipelineState& pipelineState)
{
#ifdef ASYNC_PSO_DEBUG
    ++g_pipelinesCurrentlyCompiling;
#endif

    RenderGraphicsPipelineDesc desc;
    desc.pipelineLayout = g_pipelineLayout.get();
    desc.vertexShader = GetOrLinkShader(pipelineState.vertexShader, pipelineState.specConstants);
    desc.pixelShader = pipelineState.pixelShader != nullptr ? GetOrLinkShader(pipelineState.pixelShader, pipelineState.specConstants) : nullptr;
    desc.depthFunction = pipelineState.zFunc;
    desc.depthEnabled = pipelineState.zEnable;
    desc.depthWriteEnabled = pipelineState.zWriteEnable;
    desc.depthBias = pipelineState.depthBias;
    desc.slopeScaledDepthBias = pipelineState.slopeScaledDepthBias;
    desc.dynamicDepthBiasEnabled = g_capabilities.dynamicDepthBias;
    desc.depthClipEnabled = true;
    desc.primitiveTopology = pipelineState.primitiveTopology;
    desc.cullMode = pipelineState.cullMode;
    desc.renderTargetFormat[0] = pipelineState.renderTargetFormat;
    desc.renderTargetBlend[0].blendEnabled = pipelineState.alphaBlendEnable;
    desc.renderTargetBlend[0].srcBlend = pipelineState.srcBlend;
    desc.renderTargetBlend[0].dstBlend = pipelineState.destBlend;
    desc.renderTargetBlend[0].blendOp = pipelineState.blendOp;
    desc.renderTargetBlend[0].srcBlendAlpha = pipelineState.srcBlendAlpha;
    desc.renderTargetBlend[0].dstBlendAlpha = pipelineState.destBlendAlpha;
    desc.renderTargetBlend[0].blendOpAlpha = pipelineState.blendOpAlpha;
    desc.renderTargetBlend[0].renderTargetWriteMask = pipelineState.colorWriteEnable;
    desc.renderTargetCount = pipelineState.renderTargetFormat != RenderFormat::UNKNOWN ? 1 : 0;
    desc.depthTargetFormat = pipelineState.depthStencilFormat;
    desc.multisampling.sampleCount = pipelineState.sampleCount;
    desc.alphaToCoverageEnabled = pipelineState.enableAlphaToCoverage;
    desc.inputElements = pipelineState.vertexDeclaration->inputElements.get();
    desc.inputElementsCount = pipelineState.vertexDeclaration->inputElementCount;
    
    RenderSpecConstant specConstant{};
    specConstant.value = pipelineState.specConstants;
    
    if (pipelineState.specConstants != 0)
    {
        desc.specConstants = &specConstant;
        desc.specConstantsCount = 1;
    }
    
    RenderInputSlot inputSlots[16]{};
    uint32_t inputSlotIndices[16]{};
    uint32_t inputSlotCount = 0;
    
    for (size_t i = 0; i < pipelineState.vertexDeclaration->inputElementCount; i++)
    {
        auto& inputElement = pipelineState.vertexDeclaration->inputElements[i];
        auto& inputSlotIndex = inputSlotIndices[inputElement.slotIndex];
    
        if (inputSlotIndex == NULL)
            inputSlotIndex = ++inputSlotCount;
    
        auto& inputSlot = inputSlots[inputSlotIndex - 1];
        inputSlot.index = inputElement.slotIndex;
        inputSlot.stride = pipelineState.vertexStrides[inputElement.slotIndex];
    
        if (pipelineState.instancing && inputElement.slotIndex != 0 && inputElement.slotIndex != 15)
            inputSlot.classification = RenderInputSlotClassification::PER_INSTANCE_DATA;
        else
            inputSlot.classification = RenderInputSlotClassification::PER_VERTEX_DATA;
    }
    
    desc.inputSlots = inputSlots;
    desc.inputSlotsCount = inputSlotCount;
    
    desc.viewportCount = GetVRViewportCount();
    auto pipeline = g_device->createGraphicsPipeline(desc);

#ifdef ASYNC_PSO_DEBUG
    --g_pipelinesCurrentlyCompiling;
#endif

    return pipeline;
}

static RenderPipeline* CreateGraphicsPipelineInRenderThread(PipelineState pipelineState)
{
    SanitizePipelineState(pipelineState);

    XXH64_hash_t hash = XXH3_64bits(&pipelineState, sizeof(pipelineState));
    auto& pipeline = g_pipelines[hash];
    if (pipeline == nullptr)
    {
        auto start = std::chrono::steady_clock::now();
        pipeline = CreateGraphicsPipeline(pipelineState);

        ++g_vrStatistics.renderThreadPipelines;
        g_vrStatistics.renderThreadPipelineMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

#ifdef ASYNC_PSO_DEBUG
        bool loading = *SWA::SGlobals::ms_IsLoading;

        if (loading)
            ++g_pipelinesCreatedAsynchronously;
        else
            ++g_pipelinesCreatedInRenderThread;

        pipeline->setName(fmt::format("{} {} {} {:X}", loading ? "ASYNC" : "",
            pipelineState.vertexShader->name, pipelineState.pixelShader != nullptr ? pipelineState.pixelShader->name : "<none>", hash));
        
        if (!loading)
        {
            std::lock_guard lock(g_debugMutex);
            g_pipelineDebugText = fmt::format(
                "PipelineState {:X}:\n"
                "  vertexShader: {}\n"
                "  pixelShader: {}\n"
                "  vertexDeclaration: {:X}\n"
                "  instancing: {}\n"
                "  zEnable: {}\n"
                "  zWriteEnable: {}\n"
                "  srcBlend: {}\n"
                "  destBlend: {}\n"
                "  cullMode: {}\n"
                "  zFunc: {}\n"
                "  alphaBlendEnable: {}\n"
                "  blendOp: {}\n"
                "  slopeScaledDepthBias: {}\n"
                "  depthBias: {}\n"
                "  srcBlendAlpha: {}\n"
                "  destBlendAlpha: {}\n"
                "  blendOpAlpha: {}\n"
                "  colorWriteEnable: {:X}\n"
                "  primitiveTopology: {}\n"
                "  vertexStrides[0]: {}\n"
                "  vertexStrides[1]: {}\n"
                "  vertexStrides[2]: {}\n"
                "  vertexStrides[3]: {}\n"
                "  renderTargetFormat: {}\n"
                "  depthStencilFormat: {}\n"
                "  sampleCount: {}\n"
                "  enableAlphaToCoverage: {}\n"
                "  specConstants: {:X}\n",
                hash,
                pipelineState.vertexShader->name,
                pipelineState.pixelShader != nullptr ? pipelineState.pixelShader->name : "<none>",
                reinterpret_cast<size_t>(pipelineState.vertexDeclaration),
                pipelineState.instancing,
                pipelineState.zEnable,
                pipelineState.zWriteEnable,
                magic_enum::enum_name(pipelineState.srcBlend),
                magic_enum::enum_name(pipelineState.destBlend),
                magic_enum::enum_name(pipelineState.cullMode),
                magic_enum::enum_name(pipelineState.zFunc),
                pipelineState.alphaBlendEnable,
                magic_enum::enum_name(pipelineState.blendOp),
                pipelineState.slopeScaledDepthBias,
                pipelineState.depthBias,
                magic_enum::enum_name(pipelineState.srcBlendAlpha),
                magic_enum::enum_name(pipelineState.destBlendAlpha),
                magic_enum::enum_name(pipelineState.blendOpAlpha),
                pipelineState.colorWriteEnable,
                magic_enum::enum_name(pipelineState.primitiveTopology),
                pipelineState.vertexStrides[0],
                pipelineState.vertexStrides[1],
                pipelineState.vertexStrides[2],
                pipelineState.vertexStrides[3],
                magic_enum::enum_name(pipelineState.renderTargetFormat),
                magic_enum::enum_name(pipelineState.depthStencilFormat),
                pipelineState.sampleCount,
                pipelineState.enableAlphaToCoverage,
                pipelineState.specConstants)
                + g_pipelineDebugText;
        }
#endif

#ifdef PSO_CACHING
        std::lock_guard lock(g_pipelineCacheMutex);
        g_pipelineStatesToCache.emplace(hash, pipelineState);
#endif
    }
    
    return pipeline.get();
}

static RenderTextureAddressMode ConvertTextureAddressMode(size_t value)
{
    switch (value)
    {
    case D3DTADDRESS_WRAP:
        return RenderTextureAddressMode::WRAP;
    case D3DTADDRESS_MIRROR:
        return RenderTextureAddressMode::MIRROR;
    case D3DTADDRESS_CLAMP:
        return RenderTextureAddressMode::CLAMP;
    case D3DTADDRESS_MIRRORONCE:
        return RenderTextureAddressMode::MIRROR_ONCE;
    case D3DTADDRESS_BORDER:
        return RenderTextureAddressMode::BORDER;
    default:
        assert(false && "Unknown texture address mode");
        return RenderTextureAddressMode::UNKNOWN;
    }
}

static RenderFilter ConvertTextureFilter(uint32_t value)
{
    switch (value)
    {
    case D3DTEXF_POINT:
    case D3DTEXF_NONE:
        return RenderFilter::NEAREST;
    case D3DTEXF_LINEAR:
        return RenderFilter::LINEAR;
    default:
        assert(false && "Unknown texture filter");
        return RenderFilter::UNKNOWN;
    }
}

static RenderBorderColor ConvertBorderColor(uint32_t value)
{
    switch (value)
    {
    case 0:
        return RenderBorderColor::TRANSPARENT_BLACK;
    case 1:
        return RenderBorderColor::OPAQUE_WHITE;
    default:
        assert(false && "Unknown border color");
        return RenderBorderColor::UNKNOWN;
    }
}

struct LocalRenderCommandQueue
{
    RenderCommand commands[20];
    uint32_t count = 0;

    RenderCommand& enqueue()
    {
        assert(count < std::size(commands));
        return commands[count++];
    }

    void submit()
    {
        g_renderQueue.enqueue_bulk(commands, count);
    }
};

static void FlushRenderStateForMainThread(GuestDevice* device, LocalRenderCommandQueue& queue)
{
    constexpr size_t BOOL_MASK = 0x100000000000000ull;
    if ((device->dirtyFlags[4].get() & BOOL_MASK) != 0)
    {
        auto& cmd = queue.enqueue();
        cmd.type = RenderCommandType::SetBooleans;
        cmd.setBooleans.booleans = (device->vertexShaderBoolConstants[0].get() & 0xFF) | ((device->pixelShaderBoolConstants[0].get() & 0xFF) << 16);

        device->dirtyFlags[4] = device->dirtyFlags[4].get() & ~BOOL_MASK;
    }

    for (uint32_t i = 0; i < 16; i++)
    {
        const size_t mask = 0x8000000000000000ull >> (i + 32);
        if (device->dirtyFlags[3].get() & mask)
        {
            auto& cmd = queue.enqueue();
            cmd.type = RenderCommandType::SetSamplerState;
            cmd.setSamplerState.index = i;
            cmd.setSamplerState.data0 = device->samplerStates[i].data[0];
            cmd.setSamplerState.data3 = device->samplerStates[i].data[3];
            cmd.setSamplerState.data5 = device->samplerStates[i].data[5];

            device->dirtyFlags[3] = device->dirtyFlags[3].get() & ~mask;
        }
    }

    uint64_t dirtyFlags = device->dirtyFlags[0].get();
    if (dirtyFlags != 0)
    {
        int startRegister = std::countl_zero(dirtyFlags);
        int endRegister = 64 - std::countr_zero(dirtyFlags);

        uint32_t index = startRegister * 16;
        uint32_t size = (endRegister - startRegister) * 64;

        auto& cmd = queue.enqueue();
        cmd.type = RenderCommandType::SetVertexShaderConstants;
        cmd.setVertexShaderConstants.memory = g_intermediaryUploadAllocator.allocate(&device->vertexShaderFloatConstants[index], size);
        cmd.setVertexShaderConstants.index = index;
        cmd.setVertexShaderConstants.size = size;

        device->dirtyFlags[0] = 0;
    }

    dirtyFlags = device->dirtyFlags[1].get();
    if (dirtyFlags != 0)
    {
        int startRegister = std::countl_zero(dirtyFlags);
        int endRegister = std::min(56, 64 - std::countr_zero(dirtyFlags));

        uint32_t index = startRegister * 16;
        uint32_t size = (endRegister - startRegister) * 64;

        auto& cmd = queue.enqueue();
        cmd.type = RenderCommandType::SetPixelShaderConstants;
        cmd.setPixelShaderConstants.memory = g_intermediaryUploadAllocator.allocate(&device->pixelShaderFloatConstants[index], size);
        cmd.setPixelShaderConstants.index = index;
        cmd.setPixelShaderConstants.size = size;

        device->dirtyFlags[1] = 0;
    }
}

static void ProcSetBooleans(const RenderCommand& cmd)
{
    SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.booleans, cmd.setBooleans.booleans);
}

static void ProcSetSamplerState(const RenderCommand& cmd)
{
    const auto& args = cmd.setSamplerState;

    const auto addressU = ConvertTextureAddressMode((args.data0 >> 10) & 0x7);
    const auto addressV = ConvertTextureAddressMode((args.data0 >> 13) & 0x7);
    const auto addressW = ConvertTextureAddressMode((args.data0 >> 16) & 0x7);
    auto magFilter = ConvertTextureFilter((args.data3 >> 19) & 0x3);
    auto minFilter = ConvertTextureFilter((args.data3 >> 21) & 0x3);
    auto mipFilter = ConvertTextureFilter((args.data3 >> 23) & 0x3);
    const auto borderColor = ConvertBorderColor(args.data5 & 0x3);

    bool anisotropyEnabled = Config::AnisotropicFiltering > 0 && mipFilter == RenderFilter::LINEAR;
    if (anisotropyEnabled)
    {
        magFilter = RenderFilter::LINEAR;
        minFilter = RenderFilter::LINEAR;
    }

    auto& samplerDesc = g_samplerDescs[args.index];

    bool dirty = false;

    SetDirtyValue(dirty, samplerDesc.addressU, addressU);
    SetDirtyValue(dirty, samplerDesc.addressV, addressV);
    SetDirtyValue(dirty, samplerDesc.addressW, addressW);
    SetDirtyValue(dirty, samplerDesc.minFilter, minFilter);
    SetDirtyValue(dirty, samplerDesc.magFilter, magFilter);
    SetDirtyValue(dirty, samplerDesc.mipmapMode, RenderMipmapMode(mipFilter));
    SetDirtyValue(dirty, samplerDesc.maxAnisotropy, anisotropyEnabled ? Config::AnisotropicFiltering : 16u);
    SetDirtyValue(dirty, samplerDesc.anisotropyEnabled, anisotropyEnabled);
    SetDirtyValue(dirty, samplerDesc.borderColor, borderColor);

    if (dirty)
    {
        auto& [descriptorIndex, sampler] = g_samplerStates[XXH3_64bits(&samplerDesc, sizeof(RenderSamplerDesc))];
        if (descriptorIndex == NULL)
        {
            descriptorIndex = g_samplerStates.size();
            sampler = g_device->createSampler(samplerDesc);

            g_samplerDescriptorSet->setSampler(descriptorIndex - 1, sampler.get());
        }

        SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.samplerIndices[args.index], descriptorIndex - 1);
    }
}

static void ProcSetVertexShaderConstants(const RenderCommand& cmd)
{
    auto& args = cmd.setVertexShaderConstants;
    assert((args.index * sizeof(uint32_t) + args.size) <= sizeof(g_vertexShaderConstants));

    memcpy(&g_vertexShaderConstants[args.index], args.memory, args.size);
    g_dirtyStates.vertexShaderConstants = true;
}

static void ProcSetPixelShaderConstants(const RenderCommand& cmd)
{
    auto& args = cmd.setPixelShaderConstants;
    assert((args.index * sizeof(uint32_t) + args.size) <= sizeof(g_pixelShaderConstants));

    memcpy(&g_pixelShaderConstants[args.index], args.memory, args.size);
    g_dirtyStates.pixelShaderConstants = true;
}

static void ProcAddPipeline(const RenderCommand& cmd)
{
    auto& args = cmd.addPipeline;
    auto& pipeline = g_pipelines[args.hash];

    if (pipeline == nullptr)
    {
        pipeline = std::unique_ptr<RenderPipeline>(args.pipeline);
#ifdef ASYNC_PSO_DEBUG
        ++g_pipelinesCreatedAsynchronously;
#endif
    }
    else
    {
#ifdef ASYNC_PSO_DEBUG
        ++g_pipelinesDropped;
#endif
        delete args.pipeline;
    }
}

static constexpr int32_t COMMON_DEPTH_BIAS_VALUE = int32_t((1 << 24) * 0.002f);
static constexpr float COMMON_SLOPE_SCALED_DEPTH_BIAS_VALUE = 1.0f;

static void NoteVRSurfaceDraw(GuestSurface* renderTarget, GuestSurface* depthStencil, VRDrawKind kind)
{
    GuestSurface* target = renderTarget != nullptr ? renderTarget : depthStencil;
    if (target == nullptr)
        return;

    for (GuestSurface* surface : { renderTarget, depthStencil })
    {
        if (surface == nullptr)
            continue;

        VRSurfaceUse& use = NoteVRSurface(surface);

        if (kind == VRDrawKind::Camera)
            ++use.cameraDraws;
        else if (kind == VRDrawKind::Reflection)
            ++use.reflectionDraws;
        else
            ++use.otherDraws;
    }

    VRSurfaceUse& targetUse = g_vrSurfaceUses[target];
    GuestShader* pixelShader = g_pipelineState.pixelShader;
    uint32_t samplerMask = pixelShader != nullptr ? pixelShader->vrSamplerMask : 0;

    for (uint32_t i = 0; i < std::size(g_textures); i++)
    {
        const GuestTexture* texture = g_textures[i];
        if ((samplerMask & (1u << i)) == 0 || texture == nullptr || texture->vrResolvedFrom == nullptr)
            continue;

        const GuestSurface* source = texture->vrResolvedFrom;

        // Only counted for a surface already described: one known by a stale
        // pointer alone is never added.
        auto findResult = g_vrSurfaceUses.find(source);
        if (findResult != g_vrSurfaceUses.end())
            ++findResult->second.reads;

        auto sourcesEnd = targetUse.sources.begin() + targetUse.sourceCount;
        if (std::find(targetUse.sources.begin(), sourcesEnd, source) == sourcesEnd && targetUse.sourceCount < targetUse.sources.size())
            targetUse.sources[targetUse.sourceCount++] = source;
    }
}

// VR: starts a GPU segment (see GpuSegmentKind) when the kind of work changes.
static void BeginGpuSegment(GpuSegmentKind kind)
{
    uint32_t& count = g_gpuSegmentCounts[g_frame];
    if (int32_t(kind) == g_gpuSegmentKind || count == GPU_SEGMENTS_UNUSED || count >= NUM_SEGMENT_QUERIES)
        return;

    g_commandLists[g_frame]->writeTimestamp(g_segmentQueryPools[g_frame].get(), count);
    g_gpuSegmentKinds[g_frame][count++] = kind;
    g_gpuSegmentKind = kind;
}

// VR single-pass stereo: how a draw into these targets reaches the eyes. In a
// headset frame, targets with a layer per eye are drawn for both.
static VRStereoDraw GetVRStereoDraw(GuestSurface* renderTarget, GuestSurface* depthStencil)
{
    if (!g_vrSinglePass || g_vrPass < 0 || (renderTarget == nullptr && depthStencil == nullptr))
        return VRStereoDraw::LeftEye;

    VRStereoDraw stereoDraw = g_pipelineState.instancing ? VRStereoDraw::EachEye : VRStereoDraw::BothEyes;
    uint32_t layerCount = 2;

    for (const GuestSurface* surface : { renderTarget, depthStencil })
    {
        if (surface != nullptr)
            layerCount = std::min(layerCount, surface->layerCount);
    }

    if (layerCount < 2)
    {
        stereoDraw = VRStereoDraw::LeftEye;

        if (renderTarget != nullptr && depthStencil != nullptr && renderTarget->layerCount != depthStencil->layerCount)
            ++g_vrStatistics.layerMismatchDraws;
    }

    switch (stereoDraw)
    {
    case VRStereoDraw::BothEyes:
        ++g_vrStatistics.bothEyesDraws;
        break;
    case VRStereoDraw::EachEye:
        ++g_vrStatistics.eachEyeDraws;
        break;
    default:
        ++g_vrStatistics.leftEyeDraws;
        break;
    }

    return stereoDraw;
}

// Uploads the shared constants (the bloom feedback pass reading this frame's
// bright pass for its history, see ReplaceVRBloomFeedbackTexture).
static void UploadSharedConstants()
{
    auto sharedConstants = g_uploadAllocators[g_frame].allocate<false>(&g_sharedConstants, sizeof(g_sharedConstants), 0x100);

    if (g_vrSharedPatch)
        ReplaceVRBloomFeedbackTexture(reinterpret_cast<SharedConstants*>(sharedConstants.memory));

    SetRootDescriptor(sharedConstants, 2);
}

// Uploads shader constants (byte-swapped), once or, for constants rewritten per
// eye in single-pass stereo, once per eye: the registers of the mask, the others
// left as they are (the shader does not read them). Upload memory is only
// written: reading it back is very slow (write-combined, uncached).
template<size_t N>
static UploadAllocation UploadShaderConstants(const uint32_t (&constants)[N], uint32_t blockCount, const uint64_t* registerMask)
{
    constexpr size_t RegisterCount = N / 4;

    auto allocation = g_uploadAllocators[g_frame].allocate(sizeof(constants) * blockCount, 0x100);
    auto blockMemory = reinterpret_cast<uint32_t*>(allocation.memory);

    for (uint32_t block = 0; block < blockCount; block++, blockMemory += N)
    {
        size_t first = 0;

        while (first < RegisterCount)
        {
            uint64_t bits = registerMask[first / 64] >> (first % 64);
            if (bits == 0)
            {
                first = (first / 64 + 1) * 64;
                continue;
            }

            first += std::countr_zero(bits);
            if (first >= RegisterCount)
                break;

            size_t last = std::min(RegisterCount, first + std::countr_one(registerMask[first / 64] >> (first % 64)));

            for (size_t i = first * 4; i < last * 4; i++)
                blockMemory[i] = ByteSwap(constants[i]);

            first = last;
        }
    }

    return allocation;
}

// Issues the draw just flushed for the eyes it reaches (g_vrStereoDraw): `draw`
// takes the factor its instance count is multiplied by. Drawn once per eye, it
// switches the eye mode to the right eye in between.
template<typename T>
static void DrawForVREyes(const T& draw)
{
    switch (g_vrStereoDraw)
    {
    case VRStereoDraw::BothEyes:
        draw(2u);
        break;

    case VRStereoDraw::EachEye:
        draw(1u);
        g_sharedConstants.vrEyeMode = (g_sharedConstants.vrEyeMode & ~VR_EYE_MODE_MASK) | VR_EYE_MODE_RIGHT;
        UploadSharedConstants();
        draw(1u);
        break;

    default:
        draw(1u);
        break;
    }
}

static void FlushRenderStateForRenderThread()
{
    // VR: only a draw just put on the HUD panel has its scissor moved there.
    bool vrDrawOnPanel = g_vrNextDrawOnPanel;
    g_vrNextDrawOnPanel = false;

    if (vrDrawOnPanel != g_vrScissorOnPanel)
    {
        g_vrScissorOnPanel = vrDrawOnPanel;
        g_dirtyStates.scissorRect = true;
    }

    auto renderTarget = g_pipelineState.colorWriteEnable ? g_renderTarget : nullptr;
    auto depthStencil = g_pipelineState.zEnable ? g_depthStencil : nullptr;

    g_vrStereoDraw = GetVRStereoDraw(renderTarget, depthStencil);

    if (g_vrPass >= 0)
    {
        const GuestSurface* target = renderTarget != nullptr ? renderTarget : depthStencil;

        if (target != nullptr && target->width == target->height)
            BeginGpuSegment(GPU_SEGMENT_SHADOWS);
        else
            BeginGpuSegment(depthStencil != nullptr ? GPU_SEGMENT_SCENE : GPU_SEGMENT_FLAT);
    }

    bool foundAny = PopulateBarriersForStretchRect(renderTarget, depthStencil);

    for (const auto surface : g_pendingMsaaResolves)
    {
        bool isDepthStencil = (surface->format == RenderFormat::D32_FLOAT);
        foundAny |= PopulateBarriersForStretchRect(isDepthStencil ? nullptr : surface, isDepthStencil ? surface : nullptr);
    }

    if (foundAny)
    {
        FlushBarriers();
        ExecutePendingStretchRectCommands(renderTarget, depthStencil);

        for (const auto surface : g_pendingMsaaResolves)
        {
            bool isDepthStencil = (surface->format == RenderFormat::D32_FLOAT);
            ExecutePendingStretchRectCommands(isDepthStencil ? nullptr : surface, isDepthStencil ? surface : nullptr);
        }
    }

    if (!g_pendingMsaaResolves.empty())
        g_pendingMsaaResolves.clear();

    AddBarrier(renderTarget, RenderTextureLayout::COLOR_WRITE);
    AddBarrier(depthStencil, RenderTextureLayout::DEPTH_WRITE);

    FlushBarriers();

    SetFramebuffer(renderTarget, depthStencil, false);
    FlushViewport();

    // VR: foveated rendering for draws into the scene's targets (see VRFoveation).
    const RenderTexture* shadingRateImage = nullptr;

    if (g_vrFoveationFrame && g_vrPass >= 0)
    {
        GuestSurface* target = renderTarget != nullptr ? renderTarget : depthStencil;

        if (target != nullptr && target != g_backBuffer &&
            std::abs(int32_t(target->width) - int32_t(g_vrFoveation.sceneWidth)) <= 2 &&
            std::abs(int32_t(target->height) - int32_t(g_vrFoveation.sceneHeight)) <= 2)
        {
            shadingRateImage = g_capabilities.shadingRateAttachment ? g_vrShadingRateAttachment.get() :
                g_vrFoveation.images[g_vrSinglePass ? 2 : g_vrPass].get();
            ++g_vrStatistics.foveatedDraws;
        }
    }

    SetShadingRateImage(shadingRateImage);

    auto& commandList = g_commandLists[g_frame];

    // D3D12 resets depth bias values to the pipeline values, even if they are dynamic.
    // We can reduce unnecessary calls by making common depth bias values part of the pipeline.
    if (g_capabilities.dynamicDepthBias && !g_vulkan)
    {
        bool useDepthBias = (g_depthBias != 0) || (g_slopeScaledDepthBias != 0.0f);

        int32_t depthBias = useDepthBias ? COMMON_DEPTH_BIAS_VALUE : 0;
        float slopeScaledDepthBias = useDepthBias ? COMMON_SLOPE_SCALED_DEPTH_BIAS_VALUE : 0.0f;

        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthBias, depthBias);
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.slopeScaledDepthBias, slopeScaledDepthBias);
    }

    if (g_dirtyStates.pipelineState)
    {
        commandList->setPipeline(CreateGraphicsPipelineInRenderThread(g_pipelineState));

        // D3D12 resets the depth bias values. Check if they need to be set again.
        if (g_capabilities.dynamicDepthBias && !g_vulkan)
            g_dirtyStates.depthBias = (g_depthBias != g_pipelineState.depthBias) || (g_slopeScaledDepthBias != g_pipelineState.slopeScaledDepthBias);
    }

    if (g_dirtyStates.depthBias && g_capabilities.dynamicDepthBias)
        commandList->setDepthBias(g_depthBias, 0.0f, g_slopeScaledDepthBias);

    // VR: draws through the game camera get the current eye's camera, boost and
    // depth of field passes lose their effect, the persistent bloom loses its
    // history, and light shafts head for the light as the eye sees it.
    // Constants uploaded for one kind of draw are not reused for another.
    if (g_vrPass == 0)
    {
        NoteVRReflectionMapRead(renderTarget);

        GuestShader* vertexShader = g_pipelineState.vertexShader;
        if (!g_vrDrawIsUP && renderTarget == g_backBuffer && vertexShader != nullptr &&
            vertexShader->vrProjectionRegister < 0 && !vertexShader->vrHudShader)
        {
            NoteVRViewLocked(g_vrStatistics.viewLockedBuffer2D, "2D from a vertex buffer");
        }
    }

    VRDrawKind vrDrawKind = (g_vrPass >= 0) ? GetVRDrawKind(renderTarget, depthStencil) : VRDrawKind::None;
    bool vrNeutraliseBoost = ShouldNeutraliseVRBoostPass();
    bool vrReplaceBloomFeedback = ShouldReplaceVRBloomFeedback();
    bool vrPatchLightShafts = ShouldPatchVRLightShafts();
    bool vrNeutraliseDepthOfField = ShouldNeutraliseVRDepthOfField();

    if (g_vrPass == 0)
        NoteVRSurfaceDraw(renderTarget, depthStencil, vrDrawKind);

    if (vrReplaceBloomFeedback && g_vrPass == 0)
        NoteVRBloomFeedback(renderTarget);

    if (vrPatchLightShafts && g_vrPass == 0)
        NoteVRLightShafts();
    GuestShader* vrVertexPatchShader = nullptr;
    GuestShader* vrPixelPatchShader = nullptr;

    if (vrDrawKind != VRDrawKind::None)
    {
        if (g_vrPass == 0)
        {
            ++g_vrCameraDrawCount;

            if (depthStencil != nullptr)
                depthStencil->vrPerspectiveDepth = true;
        }

        vrVertexPatchShader = g_pipelineState.vertexShader;

        // A reflection keeps the game's view, which the pixel shader constants follow.
        if (vrDrawKind == VRDrawKind::Camera)
            vrPixelPatchShader = g_pipelineState.pixelShader;
    }

    if (vrNeutraliseBoost || vrReplaceBloomFeedback || vrPatchLightShafts || vrNeutraliseDepthOfField)
        vrPixelPatchShader = g_pipelineState.pixelShader;

    if (vrReplaceBloomFeedback != g_vrSharedPatch)
    {
        g_dirtyStates.sharedConstants = true;
        g_vrSharedPatch = vrReplaceBloomFeedback;
    }

    if (vrVertexPatchShader != g_vrVertexPatchShader || vrDrawKind != g_vrVertexPatchKind)
    {
        g_dirtyStates.vertexShaderConstants = true;
        g_vrVertexPatchShader = vrVertexPatchShader;
        g_vrVertexPatchKind = vrDrawKind;
    }

    if (vrPixelPatchShader != g_vrPixelPatchShader)
    {
        g_dirtyStates.pixelShaderConstants = true;
        g_vrPixelPatchShader = vrPixelPatchShader;
    }

    // Single-pass stereo: constants rewritten for the eye get a block per eye
    // when the draw reaches both; otherwise both eyes read the first block.
    bool vrStereo = g_vrStereoDraw != VRStereoDraw::LeftEye;
    uint32_t vrVertexBlocks = (vrStereo && vrVertexPatchShader != nullptr) ? 2 : 1;
    uint32_t vrPixelBlocks = (vrStereo && vrPixelPatchShader != nullptr) ? 2 : 1;

    if (vrVertexBlocks != g_vrVertexConstantBlocks)
    {
        g_dirtyStates.vertexShaderConstants = true;
        g_vrVertexConstantBlocks = vrVertexBlocks;
    }

    if (vrPixelBlocks != g_vrPixelConstantBlocks)
    {
        g_dirtyStates.pixelShaderConstants = true;
        g_vrPixelConstantBlocks = vrPixelBlocks;
    }

    if (g_vrSinglePass)
    {
        uint32_t eyeMode = VR_EYE_MODE_LEFT;

        if (vrStereo)
            eyeMode = (g_vrStereoDraw == VRStereoDraw::BothEyes ? VR_EYE_MODE_BOTH : VR_EYE_MODE_LEFT) | VR_EYE_MODE_LAYERED;

        if (vrDrawOnPanel)
            eyeMode |= VR_EYE_MODE_PANEL;

        if (vrVertexBlocks == 1)
            eyeMode |= VR_EYE_MODE_SHARED_VERTEX_CONSTANTS;

        if (vrPixelBlocks == 1)
            eyeMode |= VR_EYE_MODE_SHARED_PIXEL_CONSTANTS;

        SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.vrEyeMode, eyeMode);
    }

    // Partial uploads: a shader needing registers the last upload lacks gets a new one.
    const uint64_t* vertexConstantMask = GetVRConstantMask(g_pipelineState.vertexShader);
    const uint64_t* pixelConstantMask = GetVRConstantMask(g_pipelineState.pixelShader);

    if (!IsVRConstantSubset(vertexConstantMask, g_vrUploadedVertexConstants))
        g_dirtyStates.vertexShaderConstants = true;

    if (!IsVRConstantSubset(pixelConstantMask, g_vrUploadedPixelConstants))
        g_dirtyStates.pixelShaderConstants = true;

    auto vrUploadStart = std::chrono::steady_clock::now();

    // The eye of the first block: the eye frame replay draws, or the left eye.
    uint32_t vrFirstEye = (!g_vrSinglePass && g_vrPass > 0) ? uint32_t(g_vrPass) : 0;
    uint32_t vrPatchedEyes = std::max(vrVertexBlocks, vrPixelBlocks);

    VRCameraPatch vrCameraPatches[2];
    bool vrPatch[2] = {};

    if (vrDrawKind == VRDrawKind::Camera && (g_dirtyStates.vertexShaderConstants || g_dirtyStates.pixelShaderConstants))
    {
        for (uint32_t i = 0; i < vrPatchedEyes; i++)
            vrPatch[i] = ComputeVRCameraPatch(vrCameraPatches[i], vrFirstEye + i);
    }

    if (g_dirtyStates.vertexShaderConstants)
    {
        auto vertexShaderConstants = UploadShaderConstants(g_vertexShaderConstants, vrVertexBlocks, vertexConstantMask);
        memcpy(g_vrUploadedVertexConstants, vertexConstantMask, sizeof(g_vrUploadedVertexConstants));

        for (uint32_t i = 0; i < vrVertexBlocks; i++)
        {
            float* constants = reinterpret_cast<float*>(vertexShaderConstants.memory) + i * std::size(g_vertexShaderConstants);

            if (vrPatch[i])
                PatchVRVertexShaderConstants(constants, vrCameraPatches[i]);
            else if (vrDrawKind == VRDrawKind::Reflection)
                PatchVRReflectionProjection(constants, vrFirstEye + i);
        }

        if (vrVertexBlocks > 1)
            ++g_vrStatistics.perEyeVertexConstants;

        SetRootDescriptor(vertexShaderConstants, 0);
    }

    if (g_dirtyStates.pixelShaderConstants)
    {
        auto pixelShaderConstants = UploadShaderConstants(g_pixelShaderConstants, vrPixelBlocks, pixelConstantMask);
        memcpy(g_vrUploadedPixelConstants, pixelConstantMask, sizeof(g_vrUploadedPixelConstants));

        for (uint32_t i = 0; i < vrPixelBlocks; i++)
        {
            float* constants = reinterpret_cast<float*>(pixelShaderConstants.memory) + i * std::size(g_pixelShaderConstants);
            bool count = (g_vrPass == 0 && i == 0);

            if (vrPatch[i] && vrPixelPatchShader != nullptr)
                PatchVRPixelShaderConstants(constants, vrCameraPatches[i]);

            if (vrNeutraliseBoost)
                NeutraliseVRBoostPass(constants, count);

            if (vrReplaceBloomFeedback)
                ReplaceVRBloomFeedbackWeight(constants);

            if (vrPatchLightShafts)
                PatchVRLightShafts(constants, vrFirstEye + i, count);

            if (vrNeutraliseDepthOfField)
                NeutraliseVRDepthOfField(constants, count);
        }

        if (vrPixelBlocks > 1)
            ++g_vrStatistics.perEyePixelConstants;

        SetRootDescriptor(pixelShaderConstants, 1);
    }

    g_vrStatistics.constantUploadMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - vrUploadStart).count();

    if (g_dirtyStates.sharedConstants)
        UploadSharedConstants();

    if (g_dirtyStates.vertexStreamFirst <= g_dirtyStates.vertexStreamLast)
    {
        commandList->setVertexBuffers(
            g_dirtyStates.vertexStreamFirst,
            g_vertexBufferViews + g_dirtyStates.vertexStreamFirst,
            g_dirtyStates.vertexStreamLast - g_dirtyStates.vertexStreamFirst + 1,
            g_inputSlots + g_dirtyStates.vertexStreamFirst);
    }

    if (g_dirtyStates.indices && (!g_vulkan || g_indexBufferView.buffer.ref != nullptr))
        commandList->setIndexBuffer(&g_indexBufferView);

    g_dirtyStates = DirtyStates(false);
}

static RenderPrimitiveTopology ConvertPrimitiveType(uint32_t primitiveType)
{
    switch (primitiveType)
    {
    case D3DPT_POINTLIST:
        return RenderPrimitiveTopology::POINT_LIST;
    case D3DPT_LINELIST:
        return RenderPrimitiveTopology::LINE_LIST;
    case D3DPT_LINESTRIP:
        return RenderPrimitiveTopology::LINE_STRIP;
    case D3DPT_TRIANGLELIST:
    case D3DPT_QUADLIST:
        return RenderPrimitiveTopology::TRIANGLE_LIST;
    case D3DPT_TRIANGLESTRIP:
        return RenderPrimitiveTopology::TRIANGLE_STRIP;
    case D3DPT_TRIANGLEFAN:
        return g_capabilities.triangleFan ? RenderPrimitiveTopology::TRIANGLE_FAN : RenderPrimitiveTopology::TRIANGLE_LIST;
    default:
        assert(false && "Unknown primitive type");
        return RenderPrimitiveTopology::UNKNOWN;
    }
}

static void SetPrimitiveType(uint32_t primitiveType)
{
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.primitiveTopology, ConvertPrimitiveType(primitiveType));
}

static uint32_t CheckInstancing()
{
    uint32_t indexCount = 0;

    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.instancing, g_pipelineState.vertexDeclaration->indexVertexStream != 0);
    if (g_pipelineState.instancing)
    {
        // Index buffer is passed as a vertex stream
        indexCount = g_vertexBufferViews[g_pipelineState.vertexDeclaration->indexVertexStream].size / 4;
    }

    return indexCount;
}

static void UnsetInstancingStream()
{
    bool dirty = false;
    uint32_t index = g_pipelineState.vertexDeclaration->indexVertexStream;

    SetDirtyValue(dirty, g_vertexBufferViews[index].buffer, RenderBufferReference{});
    SetDirtyValue(dirty, g_vertexBufferViews[index].size, 0u);
    SetDirtyValue(dirty, g_inputSlots[index].stride, 0u);

    if (dirty)
    {
        g_dirtyStates.vertexStreamFirst = std::min<uint8_t>(g_dirtyStates.vertexStreamFirst, index);
        g_dirtyStates.vertexStreamLast = std::max<uint8_t>(g_dirtyStates.vertexStreamLast, index);
    }
}

static void DrawPrimitive(GuestDevice* device, uint32_t primitiveType, uint32_t startVertex, uint32_t primitiveCount) 
{
    LocalRenderCommandQueue queue;
    FlushRenderStateForMainThread(device, queue);

    auto& cmd = queue.enqueue();
    cmd.type = RenderCommandType::DrawPrimitive;
    cmd.drawPrimitive.primitiveType = primitiveType;
    cmd.drawPrimitive.startVertex = startVertex;
    cmd.drawPrimitive.primitiveCount = primitiveCount;

    queue.submit();
}

VRDrawTimer::~VRDrawTimer()
{
    g_vrStatistics.drawMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

static void ProcDrawPrimitive(const RenderCommand& cmd)
{
    VRDrawTimer timer;
    const auto& args = cmd.drawPrimitive;

    SetPrimitiveType(args.primitiveType);

    uint32_t indexCount = CheckInstancing();
    if (indexCount > 0)
    {
        auto& vertexBufferView = g_vertexBufferViews[g_pipelineState.vertexDeclaration->indexVertexStream];

        SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.buffer, vertexBufferView.buffer);
        SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.size, vertexBufferView.size);
        SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.format, RenderFormat::R32_UINT);

        UnsetInstancingStream();
    }

    if (ShouldReuseVRShadowWork())
        return;

    FlushRenderStateForRenderThread();

    auto& commandList = g_commandLists[g_frame];

    DrawForVREyes([&](uint32_t instances)
        {
            if (indexCount > 0)
                commandList->drawIndexedInstanced(indexCount, args.primitiveCount / indexCount * instances, 0, 0, 0);
            else
                commandList->drawInstanced(args.primitiveCount, instances, args.startVertex, 0);
        });
}

static void DrawIndexedPrimitive(GuestDevice* device, uint32_t primitiveType, int32_t baseVertexIndex, uint32_t startIndex, uint32_t primCount)
{
    LocalRenderCommandQueue queue;
    FlushRenderStateForMainThread(device, queue);

    auto& cmd = queue.enqueue();
    cmd.type = RenderCommandType::DrawIndexedPrimitive;
    cmd.drawIndexedPrimitive.primitiveType = primitiveType;
    cmd.drawIndexedPrimitive.baseVertexIndex = baseVertexIndex;
    cmd.drawIndexedPrimitive.startIndex = startIndex;
    cmd.drawIndexedPrimitive.primCount = primCount;

    queue.submit();
}

static void ProcDrawIndexedPrimitive(const RenderCommand& cmd)
{
    VRDrawTimer timer;
    const auto& args = cmd.drawIndexedPrimitive;

    uint32_t indexCount = CheckInstancing();
    if (indexCount > 0)
        UnsetInstancingStream();

    SetPrimitiveType(args.primitiveType);

    if (ShouldReuseVRShadowWork())
        return;

    FlushRenderStateForRenderThread();

    DrawForVREyes([&](uint32_t instances)
        {
            g_commandLists[g_frame]->drawIndexedInstanced(args.primCount, instances, args.startIndex, args.baseVertexIndex, 0);
        });
}

static void DrawPrimitiveUP(GuestDevice* device, uint32_t primitiveType, uint32_t primitiveCount, void* vertexStreamZeroData, uint32_t vertexStreamZeroStride)
{
    LocalRenderCommandQueue queue;
    FlushRenderStateForMainThread(device, queue);

    auto& cmd = queue.enqueue();
    cmd.type = RenderCommandType::DrawPrimitiveUP;
    cmd.drawPrimitiveUP.primitiveType = primitiveType;
    cmd.drawPrimitiveUP.primitiveCount = primitiveCount;
    cmd.drawPrimitiveUP.vertexStreamZeroData = g_intermediaryUploadAllocator.allocate(vertexStreamZeroData, primitiveCount * vertexStreamZeroStride);
    cmd.drawPrimitiveUP.vertexStreamZeroSize = primitiveCount * vertexStreamZeroStride;
    cmd.drawPrimitiveUP.vertexStreamZeroStride = vertexStreamZeroStride;
    cmd.drawPrimitiveUP.csdFilterState = g_csdFilterState;
    
    queue.submit();
}

static void ProcDrawPrimitiveUP(const RenderCommand& cmd)
{
    VRDrawTimer timer;
    const auto& args = cmd.drawPrimitiveUP;

    uint32_t indexCount = CheckInstancing();
    if (indexCount > 0)
        UnsetInstancingStream();

    SetPrimitiveType(args.primitiveType);
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexStrides[0], uint8_t(args.vertexStreamZeroStride));

    auto allocation = g_uploadAllocators[g_frame].allocate<true>(reinterpret_cast<const uint32_t*>(args.vertexStreamZeroData), args.vertexStreamZeroSize, 0x4);

    if (IsVRFrameImmersive() && PlaceVR2DPrimitiveOnPanel(args.vertexStreamZeroData, allocation.memory, args.vertexStreamZeroSize, args.vertexStreamZeroStride))
        g_vrNextDrawOnPanel = true;

    auto& vertexBufferView = g_vertexBufferViews[0];
    vertexBufferView.size = args.primitiveCount * args.vertexStreamZeroStride;
    vertexBufferView.buffer = allocation.buffer->at(allocation.offset);
    g_inputSlots[0].stride = args.vertexStreamZeroStride;
    g_dirtyStates.vertexStreamFirst = 0;

    indexCount = 0;

    if (args.primitiveType == D3DPT_QUADLIST)
        indexCount = g_quadIndexData.prepare(args.primitiveCount);
    else if (!g_capabilities.triangleFan && args.primitiveType == D3DPT_TRIANGLEFAN)
        indexCount = g_triangleFanIndexData.prepare(args.primitiveCount);

    if (args.csdFilterState != CsdFilterState::Unknown &&
        (g_pipelineState.pixelShader == g_csdShader || g_pipelineState.pixelShader == g_csdFilterShader.get()))
    {
        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.pixelShader,
            args.csdFilterState == CsdFilterState::On ? g_csdFilterShader.get() : g_csdShader);
    }

    // VR: in stereo frames the HUD goes on a panel, except plain colour over the
    // whole screen (fades). Textured draws go on it even when they cover the
    // canvas: pictures fading in, and elements zooming in from beyond the screen.
    if (g_pipelineState.vertexShader != nullptr && g_pipelineState.vertexShader->vrHudShader)
    {
        bool untextured = g_pipelineState.vertexShader->vrHash == 0xB1086A4947A797DE; // csd_no_tex_vs
        bool viewLocked = untextured && CoversVRHudCanvas(args.vertexStreamZeroData, args.vertexStreamZeroSize, args.vertexStreamZeroStride);
        SetVRHudOnPanel(IsVRFrameImmersive() && !viewLocked);

        if (viewLocked)
            NoteVRViewLocked(g_vrStatistics.viewLockedCsd, "untextured CSD covering the canvas");
    }

    if (ShouldReuseVRShadowWork())
        return;

    g_vrDrawIsUP = true;
    FlushRenderStateForRenderThread();
    g_vrDrawIsUP = false;

    DrawForVREyes([&](uint32_t instances)
        {
            if (indexCount != 0)
                g_commandLists[g_frame]->drawIndexedInstanced(indexCount, instances, 0, 0, 0);
            else
                g_commandLists[g_frame]->drawInstanced(args.primitiveCount, instances, 0, 0);
        });
}

static const char* ConvertDeclUsage(uint32_t usage)
{
    switch (usage)
    {
    case D3DDECLUSAGE_POSITION:
        return "POSITION";
    case D3DDECLUSAGE_BLENDWEIGHT:
        return "BLENDWEIGHT";
    case D3DDECLUSAGE_BLENDINDICES:
        return "BLENDINDICES";
    case D3DDECLUSAGE_NORMAL:
        return "NORMAL";
    case D3DDECLUSAGE_PSIZE:
        return "PSIZE";
    case D3DDECLUSAGE_TEXCOORD:
        return "TEXCOORD";
    case D3DDECLUSAGE_TANGENT:
        return "TANGENT";
    case D3DDECLUSAGE_BINORMAL:
        return "BINORMAL";
    case D3DDECLUSAGE_TESSFACTOR:
        return "TESSFACTOR";
    case D3DDECLUSAGE_POSITIONT:
        return "POSITIONT";
    case D3DDECLUSAGE_COLOR:
        return "COLOR";
    case D3DDECLUSAGE_FOG:
        return "FOG";
    case D3DDECLUSAGE_DEPTH:
        return "DEPTH";
    case D3DDECLUSAGE_SAMPLE:
        return "SAMPLE";
    default:
        assert(false && "Unknown usage");
        return "UNKNOWN";
    }
}

static RenderFormat ConvertDeclType(uint32_t type)
{
    switch (type)
    {
    case D3DDECLTYPE_FLOAT1:
        return RenderFormat::R32_FLOAT;
    case D3DDECLTYPE_FLOAT2:
        return RenderFormat::R32G32_FLOAT;
    case D3DDECLTYPE_FLOAT3:
        return RenderFormat::R32G32B32_FLOAT;
    case D3DDECLTYPE_FLOAT4:
        return RenderFormat::R32G32B32A32_FLOAT;
    case D3DDECLTYPE_D3DCOLOR:
        return RenderFormat::B8G8R8A8_UNORM;
    case D3DDECLTYPE_UBYTE4:
    case D3DDECLTYPE_UBYTE4_2:
        return RenderFormat::R8G8B8A8_UINT;
    case D3DDECLTYPE_SHORT2:
        return RenderFormat::R16G16_SINT;
    case D3DDECLTYPE_SHORT4:
        return RenderFormat::R16G16B16A16_SINT;
    case D3DDECLTYPE_UBYTE4N:
    case D3DDECLTYPE_UBYTE4N_2:
        return RenderFormat::R8G8B8A8_UNORM;
    case D3DDECLTYPE_SHORT2N:
        return RenderFormat::R16G16_SNORM;
    case D3DDECLTYPE_SHORT4N:
        return RenderFormat::R16G16B16A16_SNORM;
    case D3DDECLTYPE_USHORT2N:
        return RenderFormat::R16G16_UNORM;
    case D3DDECLTYPE_USHORT4N:
        return RenderFormat::R16G16B16A16_UNORM;
    case D3DDECLTYPE_UINT1:
        return RenderFormat::R32_UINT;
    case D3DDECLTYPE_DEC3N_2:
    case D3DDECLTYPE_DEC3N_3:
        return RenderFormat::R32_UINT;
    case D3DDECLTYPE_FLOAT16_2:
        return RenderFormat::R16G16_FLOAT;
    case D3DDECLTYPE_FLOAT16_4:
        return RenderFormat::R16G16B16A16_FLOAT;
    default:
        assert(false && "Unknown type");
        return RenderFormat::UNKNOWN;
    }
}

static GuestVertexDeclaration* CreateVertexDeclarationWithoutAddRef(GuestVertexElement* vertexElements) 
{
    size_t vertexElementCount = 0;
    auto vertexElement = vertexElements;

    while (vertexElement->stream != 0xFF && vertexElement->type != D3DDECLTYPE_UNUSED)
    {
        vertexElement->padding = 0;
        ++vertexElement;
        ++vertexElementCount;
    }

    vertexElement->padding = 0; // Clear the padding in D3DDECL_END() 

    std::lock_guard lock(g_vertexDeclarationMutex);

    XXH64_hash_t hash = XXH3_64bits(vertexElements, vertexElementCount * sizeof(GuestVertexElement));
    auto& vertexDeclaration = g_vertexDeclarations[hash];

    if (vertexDeclaration == nullptr)
    {
        vertexDeclaration = g_userHeap.AllocPhysical<GuestVertexDeclaration>(ResourceType::VertexDeclaration);
        vertexDeclaration->hash = hash;

        static std::vector<RenderInputElement> inputElements;
        inputElements.clear();

        struct Location
        {
            uint32_t usage;
            uint32_t usageIndex;
            uint32_t location;
        };

        constexpr Location locations[] =
        {
            { D3DDECLUSAGE_POSITION, 0, 0 },
            { D3DDECLUSAGE_NORMAL, 0, 1 },
            { D3DDECLUSAGE_TANGENT, 0, 2 },
            { D3DDECLUSAGE_BINORMAL, 0, 3 },
            { D3DDECLUSAGE_TEXCOORD, 0, 4 },
            { D3DDECLUSAGE_TEXCOORD, 1, 5 },
            { D3DDECLUSAGE_TEXCOORD, 2, 6 },
            { D3DDECLUSAGE_TEXCOORD, 3, 7 },
            { D3DDECLUSAGE_COLOR, 0, 8 },
            { D3DDECLUSAGE_BLENDINDICES, 0, 9 },
            { D3DDECLUSAGE_BLENDWEIGHT, 0, 10 },
            { D3DDECLUSAGE_COLOR, 1, 11 },
            { D3DDECLUSAGE_TEXCOORD, 4, 12 },
            { D3DDECLUSAGE_TEXCOORD, 5, 13 },
            { D3DDECLUSAGE_TEXCOORD, 6, 14 },
            { D3DDECLUSAGE_TEXCOORD, 7, 15 },
            { D3DDECLUSAGE_POSITION, 1, 15 }
        };

        vertexElement = vertexElements;
        while (vertexElement->stream != 0xFF && vertexElement->type != D3DDECLTYPE_UNUSED)
        {
            if (vertexElement->usage == D3DDECLUSAGE_POSITION && vertexElement->usageIndex == 2)
            {
                ++vertexElement;
                continue;
            }

            auto& inputElement = inputElements.emplace_back();
            
            inputElement.semanticName = ConvertDeclUsage(vertexElement->usage);
            inputElement.semanticIndex = vertexElement->usageIndex;
            inputElement.location = ~0;

            for (auto& location : locations)
            {
                if (location.usage == vertexElement->usage && location.usageIndex == vertexElement->usageIndex)
                {
                    inputElement.location = location.location;
                    break;
                }
            }

            assert(inputElement.location != ~0);

            inputElement.format = ConvertDeclType(vertexElement->type);
            inputElement.slotIndex = vertexElement->stream;
            inputElement.alignedByteOffset = vertexElement->offset;

            switch (vertexElement->usage)
            {
            case D3DDECLUSAGE_POSITION:
                if (vertexElement->usageIndex == 1)
                    vertexDeclaration->indexVertexStream = vertexElement->stream;
                break;

            case D3DDECLUSAGE_NORMAL:
            case D3DDECLUSAGE_TANGENT:
            case D3DDECLUSAGE_BINORMAL:
                if (vertexElement->type == D3DDECLTYPE_FLOAT3)
                    inputElement.format = RenderFormat::R32G32B32_UINT;
                else
                    vertexDeclaration->hasR11G11B10Normal = true;
                break;

            case D3DDECLUSAGE_TEXCOORD:
                switch (vertexElement->type)
                {
                case D3DDECLTYPE_SHORT2:
                case D3DDECLTYPE_SHORT4:
                case D3DDECLTYPE_SHORT2N:
                case D3DDECLTYPE_SHORT4N:
                case D3DDECLTYPE_USHORT2N:
                case D3DDECLTYPE_USHORT4N:
                case D3DDECLTYPE_FLOAT16_2:
                case D3DDECLTYPE_FLOAT16_4:
                    vertexDeclaration->swappedTexcoords |= 1 << vertexElement->usageIndex;
                    break;
                }

                break;
            }

            vertexDeclaration->vertexStreams[vertexElement->stream] = true;

            ++vertexElement;
        }

        auto addInputElement = [&](uint32_t usage, uint32_t usageIndex)
            {
                uint32_t location = ~0;

                for (auto& alsoLocation : locations)
                {
                    if (alsoLocation.usage == usage && alsoLocation.usageIndex == usageIndex)
                    {
                        location = alsoLocation.location;
                        break;
                    }
                }

                assert(location != ~0);

                for (auto& inputElement : inputElements)
                {
                    if (inputElement.location == location)
                        return;
                }

                auto format = RenderFormat::R32_FLOAT;
                switch (usage)
                {
                case D3DDECLUSAGE_NORMAL:
                case D3DDECLUSAGE_TANGENT:
                case D3DDECLUSAGE_BINORMAL:
                case D3DDECLUSAGE_BLENDINDICES:
                    format = RenderFormat::R32_UINT;
                    break;
                }

                inputElements.emplace_back(ConvertDeclUsage(usage), usageIndex, location, format, 15, 0);
            };

        addInputElement(D3DDECLUSAGE_POSITION, 0);
        addInputElement(D3DDECLUSAGE_NORMAL, 0);
        addInputElement(D3DDECLUSAGE_TANGENT, 0);
        addInputElement(D3DDECLUSAGE_BINORMAL, 0);
        addInputElement(D3DDECLUSAGE_TEXCOORD, 0);
        addInputElement(D3DDECLUSAGE_TEXCOORD, 1);
        addInputElement(D3DDECLUSAGE_TEXCOORD, 2);
        addInputElement(D3DDECLUSAGE_TEXCOORD, 3);
        addInputElement(D3DDECLUSAGE_COLOR, 0);
        addInputElement(D3DDECLUSAGE_BLENDWEIGHT, 0);
        addInputElement(D3DDECLUSAGE_BLENDINDICES, 0);

        vertexDeclaration->inputElements = std::make_unique<RenderInputElement[]>(inputElements.size());
        std::copy(inputElements.begin(), inputElements.end(), vertexDeclaration->inputElements.get());

        vertexDeclaration->vertexElements = std::make_unique<GuestVertexElement[]>(vertexElementCount + 1);
        std::copy(vertexElements, vertexElements + vertexElementCount + 1, vertexDeclaration->vertexElements.get());

        vertexDeclaration->inputElementCount = uint32_t(inputElements.size());
        vertexDeclaration->vertexElementCount = vertexElementCount + 1;
    }

    vertexDeclaration->AddRef();
    return vertexDeclaration;
}

static GuestVertexDeclaration* CreateVertexDeclaration(GuestVertexElement* vertexElements)
{
    auto vertexDeclaration = CreateVertexDeclarationWithoutAddRef(vertexElements);
    vertexDeclaration->AddRef();
    return vertexDeclaration;
}

static void SetVertexDeclaration(GuestDevice* device, GuestVertexDeclaration* vertexDeclaration) 
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetVertexDeclaration;
    cmd.setVertexDeclaration.vertexDeclaration = vertexDeclaration;
    g_renderQueue.enqueue(cmd);

    device->vertexDeclaration = g_memory.MapVirtual(vertexDeclaration);
}

static void ProcSetVertexDeclaration(const RenderCommand& cmd)
{
    auto& args = cmd.setVertexDeclaration;

    if (args.vertexDeclaration != nullptr)
    {
        SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.swappedTexcoords, args.vertexDeclaration->swappedTexcoords);

        uint32_t specConstants = g_pipelineState.specConstants;
        if (args.vertexDeclaration->hasR11G11B10Normal)
            specConstants |= SPEC_CONSTANT_R11G11B10_NORMAL;
        else
            specConstants &= ~SPEC_CONSTANT_R11G11B10_NORMAL;

        SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants, specConstants);
    }
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexDeclaration, args.vertexDeclaration);
}

static ShaderCacheEntry* FindShaderCacheEntry(XXH64_hash_t hash)
{
    auto end = g_shaderCacheEntries + g_shaderCacheEntryCount;
    auto findResult = std::lower_bound(g_shaderCacheEntries, end, hash, [](ShaderCacheEntry& lhs, XXH64_hash_t rhs)
        {
            return lhs.hash < rhs;
        });

    return findResult != end && findResult->hash == hash ? findResult : nullptr;
}

// Reads the Xenos shader container's constant table (D3DXSHADER_CONSTANTTABLE)
// for the registers of the camera constants VR rewrites per eye.
static void FindVRCameraConstants(GuestShader* shader, const be<uint32_t>* function)
{
    const uint8_t* container = reinterpret_cast<const uint8_t*>(function);
    uint32_t containerSize = function[1] + function[2];
    uint32_t constantTableOffset = function[4];
    if (constantTableOffset == 0 || constantTableOffset + 0x20 > containerSize)
        return;

    // ConstantTableContainer: a size, then the D3DXSHADER_CONSTANTTABLE its offsets are relative to.
    const uint8_t* table = container + constantTableOffset + 4;
    uint32_t tableSize = containerSize - constantTableOffset - 4;
    uint32_t constantCount = *reinterpret_cast<const be<uint32_t>*>(table + 12);
    uint32_t constantInfoOffset = *reinterpret_cast<const be<uint32_t>*>(table + 16);

    constexpr uint32_t ConstantInfoSize = 20;
    constexpr uint16_t RegisterSetFloat4 = 2;
    constexpr uint16_t RegisterSetSampler = 3;

    shader->vrSamplerMask = 0;
    memset(shader->vrConstantMask, 0, sizeof(shader->vrConstantMask));

    for (uint32_t i = 0; i < constantCount; i++)
    {
        uint32_t entryOffset = constantInfoOffset + i * ConstantInfoSize;
        if (entryOffset + ConstantInfoSize > tableSize)
            break;

        const uint8_t* entry = table + entryOffset;
        uint32_t nameOffset = *reinterpret_cast<const be<uint32_t>*>(entry);
        uint16_t registerSet = *reinterpret_cast<const be<uint16_t>*>(entry + 4);
        uint16_t registerIndex = *reinterpret_cast<const be<uint16_t>*>(entry + 6);
        uint16_t registerCount = *reinterpret_cast<const be<uint16_t>*>(entry + 8);

        if ((registerSet != RegisterSetFloat4 && registerSet != RegisterSetSampler) || nameOffset >= tableSize)
            continue;

        std::string_view name(reinterpret_cast<const char*>(table + nameOffset), strnlen(reinterpret_cast<const char*>(table + nameOffset), tableSize - nameOffset));

        if (registerSet == RegisterSetFloat4)
        {
            for (uint32_t j = registerIndex; j < uint32_t(registerIndex) + std::max<uint16_t>(registerCount, 1) && j < 256; j++)
                shader->vrConstantMask[j / 64] |= 1ull << (j % 64);
        }

        if (registerSet == RegisterSetSampler)
        {
            for (uint32_t j = registerIndex; j < uint32_t(registerIndex) + std::max<uint16_t>(registerCount, 1) && j < 16; j++)
                shader->vrSamplerMask |= uint16_t(1u << j);

            if (name == "g_ReflectionMapSampler" && registerIndex < std::size(g_textures))
                shader->vrReflectionSamplerRegister = int8_t(registerIndex);
        }
        else if (name == "g_MtxProjection" && registerCount == 4)
        {
            shader->vrProjectionRegister = registerIndex;
        }
        else if (name == "g_MtxInvProjection" && registerCount == 4)
        {
            shader->vrInverseProjectionRegister = registerIndex;
        }
        else if (name == "g_MtxView" && (registerCount == 3 || registerCount == 4))
        {
            shader->vrViewRegister = registerIndex;
            shader->vrViewRegisterCount = registerCount;
        }
        else if (name == "g_EyePosition")
        {
            shader->vrEyePositionRegister = registerIndex;
        }
        else if (name == "g_EyeDirection")
        {
            shader->vrEyeDirectionRegister = registerIndex;
        }
    }
}

static GuestShader* CreateShader(const be<uint32_t>* function, ResourceType resourceType)
{
    XXH64_hash_t hash = XXH3_64bits(function, function[1] + function[2]);

    auto findResult = FindShaderCacheEntry(hash);
    GuestShader* shader = nullptr;

    if (findResult != nullptr)
    {
        if (findResult->guestShader == nullptr)
        {
            shader = g_userHeap.AllocPhysical<GuestShader>(resourceType);

            if (hash == 0x85ED723035ECF535)
                shader->shader = CREATE_SHADER(blend_color_alpha_ps);
            else if (hash == 0xB1086A4947A797DE)
                shader->shader = CREATE_SHADER(csd_no_tex_vs);
            else if (hash == 0xB4CAFC034A37C8A8)
                shader->shader = CREATE_SHADER(csd_vs);
            else
                shader->shaderCacheEntry = findResult;

            shader->vrHudShader = (hash == 0xB1086A4947A797DE || hash == 0xB4CAFC034A37C8A8);
            shader->vrHash = hash;

            if (hash == 0xD44C189D5067922E) // FxBoostBlur.xpu
                shader->vrBoostShader = VR_BOOST_SHADER_FX_BOOST_BLUR;
            else if (hash == 0xD22F805F4280B8BA) // BoostBlur.xpu
                shader->vrBoostShader = VR_BOOST_SHADER_BOOST_BLUR;
            else if (hash == 0xC533F8ED014AC592) // BoostBlur2D.xpu
                shader->vrBoostShader = VR_BOOST_SHADER_BOOST_BLUR_2D;

            shader->vrPersistentBloomShader = (hash == 0x605C1E349CC4CAAB); // Bloom_BrightPassPersistent.xpu
            shader->vrLightShaftShader = (hash == 0x66578F29004F8FD0); // LightShaftFilter.xpu
            shader->vrDepthOfFieldShader = (hash == 0xFB79F59782376846 || hash == 0x65325B9C7DA3DB04); // AlternativeDOF(_IgnoreSky).xpu

            if (shader->shaderCacheEntry != nullptr)
                FindVRCameraConstants(shader, function);

            findResult->guestShader = shader;
        }
        else
        {
            shader = findResult->guestShader;
        }
    }

    if (shader == nullptr)
        shader = g_userHeap.AllocPhysical<GuestShader>(resourceType);
    else
        shader->AddRef();

    if (hash == 0x31173204A896098A)
        g_csdShader = shader;

    return shader;
}

static GuestShader* CreateVertexShader(const be<uint32_t>* function) 
{
    return CreateShader(function, ResourceType::VertexShader);
}

static void SetVertexShader(GuestDevice* device, GuestShader* shader)
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetVertexShader;
    cmd.setVertexShader.shader = shader;
    g_renderQueue.enqueue(cmd);
}

static void ProcSetVertexShader(const RenderCommand& cmd)
{
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexShader, cmd.setVertexShader.shader);
}

static void SetStreamSource(GuestDevice* device, uint32_t index, GuestBuffer* buffer, uint32_t offset, uint32_t stride) 
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetStreamSource;
    cmd.setStreamSource.index = index;
    cmd.setStreamSource.buffer = buffer;
    cmd.setStreamSource.offset = offset;
    cmd.setStreamSource.stride = stride;
    g_renderQueue.enqueue(cmd);
}

static void ProcSetStreamSource(const RenderCommand& cmd)
{
    const auto& args = cmd.setStreamSource;

    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexStrides[args.index], uint8_t(args.buffer != nullptr ? args.stride : 0));

    bool dirty = false;

    SetDirtyValue(dirty, g_vertexBufferViews[args.index].buffer, args.buffer != nullptr ? args.buffer->buffer->at(args.offset) : RenderBufferReference{});
    SetDirtyValue(dirty, g_vertexBufferViews[args.index].size, args.buffer != nullptr ? (args.buffer->dataSize - args.offset) : 0u);
    SetDirtyValue(dirty, g_inputSlots[args.index].stride, args.buffer != nullptr ? args.stride : 0u);

    if (dirty)
    {
        g_dirtyStates.vertexStreamFirst = std::min<uint8_t>(g_dirtyStates.vertexStreamFirst, args.index);
        g_dirtyStates.vertexStreamLast = std::max<uint8_t>(g_dirtyStates.vertexStreamLast, args.index);
    }
}

static void SetIndices(GuestDevice* device, GuestBuffer* buffer) 
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetIndices;
    cmd.setIndices.buffer = buffer;
    g_renderQueue.enqueue(cmd);
}

static void ProcSetIndices(const RenderCommand& cmd)
{
    const auto& args = cmd.setIndices;

    SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.buffer, args.buffer != nullptr ? args.buffer->buffer->at(0) : RenderBufferReference{});
    SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.format, args.buffer != nullptr ? args.buffer->format : RenderFormat::R16_UINT);
    SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.size, args.buffer != nullptr ? args.buffer->dataSize : 0u);
}

static GuestShader* CreatePixelShader(const be<uint32_t>* function)
{
    return CreateShader(function, ResourceType::PixelShader);
}

static void SetPixelShader(GuestDevice* device, GuestShader* shader)
{
    RenderCommand cmd;
    cmd.type = RenderCommandType::SetPixelShader;
    cmd.setPixelShader.shader = shader;
    g_renderQueue.enqueue(cmd);
}

static void ProcSetPixelShader(const RenderCommand& cmd)
{
    GuestShader* shader = cmd.setPixelShader.shader;
    if (shader != nullptr && 
        shader->shaderCacheEntry != nullptr)
    {
        if (shader->shaderCacheEntry->hash == 0x4294510C775F4EE8)
        {
            size_t shaderIndex = GAUSSIAN_BLUR_3X3;

            switch (Config::DepthOfFieldQuality)
            {
            case EDepthOfFieldQuality::Low:
                shaderIndex = GAUSSIAN_BLUR_3X3;
                break;

            case EDepthOfFieldQuality::Medium:
                shaderIndex = GAUSSIAN_BLUR_5X5;
                break;

            case EDepthOfFieldQuality::High:
                shaderIndex = GAUSSIAN_BLUR_7X7;
                break;

            case EDepthOfFieldQuality::Ultra:
                shaderIndex = GAUSSIAN_BLUR_9X9;
                break;

            default:
            {
                if (g_aspectRatio >= WIDE_ASPECT_RATIO)
                {
                    size_t height = round(Video::s_viewportHeight * Config::ResolutionScale);

                    if (height > 1440)
                        shaderIndex = GAUSSIAN_BLUR_9X9;
                    else if (height > 1080)
                        shaderIndex = GAUSSIAN_BLUR_7X7;
                    else if (height > 720)
                        shaderIndex = GAUSSIAN_BLUR_5X5;
                    else
                        shaderIndex = GAUSSIAN_BLUR_3X3;
                }
                else
                {
                    // Narrow aspect ratios should check for width to account for VERT+.
                    size_t width = round(Video::s_viewportWidth * Config::ResolutionScale);

                    if (width > 2560)
                        shaderIndex = GAUSSIAN_BLUR_9X9;
                    else if (width > 1920)
                        shaderIndex = GAUSSIAN_BLUR_7X7;
                    else if (width > 1280)
                        shaderIndex = GAUSSIAN_BLUR_5X5;
                    else
                        shaderIndex = GAUSSIAN_BLUR_3X3;
                }

                break;
            }
            }

            shader = g_gaussianBlurShaders[shaderIndex].get();
        }
        else if (shader->shaderCacheEntry->hash == 0x6B9732B4CD7E7740 && Config::MotionBlur == EMotionBlur::Enhanced)
        {
            shader = g_enhancedMotionBlurShader.get();
        }
    }

    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.pixelShader, shader);
}

static void ProcessRenderCommand(const RenderCommand& cmd)
{
    // Recorded before running, so a command that ends the recording (ExecuteCommandList) or
    // records an upload of its own (the unlocks) keeps the recording in stream order.
    if (g_vrRecording)
        RecordVRCommand(cmd);

    switch (cmd.type)
    {
    case RenderCommandType::SetRenderState:                    ProcSetRenderState(cmd); break;
    case RenderCommandType::DestructResource:                  ProcDestructResource(cmd); break;
    case RenderCommandType::UnlockTextureRect:                 ProcUnlockTextureRect(cmd); break;
    case RenderCommandType::UnlockBuffer16:                    ProcUnlockBuffer16(cmd); break;
    case RenderCommandType::UnlockBuffer32:                    ProcUnlockBuffer32(cmd); break;
    case RenderCommandType::DrawImGui:                         ProcDrawImGui(cmd); break;
    case RenderCommandType::ExecuteCommandList:                ProcExecuteCommandList(cmd); break;
    case RenderCommandType::BeginCommandList:                  ProcBeginCommandList(cmd); break;
    case RenderCommandType::StretchRect:                       ProcStretchRect(cmd); break;
    case RenderCommandType::SetRenderTarget:                   ProcSetRenderTarget(cmd); break;
    case RenderCommandType::SetDepthStencilSurface:            ProcSetDepthStencilSurface(cmd); break;
    case RenderCommandType::ExecutePendingStretchRectCommands: ProcExecutePendingStretchRectCommands(cmd); break;
    case RenderCommandType::Clear:                             ProcClear(cmd); break;
    case RenderCommandType::SetViewport:                       ProcSetViewport(cmd); break;
    case RenderCommandType::SetTexture:                        ProcSetTexture(cmd); break;
    case RenderCommandType::SetScissorRect:                    ProcSetScissorRect(cmd); break;
    case RenderCommandType::SetSamplerState:                   ProcSetSamplerState(cmd); break;
    case RenderCommandType::SetBooleans:                       ProcSetBooleans(cmd); break;
    case RenderCommandType::SetVertexShaderConstants:          ProcSetVertexShaderConstants(cmd); break;
    case RenderCommandType::SetPixelShaderConstants:           ProcSetPixelShaderConstants(cmd); break;
    case RenderCommandType::AddPipeline:                       ProcAddPipeline(cmd); break;
    case RenderCommandType::DrawPrimitive:                     ProcDrawPrimitive(cmd); break;
    case RenderCommandType::DrawIndexedPrimitive:              ProcDrawIndexedPrimitive(cmd); break;
    case RenderCommandType::DrawPrimitiveUP:                   ProcDrawPrimitiveUP(cmd); break;
    case RenderCommandType::SetVertexDeclaration:              ProcSetVertexDeclaration(cmd); break;
    case RenderCommandType::SetVertexShader:                   ProcSetVertexShader(cmd); break;
    case RenderCommandType::SetStreamSource:                   ProcSetStreamSource(cmd); break;
    case RenderCommandType::SetIndices:                        ProcSetIndices(cmd); break;
    case RenderCommandType::SetPixelShader:                    ProcSetPixelShader(cmd); break;
    case RenderCommandType::VRReplayUpload:                    ProcVRReplayUpload(cmd); break;
    default:                                                   assert(false && "Unrecognized render command type."); break;
    }
}

static std::thread g_renderThread([]
    {
#ifdef _WIN32
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
        GuestThread::SetThreadName(GetCurrentThreadId(), "Render Thread");
#endif

        RenderCommand commands[32];

        while (true)
        {
            size_t count = g_renderQueue.wait_dequeue_bulk(commands, std::size(commands));
            g_renderThreadBatchStart = std::chrono::steady_clock::now();

            for (size_t i = 0; i < count; i++)
                ProcessRenderCommand(commands[i]);

            g_renderThreadBusyMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - g_renderThreadBatchStart).count();
        }
    });

static void D3DXFillTexture(GuestTexture* texture, uint32_t function, void* data)
{
    if (texture->width == 1 && texture->height == 1 && texture->format == RenderFormat::R8_UNORM && function == 0x82BA2150)
    {
        auto uploadBuffer = g_device->createBuffer(RenderBufferDesc::UploadBuffer(PLACEMENT_ALIGNMENT));

        uint8_t* mappedData = reinterpret_cast<uint8_t*>(uploadBuffer->map());
        *mappedData = 0xFF;
        uploadBuffer->unmap();

        ExecuteCopyCommandList([&]
            {
                g_copyCommandList->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(texture->texture, RenderTextureLayout::COPY_DEST));

                g_copyCommandList->copyTextureRegion(
                    RenderTextureCopyLocation::Subresource(texture->texture, 0),
                    RenderTextureCopyLocation::PlacedFootprint(uploadBuffer.get(), texture->format, 1, 1, 1, PLACEMENT_ALIGNMENT, 0));
            });

        texture->layout = RenderTextureLayout::COPY_DEST;
    }
}

static void D3DXFillVolumeTexture(GuestTexture* texture, uint32_t function, void* data)
{
    uint32_t rowPitch0 = (texture->width * 4 + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
    uint32_t slicePitch0 = (rowPitch0 * texture->height * texture->depth + PLACEMENT_ALIGNMENT - 1) & ~(PLACEMENT_ALIGNMENT - 1);

    uint32_t rowPitch1 = ((texture->width / 2) * 4 + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
    uint32_t slicePitch1 = (rowPitch1 * (texture->height / 2) * (texture->depth / 2) + PLACEMENT_ALIGNMENT - 1) & ~(PLACEMENT_ALIGNMENT - 1);

    auto uploadBuffer = g_device->createBuffer(RenderBufferDesc::UploadBuffer(slicePitch0 + slicePitch1));
    uint8_t* mappedData = reinterpret_cast<uint8_t*>(uploadBuffer->map());

    thread_local std::vector<float> mipData;
    mipData.resize((texture->width / 2) * (texture->height / 2) * (texture->depth / 2) * 4);
    memset(mipData.data(), 0, mipData.size() * sizeof(float));

    for (size_t z = 0; z < texture->depth; z++)
    {
        for (size_t y = 0; y < texture->height; y++)
        {
            for (size_t x = 0; x < texture->width; x++)
            {
                auto dest = mappedData + z * rowPitch0 * texture->height + y * rowPitch0 + x * sizeof(uint32_t);
                size_t index = z * texture->width * texture->height + y * texture->width + x;
                size_t mipIndex = ((z / 2) * (texture->width / 2) * (texture->height / 2) + (y / 2) * (texture->width / 2) + x / 2) * 4;

                if (function == 0x82BC7820)
                {
                    auto src = reinterpret_cast<be<float>*>(data) + index * 4;

                    float r = static_cast<uint8_t>(src[0] * 255.0f);
                    float g = static_cast<uint8_t>(src[1] * 255.0f);
                    float b = static_cast<uint8_t>(src[2] * 255.0f);
                    float a = static_cast<uint8_t>(src[3] * 255.0f);

                    dest[0] = r;
                    dest[1] = g;
                    dest[2] = b;
                    dest[3] = a;

                    mipData[mipIndex + 0] += r;
                    mipData[mipIndex + 1] += g;
                    mipData[mipIndex + 2] += b;
                    mipData[mipIndex + 3] += a;
                }
                else if (function == 0x82BC78A8)
                {
                    auto src = reinterpret_cast<uint8_t*>(data) + index * 4;

                    dest[0] = src[3];
                    dest[1] = src[2];
                    dest[2] = src[1];
                    dest[3] = src[0];

                    mipData[mipIndex + 0] += src[3];
                    mipData[mipIndex + 1] += src[2];
                    mipData[mipIndex + 2] += src[1];
                    mipData[mipIndex + 3] += src[0];
                }
            }
        }
    }

    for (size_t z = 0; z < texture->depth / 2; z++)
    {
        for (size_t y = 0; y < texture->height / 2; y++)
        {
            for (size_t x = 0; x < texture->width / 2; x++)
            {
                auto dest = mappedData + slicePitch0 + z * rowPitch1 * (texture->height / 2) + y * rowPitch1 + x * sizeof(uint32_t);
                size_t index = (z * (texture->width / 2) * (texture->height / 2) + y * (texture->width / 2) + x) * 4;

                dest[0] = static_cast<uint8_t>(mipData[index + 0] / 8.0f);
                dest[1] = static_cast<uint8_t>(mipData[index + 1] / 8.0f);
                dest[2] = static_cast<uint8_t>(mipData[index + 2] / 8.0f);
                dest[3] = static_cast<uint8_t>(mipData[index + 3] / 8.0f);
            }
        }
    }

    uploadBuffer->unmap();

    ExecuteCopyCommandList([&]
        {
            g_copyCommandList->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(texture->texture, RenderTextureLayout::COPY_DEST));

            g_copyCommandList->copyTextureRegion(
                RenderTextureCopyLocation::Subresource(texture->texture, 0),
                RenderTextureCopyLocation::PlacedFootprint(uploadBuffer.get(), texture->format, texture->width, texture->height, texture->depth, rowPitch0 / RenderFormatSize(texture->format), 0));

            g_copyCommandList->copyTextureRegion(
                RenderTextureCopyLocation::Subresource(texture->texture, 1),
                RenderTextureCopyLocation::PlacedFootprint(uploadBuffer.get(), texture->format, texture->width / 2, texture->height / 2, texture->depth / 2, rowPitch1 / RenderFormatSize(texture->format), slicePitch0));
        });

    texture->layout = RenderTextureLayout::COPY_DEST;
}

struct GuestPictureData
{
    be<uint32_t> vtable;
    uint8_t flags;
    be<uint32_t> name;
    be<uint32_t> texture;
    be<uint32_t> type;
};

static RenderTextureDimension ConvertTextureDimension(ddspp::TextureType type)
{
    switch (type) 
    {
    case ddspp::Texture1D:
        return RenderTextureDimension::TEXTURE_1D;
    case ddspp::Texture2D:
    case ddspp::Cubemap:
        return RenderTextureDimension::TEXTURE_2D;
    case ddspp::Texture3D:
        return RenderTextureDimension::TEXTURE_3D;
    default:
        assert(false && "Unknown texture type from DDS.");
        return RenderTextureDimension::UNKNOWN;
    }
}

static RenderTextureViewDimension ConvertTextureViewDimension(ddspp::TextureType type)
{
    switch (type)
    {
    case ddspp::Texture1D:
        return RenderTextureViewDimension::TEXTURE_1D;
    case ddspp::Texture2D:
        return RenderTextureViewDimension::TEXTURE_2D;
    case ddspp::Texture3D:
        return RenderTextureViewDimension::TEXTURE_3D;
    case ddspp::Cubemap:
        return RenderTextureViewDimension::TEXTURE_CUBE;
    default:
        assert(false && "Unknown texture type from DDS.");
        return RenderTextureViewDimension::UNKNOWN;
    }
}

static RenderFormat ConvertDXGIFormat(ddspp::DXGIFormat format) 
{
    switch (format)
    {
    case ddspp::R32G32B32A32_TYPELESS:
        return RenderFormat::R32G32B32A32_TYPELESS;
    case ddspp::R32G32B32A32_FLOAT:
        return RenderFormat::R32G32B32A32_FLOAT;
    case ddspp::R32G32B32A32_UINT:
        return RenderFormat::R32G32B32A32_UINT;
    case ddspp::R32G32B32A32_SINT:
        return RenderFormat::R32G32B32A32_SINT;
    case ddspp::R32G32B32_TYPELESS:
        return RenderFormat::R32G32B32_TYPELESS;
    case ddspp::R32G32B32_FLOAT:
        return RenderFormat::R32G32B32_FLOAT;
    case ddspp::R32G32B32_UINT:
        return RenderFormat::R32G32B32_UINT;
    case ddspp::R32G32B32_SINT:
        return RenderFormat::R32G32B32_SINT;
    case ddspp::R16G16B16A16_TYPELESS:
        return RenderFormat::R16G16B16A16_TYPELESS;
    case ddspp::R16G16B16A16_FLOAT:
        return RenderFormat::R16G16B16A16_FLOAT;
    case ddspp::R16G16B16A16_UNORM:
        return RenderFormat::R16G16B16A16_UNORM;
    case ddspp::R16G16B16A16_UINT:
        return RenderFormat::R16G16B16A16_UINT;
    case ddspp::R16G16B16A16_SNORM:
        return RenderFormat::R16G16B16A16_SNORM;
    case ddspp::R16G16B16A16_SINT:
        return RenderFormat::R16G16B16A16_SINT;
    case ddspp::R32G32_TYPELESS:
        return RenderFormat::R32G32_TYPELESS;
    case ddspp::R32G32_FLOAT:
        return RenderFormat::R32G32_FLOAT;
    case ddspp::R32G32_UINT:
        return RenderFormat::R32G32_UINT;
    case ddspp::R32G32_SINT:
        return RenderFormat::R32G32_SINT;
    case ddspp::R8G8B8A8_TYPELESS:
        return RenderFormat::R8G8B8A8_TYPELESS;
    case ddspp::R8G8B8A8_UNORM:
        return RenderFormat::R8G8B8A8_UNORM;
    case ddspp::R8G8B8A8_UINT:
        return RenderFormat::R8G8B8A8_UINT;
    case ddspp::R8G8B8A8_SNORM:
        return RenderFormat::R8G8B8A8_SNORM;
    case ddspp::R8G8B8A8_SINT:
        return RenderFormat::R8G8B8A8_SINT;
    case ddspp::B8G8R8A8_UNORM:
        return RenderFormat::B8G8R8A8_UNORM;
    case ddspp::B8G8R8X8_UNORM:
        return RenderFormat::B8G8R8A8_UNORM;   
    case ddspp::R16G16_TYPELESS:
        return RenderFormat::R16G16_TYPELESS;
    case ddspp::R16G16_FLOAT:
        return RenderFormat::R16G16_FLOAT;
    case ddspp::R16G16_UNORM:
        return RenderFormat::R16G16_UNORM;
    case ddspp::R16G16_UINT:
        return RenderFormat::R16G16_UINT;
    case ddspp::R16G16_SNORM:
        return RenderFormat::R16G16_SNORM;
    case ddspp::R16G16_SINT:
        return RenderFormat::R16G16_SINT;
    case ddspp::R32_TYPELESS:
        return RenderFormat::R32_TYPELESS;
    case ddspp::D32_FLOAT:
        return RenderFormat::D32_FLOAT;
    case ddspp::R32_FLOAT:
        return RenderFormat::R32_FLOAT;
    case ddspp::R32_UINT:
        return RenderFormat::R32_UINT;
    case ddspp::R32_SINT:
        return RenderFormat::R32_SINT;
    case ddspp::R8G8_TYPELESS:
        return RenderFormat::R8G8_TYPELESS;
    case ddspp::R8G8_UNORM:
        return RenderFormat::R8G8_UNORM;
    case ddspp::R8G8_UINT:
        return RenderFormat::R8G8_UINT;
    case ddspp::R8G8_SNORM:
        return RenderFormat::R8G8_SNORM;
    case ddspp::R8G8_SINT:
        return RenderFormat::R8G8_SINT;
    case ddspp::R16_TYPELESS:
        return RenderFormat::R16_TYPELESS;
    case ddspp::R16_FLOAT:
        return RenderFormat::R16_FLOAT;
    case ddspp::D16_UNORM:
        return RenderFormat::D16_UNORM;
    case ddspp::R16_UNORM:
        return RenderFormat::R16_UNORM;
    case ddspp::R16_UINT:
        return RenderFormat::R16_UINT;
    case ddspp::R16_SNORM:
        return RenderFormat::R16_SNORM;
    case ddspp::R16_SINT:
        return RenderFormat::R16_SINT;
    case ddspp::R8_TYPELESS:
        return RenderFormat::R8_TYPELESS;
    case ddspp::R8_UNORM:
        return RenderFormat::R8_UNORM;
    case ddspp::R8_UINT:
        return RenderFormat::R8_UINT;
    case ddspp::R8_SNORM:
        return RenderFormat::R8_SNORM;
    case ddspp::R8_SINT:
        return RenderFormat::R8_SINT;
    case ddspp::BC1_TYPELESS:
        return RenderFormat::BC1_TYPELESS;
    case ddspp::BC1_UNORM:
        return RenderFormat::BC1_UNORM;
    case ddspp::BC1_UNORM_SRGB:
        return RenderFormat::BC1_UNORM_SRGB;
    case ddspp::BC2_TYPELESS:
        return RenderFormat::BC2_TYPELESS;
    case ddspp::BC2_UNORM:
        return RenderFormat::BC2_UNORM;
    case ddspp::BC2_UNORM_SRGB:
        return RenderFormat::BC2_UNORM_SRGB;
    case ddspp::BC3_TYPELESS:
        return RenderFormat::BC3_TYPELESS;
    case ddspp::BC3_UNORM:
        return RenderFormat::BC3_UNORM;
    case ddspp::BC3_UNORM_SRGB:
        return RenderFormat::BC3_UNORM_SRGB;
    case ddspp::BC4_TYPELESS:
        return RenderFormat::BC4_TYPELESS;
    case ddspp::BC4_UNORM:
        return RenderFormat::BC4_UNORM;
    case ddspp::BC4_SNORM:
        return RenderFormat::BC4_SNORM;
    case ddspp::BC5_TYPELESS:
        return RenderFormat::BC5_TYPELESS;
    case ddspp::BC5_UNORM:
        return RenderFormat::BC5_UNORM;
    case ddspp::BC5_SNORM:
        return RenderFormat::BC5_SNORM;
    case ddspp::BC6H_TYPELESS:
        return RenderFormat::BC6H_TYPELESS;
    case ddspp::BC6H_UF16:
        return RenderFormat::BC6H_UF16;
    case ddspp::BC6H_SF16:
        return RenderFormat::BC6H_SF16;
    case ddspp::BC7_TYPELESS:
        return RenderFormat::BC7_TYPELESS;
    case ddspp::BC7_UNORM:
        return RenderFormat::BC7_UNORM;
    case ddspp::BC7_UNORM_SRGB:
        return RenderFormat::BC7_UNORM_SRGB;
    default:
        assert(false && "Unsupported format from DDS.");
        return RenderFormat::UNKNOWN;
    }
}

static bool LoadTexture(GuestTexture& texture, const uint8_t* data, size_t dataSize, RenderComponentMapping componentMapping, bool forceCubeMap = false)
{
    ddspp::Descriptor ddsDesc;
    if (ddspp::decode_header((unsigned char *)(data), ddsDesc) != ddspp::Error)
    {
        forceCubeMap &= (ddsDesc.type == ddspp::Texture2D) && (ddsDesc.arraySize == 1);
        uint32_t arraySize = ddsDesc.type == ddspp::TextureType::Cubemap ? (ddsDesc.arraySize * 6) : ddsDesc.arraySize;
            
        RenderTextureDesc desc;
        desc.dimension = ConvertTextureDimension(ddsDesc.type);
        desc.width = ddsDesc.width;
        desc.height = ddsDesc.height;
        desc.depth = ddsDesc.depth;
        desc.mipLevels = ddsDesc.numMips;
        desc.arraySize = arraySize;
        desc.format = ConvertDXGIFormat(ddsDesc.format);
        desc.flags = ddsDesc.type == ddspp::TextureType::Cubemap ? RenderTextureFlag::CUBE : RenderTextureFlag::NONE;

        if (forceCubeMap)
        {
            desc.arraySize = 6;
            desc.flags = RenderTextureFlag::CUBE;
        }

        texture.textureHolder = g_device->createTexture(desc);
        texture.texture = texture.textureHolder.get();
        texture.layout = RenderTextureLayout::COPY_DEST;

        RenderTextureViewDesc viewDesc;
        viewDesc.format = desc.format;
        viewDesc.dimension = ConvertTextureViewDimension(ddsDesc.type);
        viewDesc.mipLevels = ddsDesc.numMips;
        viewDesc.componentMapping = componentMapping;

        if (forceCubeMap)
            viewDesc.dimension = RenderTextureViewDimension::TEXTURE_CUBE;

        RenderTextureViewDimension viewDimension = viewDesc.dimension;
        viewDesc.dimension = GetShaderViewDimension(viewDimension);

        texture.textureView = texture.texture->createTextureView(viewDesc);
        texture.descriptorIndex = g_textureDescriptorAllocator.allocate();
        g_textureDescriptorSet->setTexture(texture.descriptorIndex, texture.texture, RenderTextureLayout::SHADER_READ, texture.textureView.get());

        texture.width = ddsDesc.width;
        texture.height = ddsDesc.height;
        texture.viewDimension = viewDimension;

        struct Slice
        {
            uint32_t width;
            uint32_t height;
            uint32_t depth;
            uint32_t srcOffset;
            uint32_t dstOffset;
            uint32_t srcRowPitch;
            uint32_t dstRowPitch;
            uint32_t rowCount;
        };

        std::vector<Slice> slices;
        uint32_t curSrcOffset = 0;
        uint32_t curDstOffset = 0;

        for (uint32_t arraySlice = 0; arraySlice < arraySize; arraySlice++)
        {
            for (uint32_t mipSlice = 0; mipSlice < ddsDesc.numMips; mipSlice++)
            {
                auto& slice = slices.emplace_back();

                slice.width = std::max(1u, ddsDesc.width >> mipSlice);
                slice.height = std::max(1u, ddsDesc.height >> mipSlice);
                slice.depth = std::max(1u, ddsDesc.depth >> mipSlice);
                slice.srcOffset = curSrcOffset;
                slice.dstOffset = curDstOffset;
                uint32_t rowPitch = ((slice.width + ddsDesc.blockWidth - 1) / ddsDesc.blockWidth) * ddsDesc.bitsPerPixelOrBlock;
                slice.srcRowPitch = (rowPitch + 7) / 8;
                slice.dstRowPitch = (slice.srcRowPitch + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
                slice.rowCount = (slice.height + ddsDesc.blockHeight - 1) / ddsDesc.blockHeight;

                curSrcOffset += slice.srcRowPitch * slice.rowCount * slice.depth;
                curDstOffset += (slice.dstRowPitch * slice.rowCount * slice.depth + PLACEMENT_ALIGNMENT - 1) & ~(PLACEMENT_ALIGNMENT - 1);
            }
        }

        auto uploadBuffer = g_device->createBuffer(RenderBufferDesc::UploadBuffer(curDstOffset));
        uint8_t* mappedMemory = reinterpret_cast<uint8_t*>(uploadBuffer->map());

        for (auto& slice : slices)
        {
            const uint8_t* srcData = data + ddsDesc.headerSize + slice.srcOffset;
            uint8_t* dstData = mappedMemory + slice.dstOffset;

            if (slice.srcRowPitch == slice.dstRowPitch)
            {
                memcpy(dstData, srcData, slice.srcRowPitch * slice.rowCount * slice.depth);
            }
            else
            {
                for (size_t i = 0; i < slice.rowCount * slice.depth; i++)
                {
                    memcpy(dstData, srcData, slice.srcRowPitch);
                    srcData += slice.srcRowPitch;
                    dstData += slice.dstRowPitch;
                }
            }
        }

        uploadBuffer->unmap();

        ExecuteCopyCommandList([&]
            {
                g_copyCommandList->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(texture.texture, RenderTextureLayout::COPY_DEST));

                auto copyTextureRegion = [&](Slice& slice, uint32_t subresourceIndex)
                    {
                        g_copyCommandList->copyTextureRegion(
                            RenderTextureCopyLocation::Subresource(texture.texture, subresourceIndex % ddsDesc.numMips, subresourceIndex / ddsDesc.numMips),
                            RenderTextureCopyLocation::PlacedFootprint(uploadBuffer.get(), desc.format, slice.width, slice.height, slice.depth, (slice.dstRowPitch * 8) / ddsDesc.bitsPerPixelOrBlock * ddsDesc.blockWidth, slice.dstOffset));
                    };

                for (size_t i = 0; i < slices.size(); i++)
                    copyTextureRegion(slices[i], i);

                // Duplicate the first face across the remaining 6 faces.
                if (forceCubeMap)
                {
                    for (size_t i = 1; i < 6; i++)
                    {
                        for (size_t j = 0; j < slices.size(); j++)
                            copyTextureRegion(slices[j], (slices.size() * i) + j);
                    }
                }
            });

        return true;
    }
    else
    {
        int width, height;
        void* stbImage = stbi_load_from_memory(data, dataSize, &width, &height, nullptr, 4);

        if (stbImage != nullptr)
        {
            texture.textureHolder = g_device->createTexture(RenderTextureDesc::Texture2D(width, height, 1, RenderFormat::R8G8B8A8_UNORM));
            texture.texture = texture.textureHolder.get();
            texture.viewDimension = RenderTextureViewDimension::TEXTURE_2D;
            texture.layout = RenderTextureLayout::COPY_DEST;

            texture.descriptorIndex = g_textureDescriptorAllocator.allocate();
            SetTexture2DDescriptor(texture.descriptorIndex, texture.texture, RenderFormat::R8G8B8A8_UNORM, texture.textureView);

            uint32_t rowPitch = (width * 4 + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
            uint32_t slicePitch = rowPitch * height;

            auto uploadBuffer = g_device->createBuffer(RenderBufferDesc::UploadBuffer(slicePitch));
            uint8_t* mappedMemory = reinterpret_cast<uint8_t*>(uploadBuffer->map());

            if (rowPitch == (width * 4))
            {
                memcpy(mappedMemory, stbImage, slicePitch);
            }
            else
            {
                auto data = reinterpret_cast<const uint8_t*>(stbImage);

                for (size_t i = 0; i < height; i++)
                {
                    memcpy(mappedMemory, data, width * 4);
                    data += width * 4;
                    mappedMemory += rowPitch;
                }
            }

            uploadBuffer->unmap();

            stbi_image_free(stbImage);

            ExecuteCopyCommandList([&]
                {
                    g_copyCommandList->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(texture.texture, RenderTextureLayout::COPY_DEST));

                    g_copyCommandList->copyTextureRegion(
                        RenderTextureCopyLocation::Subresource(texture.texture, 0),
                        RenderTextureCopyLocation::PlacedFootprint(uploadBuffer.get(), RenderFormat::R8G8B8A8_UNORM, width, height, 1, rowPitch / 4, 0));
                });

            return true;
        }
    }

    return false;
}

std::unique_ptr<GuestTexture> LoadTexture(const uint8_t* data, size_t dataSize, RenderComponentMapping componentMapping)
{
    GuestTexture texture(ResourceType::Texture);

    if (LoadTexture(texture, data, dataSize, componentMapping))
        return std::make_unique<GuestTexture>(std::move(texture));

    return nullptr;
}

static void DiffPatchTexture(GuestTexture& texture, uint8_t* data, uint32_t dataSize, XXH64_hash_t hash)
{
    auto header = reinterpret_cast<BlockCompressionDiffPatchHeader*>(g_buttonBcDiff.get());
    auto entries = reinterpret_cast<BlockCompressionDiffPatchEntry*>(g_buttonBcDiff.get() + header->entriesOffset);
    auto end = entries + header->entryCount;
    
    auto findResult = std::lower_bound(entries, end, hash, [](BlockCompressionDiffPatchEntry& lhs, XXH64_hash_t rhs)
        {
            return lhs.hash < rhs;
        });

    if (findResult != end && findResult->hash == hash)
    {
        auto patch = reinterpret_cast<BlockCompressionDiffPatch*>(g_buttonBcDiff.get() + findResult->patchesOffset);
        for (size_t i = 0; i < findResult->patchCount; i++)
        {
            assert(patch->destinationOffset + patch->patchBytesSize <= dataSize);
            memcpy(data + patch->destinationOffset, g_buttonBcDiff.get() + patch->patchBytesOffset, patch->patchBytesSize);
            ++patch;
        }

        GuestTexture patchedTexture(ResourceType::Texture);
        if (LoadTexture(patchedTexture, data, dataSize, {}))
            texture.patchedTexture = std::make_unique<GuestTexture>(std::move(patchedTexture));
    }
}

static void MakePictureData(GuestPictureData* pictureData, uint8_t* data, uint32_t dataSize)
{
    if ((pictureData->flags & 0x1) == 0 && data != nullptr)
    {
        GuestTexture texture(ResourceType::Texture);

        if (LoadTexture(texture, data, dataSize, {}))
        {
#ifdef _DEBUG
            texture.texture->setName(reinterpret_cast<char*>(g_memory.Translate(pictureData->name + 2)));
#endif
            XXH64_hash_t hash = XXH3_64bits(data, dataSize);

            // The whale in Cool Edge has a 2D texture assigned as a cubemap which makes it not display in recomp.
            // The hardware duplicates the first face to the remaining 6 faces, so to simulate that we'll recreate the asset.
            bool forceCubeMap = (dataSize == 0xAB38) && (hash == 0x160E9E250FDE88A9);
            if (forceCubeMap)
            {
                GuestTexture recreatedCubeMapTexture(ResourceType::Texture);
                if (LoadTexture(recreatedCubeMapTexture, data, dataSize, {}, true))
                    texture.recreatedCubeMapTexture = std::make_unique<GuestTexture>(std::move(recreatedCubeMapTexture));
            }

            DiffPatchTexture(texture, data, dataSize, hash);

            pictureData->texture = g_memory.MapVirtual(g_userHeap.AllocPhysical<GuestTexture>(std::move(texture)));
            pictureData->type = 0;
        }
    }
}

void IndexBufferLengthMidAsmHook(PPCRegister& r3)
{
    r3.u64 *= 2;
}

void SetShadowResolutionMidAsmHook(PPCRegister& r11)
{
    auto res = (int32_t)Config::ShadowResolution.Value;

    if (res > 0)
        r11.u64 = res;
}

static void SetResolution(be<uint32_t>* device)
{
    Video::ComputeViewportDimensions();

    uint32_t width = uint32_t(round(Video::s_viewportWidth * Config::ResolutionScale));
    uint32_t height = uint32_t(round(Video::s_viewportHeight * Config::ResolutionScale));
    device[46] = width == 0 ? 880 : width;
    device[47] = height == 0 ? 720 : height;
}

// The game does some weird stuff to render targets if they are above 
// 1024x1024 resolution, setting this bool at address 20 seems to avoid all that.
PPC_FUNC(sub_82E9F048)
{
    PPC_STORE_U8(ctx.r4.u32 + 20, 1);
    PPC_STORE_U32(ctx.r4.u32 + 44, PPC_LOAD_U32(ctx.r4.u32 + 8)); // Width
    PPC_STORE_U32(ctx.r4.u32 + 48, PPC_LOAD_U32(ctx.r4.u32 + 12)); // Height
}

static GuestShader* g_movieVertexShader;
static GuestShader* g_moviePixelShader;
static GuestVertexDeclaration* g_movieVertexDeclaration;

static void ScreenShaderInit(be<uint32_t>* a1, uint32_t a2, uint32_t a3, GuestVertexElement* vertexElements)
{
    if (g_moviePixelShader == nullptr)
    {
        g_moviePixelShader = g_userHeap.AllocPhysical<GuestShader>(ResourceType::PixelShader);
        g_moviePixelShader->shader = CREATE_SHADER(movie_ps);
    }

    if (g_movieVertexShader == nullptr)
    {
        g_movieVertexShader = g_userHeap.AllocPhysical<GuestShader>(ResourceType::VertexShader);
        g_movieVertexShader->shader = CREATE_SHADER(movie_vs);
    }

    if (g_movieVertexDeclaration == nullptr)
        g_movieVertexDeclaration = CreateVertexDeclarationWithoutAddRef(vertexElements);

    g_moviePixelShader->AddRef();
    g_movieVertexShader->AddRef();
    g_movieVertexDeclaration->AddRef();

    a1[2] = g_memory.MapVirtual(g_moviePixelShader);
    a1[3] = g_memory.MapVirtual(g_movieVertexShader);
    a1[4] = g_memory.MapVirtual(g_movieVertexDeclaration);
}

void MovieRendererMidAsmHook(PPCRegister& r3)
{
    auto device = reinterpret_cast<GuestDevice*>(g_memory.Translate(r3.u32));

    // Force linear filtering & clamp addressing
    for (size_t i = 0; i < 3; i++)
    {
        device->samplerStates[i].data[0] = (device->samplerStates[i].data[0].get() & ~0x7fc00) | 0x24800;
        device->samplerStates[i].data[3] = (device->samplerStates[i].data[3].get() & ~0x1f80000) | 0x1280000;
    }

    device->dirtyFlags[3] = device->dirtyFlags[3].get() | 0xe0000000ull;
}

static PPCRegister g_r4;
static PPCRegister g_r5;

// CRenderDirectorFxPipeline::Initialize
PPC_FUNC_IMPL(__imp__sub_8258C8A0);
PPC_FUNC(sub_8258C8A0)
{
    g_r4 = ctx.r4;
    g_r5 = ctx.r5;
    __imp__sub_8258C8A0(ctx, base);
}

// CRenderDirectorFxPipeline::Update
PPC_FUNC_IMPL(__imp__sub_8258CAE0);
PPC_FUNC(sub_8258CAE0)
{
    if (g_needsResize)
    {
        // Backup job values. These get modified by cutscenes, 
        // and resizing will cause the values to be forgotten.
        auto traverseFxJobs = [&]<typename TCallback>(const TCallback& callback)
        {
            uint32_t scheduler = PPC_LOAD_U32(ctx.r3.u32 + 0xE0);
            if (scheduler != NULL)
            {
                uint32_t member = PPC_LOAD_U32(scheduler + 0x8);
                if (member != NULL)
                {
                    for (uint32_t it = PPC_LOAD_U32(member + 0x24); it != PPC_LOAD_U32(member + 0x28); it += 8)
                    {
                        uint32_t job = PPC_LOAD_U32(it);
                        if (job != NULL)
                            callback(job);
                    }
                }
            }
        };

        union JobValues
        {
            struct
            {
                uint8_t field50[0x18];
                uint8_t field88;
            } fade;

            struct
            {
                uint8_t camera[0x120];
                uint8_t field44;
                uint8_t fieldA0;
            } shadowMap;
        };

        std::map<uint32_t, JobValues> jobValuesMap;
        traverseFxJobs([&](uint32_t job)
            {
                uint32_t vfTable = PPC_LOAD_U32(job);

                if (vfTable == 0x820CA6F8) // SWA::CFxFade
                {
                    // NOTE: Intentionally not storing shared pointers here. 
                    // Game sends messages that assign these every frame already.
                    JobValues jobValues{};

                    memcpy(jobValues.fade.field50, base + job + 0x50, sizeof(jobValues.fade.field50));
                    jobValues.fade.field88 = PPC_LOAD_U8(job + 0x88);

                    jobValuesMap.emplace(PPC_LOAD_U32(job + 0x48), jobValues);
                }
                else if (vfTable == 0x820CAC5C) // SWA::CFxShadowMap
                {
                    for (uint32_t it = PPC_LOAD_U32(job + 0x88); it != PPC_LOAD_U32(job + 0x8C); it += 8)
                    {
                        uint32_t camera = PPC_LOAD_U32(it);
                        if (camera != NULL && PPC_LOAD_U32(camera) == 0x820BF83C) // SWA::CShadowMapCameraLiSPSM
                        {
                            JobValues jobValues{};

                            memcpy(jobValues.shadowMap.camera, base + camera, sizeof(jobValues.shadowMap.camera));
                            jobValues.shadowMap.field44 = PPC_LOAD_U8(job + 0x44);
                            jobValues.shadowMap.fieldA0 = PPC_LOAD_U8(job + 0xA0);

                            jobValuesMap.emplace(vfTable, jobValues);
                            break;
                        }
                    }
                }
            });

        auto r3 = ctx.r3;
        ctx.r4 = g_r4;
        ctx.r5 = g_r5;
        __imp__sub_8258C8A0(ctx, base);
        ctx.r3 = r3;

        // Restore job values.
        traverseFxJobs([&](uint32_t job)
            {
                uint32_t vfTable = PPC_LOAD_U32(job);

                if (vfTable == 0x820CA6F8) // SWA::CFxFade
                {
                    auto findResult = jobValuesMap.find(PPC_LOAD_U32(job + 0x48));
                    if (findResult != jobValuesMap.end()) // May NOT actually be found.
                    {
                        memcpy(base + job + 0x50, findResult->second.fade.field50, sizeof(findResult->second.fade.field50));
                        PPC_STORE_U8(job + 0x88, findResult->second.fade.field88);
                    }
                }
                else if (vfTable == 0x820CAC5C) // SWA::CFxShadowMap
                {
                    auto findResult = jobValuesMap.find(vfTable);
                    if (findResult != jobValuesMap.end()) // Would be weird if this one wasn't found.
                    {
                        for (uint32_t it = PPC_LOAD_U32(job + 0x88); it != PPC_LOAD_U32(job + 0x8C); it += 8)
                        {
                            uint32_t camera = PPC_LOAD_U32(it);
                            if (camera != NULL && PPC_LOAD_U32(camera) == 0x820BF83C) // SWA::CShadowMapCameraLiSPSM
                            {
                                memcpy(base + camera, findResult->second.shadowMap.camera, sizeof(findResult->second.shadowMap.camera));
                                PPC_STORE_U32(job + 0x80, camera);
                                PPC_STORE_U8(job + 0x44, findResult->second.shadowMap.field44);
                                PPC_STORE_U8(job + 0xA0, findResult->second.shadowMap.fieldA0);
                                break;
                            }
                        }
                    }
                }
            });

        g_needsResize = false;
    }

    __imp__sub_8258CAE0(ctx, base);
}

PPC_FUNC_IMPL(__imp__sub_824EB5B0);
PPC_FUNC(sub_824EB5B0)
{
    g_updateDirectorProfiler.Begin();
    __imp__sub_824EB5B0(ctx, base);
    g_updateDirectorProfiler.End();
}

PPC_FUNC_IMPL(__imp__sub_824EB290);
PPC_FUNC(sub_824EB290)
{
    g_renderDirectorProfiler.Begin();
    __imp__sub_824EB290(ctx, base);
    g_renderDirectorProfiler.End();
}

// World map disables VERT+, so scaling by width does not work for it.
static uint32_t g_forceCheckHeightForPostProcessFix;

// SWA::CWorldMapCamera::CWorldMapCamera
PPC_FUNC_IMPL(__imp__sub_824860E0);
PPC_FUNC(sub_824860E0)
{
    ++g_forceCheckHeightForPostProcessFix;
    __imp__sub_824860E0(ctx, base);
}

// SWA::CCameraController::~CCameraController
PPC_FUNC_IMPL(__imp__sub_824831D0);
PPC_FUNC(sub_824831D0)
{
    if (PPC_LOAD_U32(ctx.r3.u32) == 0x8202BF1C) // SWA::CWorldMapCamera
        --g_forceCheckHeightForPostProcessFix;

    __imp__sub_824831D0(ctx, base);
}

void PostProcessResolutionFix(PPCRegister& r4, PPCRegister& f1, PPCRegister& f2)
{
    auto device = reinterpret_cast<be<uint32_t>*>(g_memory.Translate(r4.u32));

    uint32_t width = device[46].get();
    uint32_t height = device[47].get();
    double aspectRatio = double(width) / double(height);

    double factor;
    if ((aspectRatio >= WIDE_ASPECT_RATIO) || (g_forceCheckHeightForPostProcessFix != 0))
        factor = 720.0 / double(height);
    else
        factor = 1280.0 / double(width);

    f1.f64 *= factor;
    f2.f64 *= factor;
}

void LightShaftAspectRatioFix(PPCRegister& f28, PPCRegister& f0)
{
    f28.f64 = f0.f64;
}

static const be<uint16_t> g_particleTestIndexBuffer[] =
{
    0, 1, 2,
    0, 2, 3,
    0, 3, 4,
    0, 4, 5
};

bool ParticleTestIndexBufferMidAsmHook(PPCRegister& r30)
{
    if (!g_capabilities.triangleFan)
    {
        auto buffer = CreateIndexBuffer(sizeof(g_particleTestIndexBuffer), 0, D3DFMT_INDEX16);
        void* memory = LockIndexBuffer(buffer, 0, 0, 0);
        memcpy(memory, g_particleTestIndexBuffer, sizeof(g_particleTestIndexBuffer));
        UnlockIndexBuffer(buffer);

        r30.u32 = g_memory.MapVirtual(buffer);
        return true;
    }
    return false;
}

void ParticleTestDrawIndexedPrimitiveMidAsmHook(PPCRegister& r7)
{
    if (!g_capabilities.triangleFan)
        r7.u64 = std::size(g_particleTestIndexBuffer);
}

void MotionBlurPrevInvViewProjectionMidAsmHook(PPCRegister& r10)
{
    auto mtxProjection = reinterpret_cast<be<float>*>(g_memory.Translate(r10.u32));

    // Reverse Z. Have to be done on CPU side because the matrix multiplications
    // add up and it loses precision by the time it's sent to GPU.
    mtxProjection[10] = -(mtxProjection[10] + 1.0f);
    mtxProjection[14] = -mtxProjection[14];
}

// Normally, we could delay setting IsMadeOne, but the game relies on that flag
// being present to handle load priority. To work around that, we can prevent
// IsMadeAll from being set until the compilation is finished. Time for a custom flag!
enum
{
    eDatabaseDataFlags_CompilingPipelines = 0x80
};

// This is passed to pipeline compilation threads to keep the loading screen busy until 
// all of them are finished. A shared pointer makes sure the destructor is called only once.
struct PipelineTaskToken
{
    PipelineTaskType type{};
    boost::shared_ptr<Hedgehog::Database::CDatabaseData> databaseData;

    PipelineTaskToken() : databaseData()
    {
    }

    PipelineTaskToken(const PipelineTaskToken&) = delete;

    PipelineTaskToken(PipelineTaskToken&& other)
        : type(std::exchange(other.type, PipelineTaskType::Null))
        , databaseData(std::exchange(other.databaseData, nullptr))
    {
    }

    ~PipelineTaskToken()
    {
        if (type != PipelineTaskType::Null)
        {
            if (databaseData.get() != nullptr)
                databaseData->m_Flags &= ~eDatabaseDataFlags_CompilingPipelines;

            if ((--g_compilingPipelineTaskCount) == 0)
                g_compilingPipelineTaskCount.notify_one();
        }
    }
};

struct PipelineStateQueueItem
{
    XXH64_hash_t pipelineHash;
    PipelineState pipelineState;
    std::shared_ptr<PipelineTaskToken> token;
#ifdef ASYNC_PSO_DEBUG
    std::string pipelineName;
#endif
};

static moodycamel::BlockingConcurrentQueue<PipelineStateQueueItem> g_pipelineStateQueue;

static void CompilePipeline(XXH64_hash_t pipelineHash, const PipelineState& pipelineState
#ifdef ASYNC_PSO_DEBUG
    , const std::string& pipelineName
#endif
)
{
    auto pipeline = CreateGraphicsPipeline(pipelineState);
#ifdef ASYNC_PSO_DEBUG
    pipeline->setName(pipelineName);
#endif

    // Will get dropped in render thread if a different thread already managed to compile this.
    RenderCommand cmd;
    cmd.type = RenderCommandType::AddPipeline;
    cmd.addPipeline.hash = pipelineHash;
    cmd.addPipeline.pipeline = pipeline.release();
    g_renderQueue.enqueue(cmd);
}

static void PipelineCompilerThread()
{
#ifdef _WIN32
    int threadPriority = THREAD_PRIORITY_LOWEST;
    SetThreadPriority(GetCurrentThread(), threadPriority);
    GuestThread::SetThreadName(GetCurrentThreadId(), "Pipeline Compiler Thread");
#endif

    std::unique_ptr<GuestThreadContext> ctx;

    while (true)
    {
        PipelineStateQueueItem queueItem;
        g_pipelineStateQueue.wait_dequeue(queueItem);

        if (ctx == nullptr)
            ctx = std::make_unique<GuestThreadContext>(0);

#ifdef _WIN32
        int newThreadPriority = threadPriority;

        bool loading = *SWA::SGlobals::ms_IsLoading;
        if (loading)
            newThreadPriority = THREAD_PRIORITY_HIGHEST;
        else
            newThreadPriority = THREAD_PRIORITY_LOWEST;

        if (newThreadPriority != threadPriority)
        {
            SetThreadPriority(GetCurrentThread(), newThreadPriority);
            threadPriority = newThreadPriority;
        }
#endif

        CompilePipeline(queueItem.pipelineHash, queueItem.pipelineState
#ifdef ASYNC_PSO_DEBUG
            , queueItem.pipelineName.c_str()
#endif
        );

        std::this_thread::yield();
    }
}

static std::vector<std::unique_ptr<std::thread>> g_pipelineCompilerThreads = []()
    {
        size_t threadCount = std::max(2u, (std::thread::hardware_concurrency() * 2) / 3);

        std::vector<std::unique_ptr<std::thread>> threads(threadCount);
        for (auto& thread : threads)
            thread = std::make_unique<std::thread>(PipelineCompilerThread);

        return threads;
    }();

static constexpr uint32_t MODEL_DATA_VFTABLE = 0x82073A44;
static constexpr uint32_t TERRAIN_MODEL_DATA_VFTABLE = 0x8211D25C;
static constexpr uint32_t PARTICLE_MATERIAL_VFTABLE = 0x8211F198;

// Allocate the shared pointer only when new compilations are happening.
// If nothing was compiled, the local "token" variable will get destructed with RAII instead.
struct PipelineTaskTokenPair
{
    PipelineTaskToken token;
    std::shared_ptr<PipelineTaskToken> sharedToken;
};

// Having this separate, because I don't want to lock a mutex in the render thread before
// every single draw. Might be worth profiling to see if it actually has an impact and merge them.
static xxHashMap<PipelineState> g_asyncPipelineStates;

static void EnqueueGraphicsPipelineCompilation(
    const PipelineState& pipelineState, 
    PipelineTaskTokenPair& tokenPair, 
    const char* name,
    bool isPrecompiledPipeline = false)
{
    XXH64_hash_t hash = XXH3_64bits(&pipelineState, sizeof(pipelineState));
    bool shouldCompile = g_asyncPipelineStates.emplace(hash, pipelineState).second;

    if (shouldCompile)
    {
        bool loading = *SWA::SGlobals::ms_IsLoading;
        if (!loading && isPrecompiledPipeline)
        {
            // We can just compile here during the logos.
            CompilePipeline(hash, pipelineState
#ifdef ASYNC_PSO_DEBUG
                , fmt::format("CACHE {} {:X}", name, hash)
#endif
            );
        }
        else
        {
            if (tokenPair.sharedToken == nullptr && tokenPair.token.type != PipelineTaskType::Null)
                tokenPair.sharedToken = std::make_shared<PipelineTaskToken>(std::move(tokenPair.token));

            PipelineStateQueueItem queueItem;
            queueItem.pipelineHash = hash;
            queueItem.pipelineState = pipelineState;
            queueItem.token = tokenPair.sharedToken;
#ifdef ASYNC_PSO_DEBUG
            queueItem.pipelineName = fmt::format("ASYNC {} {:X}", name, hash);
#endif
            g_pipelineStateQueue.enqueue(queueItem);
        }
    }

#ifdef PSO_CACHING_CLEANUP
    if (shouldCompile && isPrecompiledPipeline)
    {
        std::lock_guard lock(g_pipelineCacheMutex);
        g_pipelineStatesToCache.emplace(hash, pipelineState);
    }
#endif

#ifdef PSO_CACHING
    if (!isPrecompiledPipeline)
    {
        std::lock_guard lock(g_pipelineCacheMutex);
        g_pipelineStatesToCache.erase(hash);
    }
#endif
}

struct CompilationArgs
{
    PipelineTaskTokenPair tokenPair;
    bool noGI{};
    bool hasMoreThanOneBone{};
    bool velocityMapQuickStep{};
    bool objectIcon{};
    bool instancing{};
};

enum class MeshLayer
{
    Opaque,
    Transparent,
    PunchThrough,
    Special
};

struct Mesh
{
    uint32_t vertexSize{};
    uint32_t morphTargetVertexSize{};
    GuestVertexDeclaration* vertexDeclaration{};
    Hedgehog::Mirage::CMaterialData* material{};
    MeshLayer layer{};
    bool morphModel{};
};

static void CompileMeshPipeline(const Mesh& mesh, CompilationArgs& args)
{
    if (mesh.material == nullptr || mesh.material->m_spShaderListData.get() == nullptr)
        return;

    auto& shaderList = mesh.material->m_spShaderListData;

    bool isFur = !mesh.morphModel && !args.instancing &&
        strstr(shaderList->m_TypeAndName.c_str(), "Fur") != nullptr;

    bool isSky = !mesh.morphModel && !args.instancing &&
        strstr(shaderList->m_TypeAndName.c_str(), "Sky") != nullptr;

    bool isSonicMouth = !mesh.morphModel && !args.instancing &&
        strcmp(mesh.material->m_TypeAndName.c_str() + 2, "sonic_gm_mouth_duble") == 0 &&
        strcmp(shaderList->m_TypeAndName.c_str() + 3, "SonicSkin_dspf[b]") == 0;

    bool compiledOutsideMainFramebuffer = !args.instancing && !isFur && !isSky;

    bool constTexCoord;
    if (args.instancing)
    {
        constTexCoord = false;
    }
    else
    {
        constTexCoord = true;
        if (mesh.material->m_spTexsetData.get() != nullptr)
        {
            for (size_t i = 1; i < mesh.material->m_spTexsetData->m_TextureList.size(); i++)
            {
                if (mesh.material->m_spTexsetData->m_TextureList[i]->m_TexcoordIndex !=
                    mesh.material->m_spTexsetData->m_TextureList[0]->m_TexcoordIndex)
                {
                    constTexCoord = false;
                    break;
                }
            }
        }
    }

    // Shadow pipeline.
    if (compiledOutsideMainFramebuffer && (mesh.layer == MeshLayer::Opaque || mesh.layer == MeshLayer::PunchThrough))
    {
        PipelineState pipelineState{};

        if (mesh.layer == MeshLayer::PunchThrough)
        {
            pipelineState.vertexShader = FindShaderCacheEntry(0xDD4FA7BB53876300)->guestShader;
            pipelineState.pixelShader = FindShaderCacheEntry(0xE2ECA594590DDE8B)->guestShader;
        }
        else
        {
            pipelineState.vertexShader = FindShaderCacheEntry(0x8E4BB23465BD909E)->guestShader;
        }

        pipelineState.vertexDeclaration = mesh.vertexDeclaration;
        pipelineState.cullMode = mesh.material->m_DoubleSided ? RenderCullMode::NONE : RenderCullMode::BACK;
        pipelineState.zFunc = RenderComparisonFunction::LESS_EQUAL;
        
        if (g_capabilities.dynamicDepthBias)
        {
            // Put common depth bias values for reducing unnecessary calls.
            if (!g_vulkan)
            {
                pipelineState.depthBias = COMMON_DEPTH_BIAS_VALUE;
                pipelineState.slopeScaledDepthBias = COMMON_SLOPE_SCALED_DEPTH_BIAS_VALUE;
            }
        }
        else 
        {
            pipelineState.depthBias = (1 << 24) * (*reinterpret_cast<be<float>*>(g_memory.Translate(0x83302760)));
            pipelineState.slopeScaledDepthBias = *reinterpret_cast<be<float>*>(g_memory.Translate(0x83302764));
        }

        pipelineState.colorWriteEnable = 0;
        pipelineState.primitiveTopology = RenderPrimitiveTopology::TRIANGLE_STRIP;
        pipelineState.vertexStrides[0] = mesh.vertexSize;
        pipelineState.depthStencilFormat = RenderFormat::D32_FLOAT;

        if (mesh.layer == MeshLayer::PunchThrough)
            pipelineState.specConstants |= SPEC_CONSTANT_ALPHA_TEST;

        const char* name = (mesh.layer == MeshLayer::PunchThrough ? "MakeShadowMapTransparent" : "MakeShadowMap");
        SanitizePipelineState(pipelineState);
        EnqueueGraphicsPipelineCompilation(pipelineState, args.tokenPair, name);

        // Morph models have 4 targets where unused targets default to the first vertex stream.
        if (mesh.morphModel)
        {
            for (size_t i = 0; i < 5; i++)
            {
                for (size_t j = 0; j < 4; j++)
                    pipelineState.vertexStrides[j + 1] = i > j ? mesh.morphTargetVertexSize : mesh.vertexSize;

                SanitizePipelineState(pipelineState);
                EnqueueGraphicsPipelineCompilation(pipelineState, args.tokenPair, name);
            }
        }
    }

    // Motion blur pipeline. We could normally do the player here only, but apparently Werehog enemies also have object blur.
    // TODO: Do punch through meshes get rendered?
    if (!mesh.morphModel && compiledOutsideMainFramebuffer && args.hasMoreThanOneBone && mesh.layer == MeshLayer::Opaque)
    {
        PipelineState pipelineState{};
        pipelineState.vertexShader = FindShaderCacheEntry(0x4620B236DC38100C)->guestShader;
        pipelineState.pixelShader = FindShaderCacheEntry(0xBBDB735BEACC8F41)->guestShader;
        pipelineState.vertexDeclaration = mesh.vertexDeclaration;
        pipelineState.cullMode = RenderCullMode::NONE;
        pipelineState.zFunc = RenderComparisonFunction::GREATER_EQUAL;
        pipelineState.primitiveTopology = RenderPrimitiveTopology::TRIANGLE_STRIP;
        pipelineState.vertexStrides[0] = mesh.vertexSize;
        pipelineState.renderTargetFormat = RenderFormat::R8G8B8A8_UNORM;
        pipelineState.depthStencilFormat = RenderFormat::D32_FLOAT;
        pipelineState.specConstants = SPEC_CONSTANT_REVERSE_Z;

        SanitizePipelineState(pipelineState);
        EnqueueGraphicsPipelineCompilation(pipelineState, args.tokenPair, "FxVelocityMap");

        if (args.velocityMapQuickStep)
        {
            pipelineState.vertexShader = FindShaderCacheEntry(0x99DC3F27E402700D)->guestShader;
            SanitizePipelineState(pipelineState);
            EnqueueGraphicsPipelineCompilation(pipelineState, args.tokenPair, "FxVelocityMapQuickStep");
        }
    }

    uint32_t defaultStr = args.instancing ? 0x820C8734 : 0x8202DDBC; // "instancing" for instancing, "default" for regular
    guest_stack_var<Hedgehog::Base::CStringSymbol> defaultSymbol(reinterpret_cast<const char*>(g_memory.Translate(defaultStr)));
    auto defaultFindResult = shaderList->m_PixelShaderPermutations.find(*defaultSymbol);
    if (defaultFindResult == shaderList->m_PixelShaderPermutations.end())
        return;

    uint32_t pixelShaderSubPermutationsToCompile = 0;
    if (constTexCoord) pixelShaderSubPermutationsToCompile |= 0x1;
    if (args.noGI) pixelShaderSubPermutationsToCompile |= 0x2;

    if ((defaultFindResult->second.m_SubPermutations.get() & (1 << pixelShaderSubPermutationsToCompile)) == 0) pixelShaderSubPermutationsToCompile &= ~0x1;
    if ((defaultFindResult->second.m_SubPermutations.get() & (1 << pixelShaderSubPermutationsToCompile)) == 0) pixelShaderSubPermutationsToCompile &= ~0x2;

    uint32_t noneStr = mesh.morphModel ? 0x820D72F0 : 0x8200D938; // "p" for morph, "none" for regular
    guest_stack_var<Hedgehog::Base::CStringSymbol> noneSymbol(reinterpret_cast<const char*>(g_memory.Translate(noneStr)));
    auto noneFindResult = defaultFindResult->second.m_VertexShaderPermutations.find(*noneSymbol);
    if (noneFindResult == defaultFindResult->second.m_VertexShaderPermutations.end())
        return;

    uint32_t vertexShaderSubPermutationsToCompile = 0;
    if (constTexCoord) vertexShaderSubPermutationsToCompile |= 0x1;

    if ((noneFindResult->second->m_SubPermutations.get() & (1 << vertexShaderSubPermutationsToCompile)) == 0)
        vertexShaderSubPermutationsToCompile &= ~0x1;

    auto vertexDeclaration = mesh.vertexDeclaration;
    bool instancing = args.instancing || isFur;

    if (instancing)
    {
        GuestVertexElement vertexElements[64];
        memcpy(vertexElements, mesh.vertexDeclaration->vertexElements.get(), (mesh.vertexDeclaration->vertexElementCount - 1) * sizeof(GuestVertexElement));

        if (args.instancing)
        {
            vertexElements[mesh.vertexDeclaration->vertexElementCount - 1] = { 1, 0, 0x2A23B9, 0, 5, 4 };
            vertexElements[mesh.vertexDeclaration->vertexElementCount] = { 1, 12, 0x2C2159, 0, 5, 5 };
            vertexElements[mesh.vertexDeclaration->vertexElementCount + 1] = { 1, 16, 0x2C2159, 0, 5, 6 };
            vertexElements[mesh.vertexDeclaration->vertexElementCount + 2] = { 1, 20, 0x182886, 0, 10, 1 };
            vertexElements[mesh.vertexDeclaration->vertexElementCount + 3] = { 2, 0, 0x2C82A1, 0, 0, 1 };
            vertexElements[mesh.vertexDeclaration->vertexElementCount + 4] = D3DDECL_END();
        }
        else if (isFur)
        {
            vertexElements[mesh.vertexDeclaration->vertexElementCount - 1] = { 1, 0, 0x2C82A1, 0, 0, 1 };
            vertexElements[mesh.vertexDeclaration->vertexElementCount] = { 2, 0, 0x2C83A4, 0, 0, 2 };
            vertexElements[mesh.vertexDeclaration->vertexElementCount + 1] = D3DDECL_END();
        }

        vertexDeclaration = CreateVertexDeclarationWithoutAddRef(vertexElements);
    }

    for (auto& [pixelShaderSubPermutations, pixelShader] : defaultFindResult->second.m_PixelShaders)
    {
        if (pixelShader.get() == nullptr || (pixelShaderSubPermutations & 0x3) != pixelShaderSubPermutationsToCompile)
            continue;

        for (auto& [vertexShaderSubPermutations, vertexShader] : noneFindResult->second->m_VertexShaders)
        {
            if (vertexShader.get() == nullptr || (vertexShaderSubPermutations & 0x1) != vertexShaderSubPermutationsToCompile)
                continue;

            PipelineState pipelineState{};
            pipelineState.vertexShader = reinterpret_cast<GuestShader*>(vertexShader->m_spCode->m_pD3DVertexShader.get());
            pipelineState.pixelShader = reinterpret_cast<GuestShader*>(pixelShader->m_spCode->m_pD3DPixelShader.get());
            pipelineState.vertexDeclaration = vertexDeclaration;
            pipelineState.instancing = instancing;
            pipelineState.zWriteEnable = !isSky && mesh.layer != MeshLayer::Transparent;
            pipelineState.srcBlend = RenderBlend::SRC_ALPHA;
            pipelineState.destBlend = mesh.material->m_Additive ? RenderBlend::ONE : RenderBlend::INV_SRC_ALPHA;
            pipelineState.cullMode = mesh.material->m_DoubleSided ? RenderCullMode::NONE : RenderCullMode::BACK;
            pipelineState.zFunc = RenderComparisonFunction::GREATER_EQUAL; // Reverse Z
            pipelineState.alphaBlendEnable = mesh.layer == MeshLayer::Transparent || mesh.layer == MeshLayer::Special;
            pipelineState.srcBlendAlpha = RenderBlend::SRC_ALPHA;
            pipelineState.destBlendAlpha = RenderBlend::INV_SRC_ALPHA;
            pipelineState.primitiveTopology = RenderPrimitiveTopology::TRIANGLE_STRIP;
            pipelineState.vertexStrides[0] = mesh.vertexSize;

            if (args.instancing)
            {
                pipelineState.vertexStrides[1] = 24;
                pipelineState.vertexStrides[2] = 4;
            }
            else if (isFur)
            {
                pipelineState.vertexStrides[1] = 4;
                pipelineState.vertexStrides[2] = 4;
            }

            pipelineState.renderTargetFormat = RenderFormat::R16G16B16A16_FLOAT;
            pipelineState.depthStencilFormat = RenderFormat::D32_FLOAT;
            pipelineState.sampleCount = Config::AntiAliasing != EAntiAliasing::None ? int32_t(Config::AntiAliasing.Value) : 1;

            if (pipelineState.vertexDeclaration->hasR11G11B10Normal)
                pipelineState.specConstants |= SPEC_CONSTANT_R11G11B10_NORMAL;

            if (Config::GITextureFiltering == EGITextureFiltering::Bicubic)
                pipelineState.specConstants |= SPEC_CONSTANT_BICUBIC_GI_FILTER;

            if (mesh.layer == MeshLayer::PunchThrough)
            {
                if (Config::AntiAliasing != EAntiAliasing::None && Config::TransparencyAntiAliasing)
                {
                    pipelineState.enableAlphaToCoverage = true;
                    pipelineState.specConstants |= SPEC_CONSTANT_ALPHA_TO_COVERAGE;
                }
                else
                {
                    pipelineState.specConstants |= SPEC_CONSTANT_ALPHA_TEST;
                }
            }

            if (!isSky)
                pipelineState.specConstants |= SPEC_CONSTANT_REVERSE_Z;

            auto createGraphicsPipeline = [&](PipelineState& pipelineStateToCreate)
                {
                    SanitizePipelineState(pipelineStateToCreate);
                    EnqueueGraphicsPipelineCompilation(pipelineStateToCreate, args.tokenPair, shaderList->m_TypeAndName.c_str() + 3);

                    // Morph models have 4 targets where unused targets default to the first vertex stream.
                    if (mesh.morphModel)
                    {
                        for (size_t i = 0; i < 5; i++)
                        {
                            for (size_t j = 0; j < 4; j++)
                                pipelineStateToCreate.vertexStrides[j + 1] = i > j ? mesh.morphTargetVertexSize : mesh.vertexSize;

                            SanitizePipelineState(pipelineStateToCreate);
                            EnqueueGraphicsPipelineCompilation(pipelineStateToCreate, args.tokenPair, shaderList->m_TypeAndName.c_str() + 3);
                        }
                    }
                };

            createGraphicsPipeline(pipelineState);

            // We cannot rely on this being accurate during loading as SceneEffect.prm.xml gets loaded a bit later.
            bool planarReflectionEnabled = *reinterpret_cast<bool*>(g_memory.Translate(0x832FA0D8));
            bool loading = *SWA::SGlobals::ms_IsLoading;
            bool compileNoMsaaPipeline = pipelineState.sampleCount != 1 && (loading || planarReflectionEnabled);

            auto noMsaaPipeline = pipelineState;
            noMsaaPipeline.sampleCount = 1;
            noMsaaPipeline.enableAlphaToCoverage = false;

            if ((noMsaaPipeline.specConstants & SPEC_CONSTANT_ALPHA_TO_COVERAGE) != 0)
            {
                noMsaaPipeline.specConstants &= ~SPEC_CONSTANT_ALPHA_TO_COVERAGE;
                noMsaaPipeline.specConstants |= SPEC_CONSTANT_ALPHA_TEST;
            }

            if (compileNoMsaaPipeline)
            {
                // Planar reflections don't use MSAA.
                createGraphicsPipeline(noMsaaPipeline);
            }

            if (args.objectIcon) 
            {
                // Object icons get rendered to a SDR buffer without MSAA.
                auto iconPipelineState = noMsaaPipeline;
                iconPipelineState.renderTargetFormat = RenderFormat::R8G8B8A8_UNORM;
                createGraphicsPipeline(iconPipelineState);
            }

            if (isSonicMouth)
            {
                // Sonic's mouth switches between "SonicSkin_dspf[b]" or "SonicSkinNodeInvX_dspf[b]" depending on the view angle.
                auto mouthPipelineState = pipelineState;
                mouthPipelineState.vertexShader = FindShaderCacheEntry(0x689AA3140AB9EBAA)->guestShader;
                createGraphicsPipeline(mouthPipelineState);

                if (compileNoMsaaPipeline)
                {
                    auto noMsaaMouthPipelineState = noMsaaPipeline;
                    noMsaaMouthPipelineState.vertexShader = mouthPipelineState.vertexShader;
                    createGraphicsPipeline(noMsaaMouthPipelineState);
                }
            }
        }
    }
}

static void CompileMeshPipeline(Hedgehog::Mirage::CMeshData* mesh, MeshLayer layer, CompilationArgs& args)
{
    CompileMeshPipeline(Mesh
        {
            mesh->m_VertexSize,
            0,
            reinterpret_cast<GuestVertexDeclaration*>(mesh->m_VertexDeclarationPtr.m_pD3DVertexDeclaration.get()),
            mesh->m_spMaterial.get(),
            layer,
            false
        }, args);
}

static void CompileMeshPipeline(Hedgehog::Mirage::CMorphModelData* morphModel, Hedgehog::Mirage::CMeshIndexData* mesh, MeshLayer layer, CompilationArgs& args)
{
    CompileMeshPipeline(Mesh
        {
            morphModel->m_VertexSize,
            morphModel->m_MorphTargetVertexSize,
            reinterpret_cast<GuestVertexDeclaration*>(morphModel->m_VertexDeclarationPtr.m_pD3DVertexDeclaration.get()),
            mesh->m_spMaterial.get(),
            layer,
            true
        }, args);
}

template<typename T>
static void CompileMeshPipelines(const T& modelData, CompilationArgs& args)
{
    for (auto& meshGroup : modelData.m_NodeGroupModels)
    {
        for (auto& mesh : meshGroup->m_OpaqueMeshes)
        {
            CompileMeshPipeline(mesh.get(), MeshLayer::Opaque, args);

            if (args.noGI) // For models that can be shown transparent (eg. medals)
                CompileMeshPipeline(mesh.get(), MeshLayer::Transparent, args);
        }

        for (auto& mesh : meshGroup->m_TransparentMeshes)
            CompileMeshPipeline(mesh.get(), MeshLayer::Transparent, args);

        for (auto& mesh : meshGroup->m_PunchThroughMeshes)
            CompileMeshPipeline(mesh.get(), MeshLayer::PunchThrough, args);

        for (auto& specialMeshGroup : meshGroup->m_SpecialMeshGroups)
        {
            for (auto& mesh : specialMeshGroup)
                CompileMeshPipeline(mesh.get(), MeshLayer::Special, args); // TODO: Are there layer types other than water in this game??
        }
    }

    for (auto& mesh : modelData.m_OpaqueMeshes)
    {
        CompileMeshPipeline(mesh.get(), MeshLayer::Opaque, args);

        if (args.noGI)
            CompileMeshPipeline(mesh.get(), MeshLayer::Transparent, args);
    }

    for (auto& mesh : modelData.m_TransparentMeshes)
        CompileMeshPipeline(mesh.get(), MeshLayer::Transparent, args);

    for (auto& mesh : modelData.m_PunchThroughMeshes)
        CompileMeshPipeline(mesh.get(), MeshLayer::PunchThrough, args);

    if constexpr (std::is_same_v<T, Hedgehog::Mirage::CModelData>)
    {
        for (auto& morphModel : modelData.m_MorphModels)
        {
            for (auto& mesh : morphModel->m_OpaqueMeshList)
                CompileMeshPipeline(morphModel.get(), mesh.get(), MeshLayer::Opaque, args);

            for (auto& mesh : morphModel->m_TransparentMeshList)
                CompileMeshPipeline(morphModel.get(), mesh.get(), MeshLayer::Transparent, args);

            for (auto& mesh : morphModel->m_PunchThroughMeshList)
                CompileMeshPipeline(morphModel.get(), mesh.get(), MeshLayer::PunchThrough, args);
        }
    }
}

static void CompileParticleMaterialPipeline(const Hedgehog::Sparkle::CParticleMaterial& material, PipelineTaskTokenPair& tokenPair)
{
    auto& shaderList = material.m_spShaderListData;
    if (shaderList.get() == nullptr)
        return;

    guest_stack_var<Hedgehog::Base::CStringSymbol> defaultSymbol(reinterpret_cast<const char*>(g_memory.Translate(0x8202DDBC)));
    auto defaultFindResult = shaderList->m_PixelShaderPermutations.find(*defaultSymbol);
    if (defaultFindResult == shaderList->m_PixelShaderPermutations.end())
        return;

    guest_stack_var<Hedgehog::Base::CStringSymbol> noneSymbol(reinterpret_cast<const char*>(g_memory.Translate(0x8200D938)));
    auto noneFindResult = defaultFindResult->second.m_VertexShaderPermutations.find(*noneSymbol);
    if (noneFindResult == defaultFindResult->second.m_VertexShaderPermutations.end())
        return;

    // All the particle models in the game come with the unoptimized format, so we can assume it.
    uint8_t unoptimizedVertexElements[144] = 
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x23, 0xB9, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x0C, 0x00, 0x2A, 0x23, 0xB9, 0x00, 0x03, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x18, 0x00, 0x2A, 0x23, 0xB9, 0x00, 0x06, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x24, 0x00, 0x2A, 0x23, 0xB9, 0x00, 0x07, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x30, 0x00, 0x2C, 0x23, 0xA5, 0x00, 0x05, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x38, 0x00, 0x2C, 0x23, 0xA5, 0x00, 0x05, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x40, 0x00, 0x2C, 0x23, 0xA5, 0x00, 0x05, 0x02, 0x00,
        0x00, 0x00, 0x00, 0x48, 0x00, 0x2C, 0x23, 0xA5, 0x00, 0x05, 0x03, 0x00,
        0x00, 0x00, 0x00, 0x50, 0x00, 0x1A, 0x23, 0xA6, 0x00, 0x0A, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x60, 0x00, 0x1A, 0x23, 0x86, 0x00, 0x02, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x64, 0x00, 0x1A, 0x20, 0x86, 0x00, 0x01, 0x00, 0x00,
        0x00, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00
    };

    auto unoptimizedVertexDeclaration = CreateVertexDeclarationWithoutAddRef(reinterpret_cast<GuestVertexElement*>(unoptimizedVertexElements));
    auto sparkleVertexDeclaration = CreateVertexDeclarationWithoutAddRef(reinterpret_cast<GuestVertexElement*>(g_memory.Translate(0x8211F540)));

    bool isMeshShader = strstr(shaderList->m_TypeAndName.c_str(), "Mesh") != nullptr;

    PipelineState pipelineState{};
    pipelineState.vertexShader = reinterpret_cast<GuestShader*>(noneFindResult->second->m_VertexShaders.begin()->second->m_spCode->m_pD3DVertexShader.get());
    pipelineState.pixelShader = reinterpret_cast<GuestShader*>(defaultFindResult->second.m_PixelShaders.begin()->second->m_spCode->m_pD3DPixelShader.get());
    pipelineState.vertexDeclaration = isMeshShader ? unoptimizedVertexDeclaration : sparkleVertexDeclaration;
    pipelineState.zWriteEnable = false;
    pipelineState.zFunc = RenderComparisonFunction::GREATER_EQUAL;
    pipelineState.alphaBlendEnable = true;
    pipelineState.srcBlendAlpha = RenderBlend::SRC_ALPHA;
    pipelineState.destBlendAlpha = RenderBlend::INV_SRC_ALPHA;
    pipelineState.primitiveTopology = RenderPrimitiveTopology::TRIANGLE_STRIP;
    pipelineState.vertexStrides[0] = isMeshShader ? 104 : 28;
    pipelineState.depthStencilFormat = RenderFormat::D32_FLOAT;
    pipelineState.specConstants = SPEC_CONSTANT_REVERSE_Z;

    if (pipelineState.vertexDeclaration->hasR11G11B10Normal)
        pipelineState.specConstants |= SPEC_CONSTANT_R11G11B10_NORMAL;

    switch (material.m_BlendMode.get())
    {
    case Hedgehog::Sparkle::CParticleMaterial::eBlendMode_Zero:
        pipelineState.srcBlend = RenderBlend::ZERO;
        pipelineState.destBlend = RenderBlend::ZERO;
        break;
    case Hedgehog::Sparkle::CParticleMaterial::eBlendMode_Typical:
        pipelineState.srcBlend = RenderBlend::SRC_ALPHA;
        pipelineState.destBlend = RenderBlend::INV_SRC_ALPHA;
        break;
    case Hedgehog::Sparkle::CParticleMaterial::eBlendMode_Add:
        pipelineState.srcBlend = RenderBlend::SRC_ALPHA;
        pipelineState.destBlend = RenderBlend::ONE;
        break;
    default:
        pipelineState.srcBlend = RenderBlend::ONE;
        pipelineState.destBlend = RenderBlend::ONE;
        break;
    }

    auto createGraphicsPipeline = [&](PipelineState& pipelineStateToCreate)
        {
            SanitizePipelineState(pipelineStateToCreate);
            EnqueueGraphicsPipelineCompilation(pipelineStateToCreate, tokenPair, shaderList->m_TypeAndName.c_str() + 3);
        };

    // Mesh particles can use both cull modes. Quad particles are only NONE.
    RenderCullMode cullModes[] = { RenderCullMode::NONE, RenderCullMode::BACK };
    uint32_t cullModeCount = isMeshShader ? std::size(cullModes) : 1;
    RenderFormat renderTargetFormats[] = { RenderFormat::R16G16B16A16_FLOAT, RenderFormat::R8G8B8A8_UNORM };

    for (size_t i = 0; i < cullModeCount; i++)
    {
        pipelineState.cullMode = cullModes[i];

        for (auto renderTargetFormat : renderTargetFormats)
        {
            pipelineState.renderTargetFormat = renderTargetFormat;

            if (renderTargetFormat == RenderFormat::R16G16B16A16_FLOAT)
                pipelineState.sampleCount = Config::AntiAliasing != EAntiAliasing::None ? int32_t(Config::AntiAliasing.Value) : 1;
            else
                pipelineState.sampleCount = 1;

            createGraphicsPipeline(pipelineState);

            // Always compile no MSAA variant for particles, as the planar
            // reflection variable isn't reliable at this time of compilation.
            bool compileNoMsaaPipeline = pipelineState.sampleCount != 1;

            auto noMsaaPipelineState = pipelineState;
            noMsaaPipelineState.sampleCount = 1;

            if (compileNoMsaaPipeline)
                createGraphicsPipeline(noMsaaPipelineState);

            if (!isMeshShader)
            {
                // Previous compilation was for locus particles. This one will be for quads.
                auto quadPipelineState = pipelineState;
                quadPipelineState.primitiveTopology = RenderPrimitiveTopology::TRIANGLE_LIST;
                createGraphicsPipeline(quadPipelineState);

                if (compileNoMsaaPipeline)
                {
                    auto noMsaaQuadPipelineState = noMsaaPipelineState;
                    noMsaaQuadPipelineState.primitiveTopology = RenderPrimitiveTopology::TRIANGLE_LIST;
                    createGraphicsPipeline(noMsaaQuadPipelineState);
                }
            }
        }
    }
}

static std::thread::id g_mainThreadId = std::this_thread::get_id();

// SWA::CGameModeStage::ExitLoading
PPC_FUNC_IMPL(__imp__sub_825369A0);
PPC_FUNC(sub_825369A0)
{
    assert(std::this_thread::get_id() == g_mainThreadId);

    // Wait for pipeline compilations to finish.
    uint32_t value;
    while ((value = g_compilingPipelineTaskCount.load()) != 0)
    {
        // Pump SDL events to prevent the OS
        // from thinking the process is unresponsive.
        SDL_PumpEvents();
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

        g_compilingPipelineTaskCount.wait(value);
    }

    __imp__sub_825369A0(ctx, base);
}

// CModelData::CheckMadeAll
PPC_FUNC_IMPL(__imp__sub_82E2EFB0);
PPC_FUNC(sub_82E2EFB0)
{   
    if (reinterpret_cast<Hedgehog::Database::CDatabaseData*>(base + ctx.r3.u32)->m_Flags & eDatabaseDataFlags_CompilingPipelines)
    {
        ctx.r3.u64 = 0;
    }
    else
    {
        __imp__sub_82E2EFB0(ctx, base);
    }
}

// CTerrainModelData::CheckMadeAll
PPC_FUNC_IMPL(__imp__sub_82E243D8);
PPC_FUNC(sub_82E243D8)
{   
    if (reinterpret_cast<Hedgehog::Database::CDatabaseData*>(base + ctx.r3.u32)->m_Flags & eDatabaseDataFlags_CompilingPipelines)
    {
        ctx.r3.u64 = 0;
    }
    else
    {
        __imp__sub_82E243D8(ctx, base);
    }
}

// CParticleMaterial::CheckMadeAll
PPC_FUNC_IMPL(__imp__sub_82E87598);
PPC_FUNC(sub_82E87598)
{   
    if (reinterpret_cast<Hedgehog::Database::CDatabaseData*>(base + ctx.r3.u32)->m_Flags & eDatabaseDataFlags_CompilingPipelines)
    {
        ctx.r3.u64 = 0;
    }
    else
    {
        __imp__sub_82E87598(ctx, base);
    }
}

void GetDatabaseDataMidAsmHook(PPCRegister& r1, PPCRegister& r4)
{
    auto& databaseData = *reinterpret_cast<boost::shared_ptr<Hedgehog::Database::CDatabaseData>*>(
        g_memory.Translate(r1.u32 + 0x58));

    if (!databaseData->IsMadeOne() && r4.u32 != NULL)
    {
        if (databaseData->m_pVftable.ptr == MODEL_DATA_VFTABLE)
        {
            // Ignore particle models, the materials they point at don't actually
            // get used and give the threads unnecessary work.
            bool isParticleModel = *reinterpret_cast<be<uint32_t>*>(g_memory.Translate(r4.u32 + 4)) != 5 &&
                strncmp(databaseData->m_TypeAndName.c_str() + 2, "eff_", 4) == 0;

            if (isParticleModel)
                return;

            // Adabat water is broken in original game, which they tried to fix by partially including the files in the update,
            // which then finally fixed for real in the DLC. This confuses the async PSO compiler and causes a hang if the DLC is missing.
            // We'll just ignore it.
            bool isAdabatWater = strcmp(databaseData->m_TypeAndName.c_str() + 2, "evl_sea_obj_st_waterCircle") == 0;
            if (isAdabatWater)
                return;
        }

        databaseData->m_Flags |= eDatabaseDataFlags_CompilingPipelines;
        EnqueuePipelineTask(PipelineTaskType::DatabaseData, databaseData);
    }
}

static bool CheckMadeAll(Hedgehog::Mirage::CMeshData* meshData)
{
    if (!meshData->IsMadeOne())
        return false;

    if (meshData->m_spMaterial.get() != nullptr)
    {
        if (!meshData->m_spMaterial->IsMadeOne())
            return false;

        if (meshData->m_spMaterial->m_spTexsetData.get() != nullptr)
        {
            if (!meshData->m_spMaterial->m_spTexsetData->IsMadeOne())
                return false;

            for (auto& texture : meshData->m_spMaterial->m_spTexsetData->m_TextureList)
            {
                if (!texture->IsMadeOne())
                    return false;
            }
        }
    }

    return true;
}

template<typename T>
static bool CheckMadeAll(const T& modelData)
{
    if (!modelData.IsMadeOne())
        return false;

    for (auto& meshGroup : modelData.m_NodeGroupModels)
    {
        for (auto& mesh : meshGroup->m_OpaqueMeshes)
        {
            if (!CheckMadeAll(mesh.get()))
                return false;
        }     

        for (auto& mesh : meshGroup->m_TransparentMeshes)
        {
            if (!CheckMadeAll(mesh.get()))
                return false;
        }    

        for (auto& mesh : meshGroup->m_PunchThroughMeshes)
        {
            if (!CheckMadeAll(mesh.get()))
                return false;
        }

        for (auto& specialMeshGroup : meshGroup->m_SpecialMeshGroups)
        {
            for (auto& mesh : specialMeshGroup)
            {
                if (!CheckMadeAll(mesh.get()))
                    return false;
            }
        }
    }

    for (auto& mesh : modelData.m_OpaqueMeshes)
    {
        if (!CheckMadeAll(mesh.get()))
            return false;
    }

    for (auto& mesh : modelData.m_TransparentMeshes)
    {
        if (!CheckMadeAll(mesh.get()))
            return false;
    }

    for (auto& mesh : modelData.m_PunchThroughMeshes)
    {
        if (!CheckMadeAll(mesh.get()))
            return false;
    }

    return true;
}

static void PipelineTaskConsumerThread()
{
#ifdef _WIN32
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
    GuestThread::SetThreadName(GetCurrentThreadId(), "Pipeline Task Consumer Thread");
#endif

    std::vector<PipelineTask> localPipelineTaskQueue;
    std::unique_ptr<GuestThreadContext> ctx;

    while (true)
    {
        // Wait for tasks to arrive.
        uint32_t pendingPipelineTaskCount;
        while ((pendingPipelineTaskCount = g_pendingPipelineTaskCount.load()) == 0)
            g_pendingPipelineTaskCount.wait(pendingPipelineTaskCount);

        if (ctx == nullptr)
            ctx = std::make_unique<GuestThreadContext>(0);

        {
            std::lock_guard lock(g_pipelineTaskMutex);
            localPipelineTaskQueue.insert(localPipelineTaskQueue.end(), g_pipelineTaskQueue.begin(), g_pipelineTaskQueue.end());
            g_pipelineTaskQueue.clear();
        }

        bool allHandled = true;

        for (auto& [type, databaseData] : localPipelineTaskQueue)
        {
            switch (type)
            {
            case PipelineTaskType::DatabaseData:
            {
                bool ready = false;

                if (databaseData->m_pVftable.ptr == MODEL_DATA_VFTABLE)
                    ready = CheckMadeAll(*reinterpret_cast<Hedgehog::Mirage::CModelData*>(databaseData.get()));
                else
                    ready = databaseData->IsMadeOne();

                if (ready || databaseData.unique())
                {
                    if (databaseData->m_pVftable.ptr == TERRAIN_MODEL_DATA_VFTABLE)
                    {
                        CompilationArgs args{};
                        args.tokenPair.token.type = type;
                        args.tokenPair.token.databaseData = databaseData;
                        args.instancing = strncmp(databaseData->m_TypeAndName.c_str() + 3, "ins", 3) == 0;
                        CompileMeshPipelines(*reinterpret_cast<Hedgehog::Mirage::CTerrainModelData*>(databaseData.get()), args);
                    }
                    else if (databaseData->m_pVftable.ptr == PARTICLE_MATERIAL_VFTABLE)
                    {
                        PipelineTaskTokenPair tokenPair;
                        tokenPair.token.type = type;
                        tokenPair.token.databaseData = databaseData;
                        CompileParticleMaterialPipeline(*reinterpret_cast<Hedgehog::Sparkle::CParticleMaterial*>(databaseData.get()), tokenPair);
                    }
                    else
                    {
                        assert(databaseData->m_pVftable.ptr == MODEL_DATA_VFTABLE);

                        auto modelData = reinterpret_cast<Hedgehog::Mirage::CModelData*>(databaseData.get());

                        CompilationArgs args{};
                        args.tokenPair.token.type = type;
                        args.tokenPair.token.databaseData = databaseData;
                        args.noGI = true;
                        args.hasMoreThanOneBone = modelData->m_NodeNum > 1;
                        args.velocityMapQuickStep = strcmp(databaseData->m_TypeAndName.c_str() + 2, "SonicRoot") == 0;

                        // Check for the on screen items, eg. rings going to HUD.
                        auto items = reinterpret_cast<xpointer<const char>*>(g_memory.Translate(0x832A8DD0));
                        for (size_t i = 0; i < 50; i++)
                        {
                            if (strcmp(databaseData->m_TypeAndName.c_str() + 2, (*items).get()) == 0)
                            {
                                args.objectIcon = true;
                                break;
                            }
                            items += 7;
                        }

                        CompileMeshPipelines(*modelData, args);
                    }

                    type = PipelineTaskType::Null;
                    databaseData = nullptr;

                    --g_pendingPipelineTaskCount;
                }
                else
                {
                    allHandled = false;
                }

                break;
            }

            case PipelineTaskType::PrecompilePipelines:
            {
                // Deliberately leaving the type null to account for the enqueue
                // call not incrementing the compiling pipeline task counter.
                PipelineTaskTokenPair tokenPair;

                for (auto vertexElements : g_vertexDeclarationCache)
                    CreateVertexDeclarationWithoutAddRef(reinterpret_cast<GuestVertexElement*>(vertexElements));

                for (auto pipelineState : g_pipelineStateCache)
                {
                    // The hashes were reinterpret casted to pointers in the cache.
                    pipelineState.vertexShader = FindShaderCacheEntry(reinterpret_cast<XXH64_hash_t>(pipelineState.vertexShader))->guestShader;

                    if (pipelineState.pixelShader != nullptr)
                        pipelineState.pixelShader = FindShaderCacheEntry(reinterpret_cast<XXH64_hash_t>(pipelineState.pixelShader))->guestShader;

                    {
                        std::lock_guard lock(g_vertexDeclarationMutex);
                        pipelineState.vertexDeclaration = g_vertexDeclarations[reinterpret_cast<XXH64_hash_t>(pipelineState.vertexDeclaration)];
                    }

                    if (!g_capabilities.triangleFan && pipelineState.primitiveTopology == RenderPrimitiveTopology::TRIANGLE_FAN)
                        pipelineState.primitiveTopology = RenderPrimitiveTopology::TRIANGLE_LIST;

                    // Zero out depth bias for Vulkan, we only store common values for D3D12.
                    if (g_capabilities.dynamicDepthBias && g_vulkan)
                    {
                        pipelineState.depthBias = 0;
                        pipelineState.slopeScaledDepthBias = 0.0f;
                    }

                    if (Config::GITextureFiltering == EGITextureFiltering::Bicubic)
                        pipelineState.specConstants |= SPEC_CONSTANT_BICUBIC_GI_FILTER;

                    auto createGraphicsPipeline = [&](PipelineState& pipelineStateToCreate, const char* name)
                        {
                            SanitizePipelineState(pipelineStateToCreate);
                            EnqueueGraphicsPipelineCompilation(pipelineStateToCreate, tokenPair, name, true);
                        };

                    // Compile both MSAA and non MSAA variants to work with reflection maps. The render formats are an assumption but it should hold true.
                    if (Config::AntiAliasing != EAntiAliasing::None &&
                        pipelineState.renderTargetFormat == RenderFormat::R16G16B16A16_FLOAT && 
                        pipelineState.depthStencilFormat == RenderFormat::D32_FLOAT)
                    {
                        auto msaaPipelineState = pipelineState;
                        msaaPipelineState.sampleCount = int32_t(Config::AntiAliasing.Value);

                        if (Config::TransparencyAntiAliasing && (msaaPipelineState.specConstants & SPEC_CONSTANT_ALPHA_TEST) != 0)
                        {
                            msaaPipelineState.enableAlphaToCoverage = true;
                            msaaPipelineState.specConstants &= ~SPEC_CONSTANT_ALPHA_TEST;
                            msaaPipelineState.specConstants |= SPEC_CONSTANT_ALPHA_TO_COVERAGE;
                        }

                        createGraphicsPipeline(msaaPipelineState, "Precompiled Pipeline MSAA");
                    }

                    if (pipelineState.pixelShader != nullptr &&
                        pipelineState.pixelShader->shaderCacheEntry != nullptr)
                    {
                        XXH64_hash_t hash = pipelineState.pixelShader->shaderCacheEntry->hash;

                        // Compile the custom gaussian blur shaders that we pass to the game.
                        if (hash == 0x4294510C775F4EE8)
                        {
                            for (auto& shader : g_gaussianBlurShaders)
                            {
                                auto newPipelineState = pipelineState;
                                newPipelineState.pixelShader = shader.get();
                                createGraphicsPipeline(newPipelineState, "Precompiled Gaussian Blur Pipeline");
                            }
                        }
                        // Compile enhanced motion blur shader.
                        else if (hash == 0x6B9732B4CD7E7740)
                        {
                            auto newPipelineState = pipelineState;
                            newPipelineState.pixelShader = g_enhancedMotionBlurShader.get();
                            createGraphicsPipeline(newPipelineState, "Precompiled Enhanced Motion Blur Pipeline");
                        }
                    }
                
                    createGraphicsPipeline(pipelineState, "Precompiled Pipeline");

                    // Compile the CSD filter shader that we pass to the game when point filtering is used.
                    if (pipelineState.pixelShader == g_csdShader)
                    {
                        pipelineState.pixelShader = g_csdFilterShader.get();
                        createGraphicsPipeline(pipelineState, "Precompiled CSD Filter Pipeline");
                    }
                }

                type = PipelineTaskType::Null;
                --g_pendingPipelineTaskCount;

                break;
            }

            case PipelineTaskType::RecompilePipelines:
            {
                PipelineTaskTokenPair tokenPair;
                tokenPair.token.type = type;

                auto asyncPipelines = g_asyncPipelineStates.values();

                for (auto& [hash, pipelineState] : asyncPipelines)
                {
                    bool alphaTest = (pipelineState.specConstants & (SPEC_CONSTANT_ALPHA_TEST | SPEC_CONSTANT_ALPHA_TO_COVERAGE)) != 0;
                    bool msaa = pipelineState.sampleCount != 1 || (pipelineState.renderTargetFormat == RenderFormat::R16G16B16A16_FLOAT && pipelineState.depthStencilFormat == RenderFormat::D32_FLOAT);

                    pipelineState.sampleCount = 1;
                    pipelineState.enableAlphaToCoverage = false;
                    pipelineState.specConstants &= ~(SPEC_CONSTANT_BICUBIC_GI_FILTER | SPEC_CONSTANT_ALPHA_TEST | SPEC_CONSTANT_ALPHA_TO_COVERAGE);

                    if (msaa && Config::AntiAliasing != EAntiAliasing::None)
                    {
                        pipelineState.sampleCount = int32_t(Config::AntiAliasing.Value);

                        if (alphaTest)
                        {
                            if (Config::TransparencyAntiAliasing)
                            {
                                pipelineState.enableAlphaToCoverage = true;
                                pipelineState.specConstants |= SPEC_CONSTANT_ALPHA_TO_COVERAGE;
                            }
                            else
                            {
                                pipelineState.specConstants |= SPEC_CONSTANT_ALPHA_TEST;
                            }
                        }
                    }
                    else if (alphaTest)
                    {
                        pipelineState.specConstants |= SPEC_CONSTANT_ALPHA_TEST;
                    }

                    if (Config::GITextureFiltering == EGITextureFiltering::Bicubic)
                        pipelineState.specConstants |= SPEC_CONSTANT_BICUBIC_GI_FILTER;

                    SanitizePipelineState(pipelineState);
                    EnqueueGraphicsPipelineCompilation(pipelineState, tokenPair, "Recompiled Pipeline State");
                }

                type = PipelineTaskType::Null;
                --g_pendingPipelineTaskCount;

                break;
            }
            }
        }

        if (allHandled)
            localPipelineTaskQueue.clear();

        std::this_thread::yield();
    }
}

static std::thread g_pipelineTaskConsumerThread(PipelineTaskConsumerThread);

#ifdef ASYNC_PSO_DEBUG

PPC_FUNC_IMPL(__imp__sub_82E33330);
PPC_FUNC(sub_82E33330)
{
    auto vertexShaderCode = reinterpret_cast<Hedgehog::Mirage::CVertexShaderCodeData*>(g_memory.Translate(ctx.r4.u32));
    __imp__sub_82E33330(ctx, base);
    reinterpret_cast<GuestShader*>(vertexShaderCode->m_pD3DVertexShader.get())->name = vertexShaderCode->m_TypeAndName.c_str() + 3;
}

PPC_FUNC_IMPL(__imp__sub_82E328D8);
PPC_FUNC(sub_82E328D8)
{
    auto pixelShaderCode = reinterpret_cast<Hedgehog::Mirage::CPixelShaderCodeData*>(g_memory.Translate(ctx.r4.u32));
    __imp__sub_82E328D8(ctx, base);
    reinterpret_cast<GuestShader*>(pixelShaderCode->m_pD3DPixelShader.get())->name = pixelShaderCode->m_TypeAndName.c_str() + 2;
}

#endif

#ifdef PSO_CACHING
class SDLEventListenerForPSOCaching : public SDLEventListener
{
public:
    bool OnSDLEvent(SDL_Event* event) override 
    {
        if (event->type != SDL_QUIT)
            return false;

        std::lock_guard lock(g_pipelineCacheMutex);
        if (g_pipelineStatesToCache.empty())
            return false;

        FILE* f = fopen("send_this_file_to_skyth.txt", "ab");
        if (f != nullptr)
        {
            ankerl::unordered_dense::set<GuestVertexDeclaration*> vertexDeclarations;
            xxHashMap<PipelineState> pipelineStatesToCache;

            for (auto& [hash, pipelineState] : g_pipelineStatesToCache)
            {
                if (pipelineState.vertexShader->shaderCacheEntry == nullptr ||
                    (pipelineState.pixelShader != nullptr && pipelineState.pixelShader->shaderCacheEntry == nullptr))
                {
                    continue;
                }

                vertexDeclarations.emplace(pipelineState.vertexDeclaration);

                // Mask out the config options.
                pipelineState.sampleCount = 1;
                pipelineState.enableAlphaToCoverage = false;

                pipelineState.specConstants &= ~SPEC_CONSTANT_BICUBIC_GI_FILTER;
                if ((pipelineState.specConstants & SPEC_CONSTANT_ALPHA_TO_COVERAGE) != 0)
                {
                    pipelineState.specConstants &= ~SPEC_CONSTANT_ALPHA_TO_COVERAGE;
                    pipelineState.specConstants |= SPEC_CONSTANT_ALPHA_TEST;
                }

                pipelineStatesToCache.emplace(XXH3_64bits(&pipelineState, sizeof(pipelineState)), pipelineState);
            }

            for (auto vertexDeclaration : vertexDeclarations)
            {
                fmt::print(f, "static uint8_t g_vertexElements_{:016X}[] = {{", vertexDeclaration->hash);

                auto bytes = reinterpret_cast<uint8_t*>(vertexDeclaration->vertexElements.get());
                for (size_t i = 0; i < vertexDeclaration->vertexElementCount * sizeof(GuestVertexElement); i++)
                    fmt::print(f, "0x{:X},", bytes[i]);

                fmt::println(f, "}};");
            }

            for (auto& [pipelineHash, pipelineState] : pipelineStatesToCache)
            {
                fmt::println(f, "{{ "
                    "reinterpret_cast<GuestShader*>(0x{:X}),"
                    "reinterpret_cast<GuestShader*>(0x{:X}),"
                    "reinterpret_cast<GuestVertexDeclaration*>(0x{:X}),"
                    "{},"
                    "{},"
                    "{},"
                    "RenderBlend::{},"
                    "RenderBlend::{},"
                    "RenderCullMode::{},"
                    "RenderComparisonFunction::{},"
                    "{},"
                    "RenderBlendOperation::{},"
                    "{},"
                    "{},"
                    "RenderBlend::{},"
                    "RenderBlend::{},"
                    "RenderBlendOperation::{},"
                    "0x{:X},"
                    "RenderPrimitiveTopology::{},"
                    "{{ {},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{} }},"
                    "RenderFormat::{},"
                    "RenderFormat::{},"
                    "{},"
                    "{},"
                    "0x{:X} }},",
                    pipelineState.vertexShader->shaderCacheEntry->hash,
                    pipelineState.pixelShader != nullptr ? pipelineState.pixelShader->shaderCacheEntry->hash : 0,
                    pipelineState.vertexDeclaration->hash,
                    pipelineState.instancing,
                    pipelineState.zEnable,
                    pipelineState.zWriteEnable,
                    magic_enum::enum_name(pipelineState.srcBlend),
                    magic_enum::enum_name(pipelineState.destBlend),
                    magic_enum::enum_name(pipelineState.cullMode),
                    magic_enum::enum_name(pipelineState.zFunc),
                    pipelineState.alphaBlendEnable,
                    magic_enum::enum_name(pipelineState.blendOp),
                    pipelineState.slopeScaledDepthBias,
                    pipelineState.depthBias,
                    magic_enum::enum_name(pipelineState.srcBlendAlpha),
                    magic_enum::enum_name(pipelineState.destBlendAlpha),
                    magic_enum::enum_name(pipelineState.blendOpAlpha),
                    pipelineState.colorWriteEnable,
                    magic_enum::enum_name(pipelineState.primitiveTopology),
                    pipelineState.vertexStrides[0],
                    pipelineState.vertexStrides[1],
                    pipelineState.vertexStrides[2],
                    pipelineState.vertexStrides[3],
                    pipelineState.vertexStrides[4],
                    pipelineState.vertexStrides[5],
                    pipelineState.vertexStrides[6],
                    pipelineState.vertexStrides[7],
                    pipelineState.vertexStrides[8],
                    pipelineState.vertexStrides[9],
                    pipelineState.vertexStrides[10],
                    pipelineState.vertexStrides[11],
                    pipelineState.vertexStrides[12],
                    pipelineState.vertexStrides[13],
                    pipelineState.vertexStrides[14],
                    pipelineState.vertexStrides[15],
                    magic_enum::enum_name(pipelineState.renderTargetFormat),
                    magic_enum::enum_name(pipelineState.depthStencilFormat),
                    pipelineState.sampleCount,
                    pipelineState.enableAlphaToCoverage,
                    pipelineState.specConstants);
            }

            fclose(f);
        }

        return false;
    }
};
SDLEventListenerForPSOCaching g_sdlEventListenerForPSOCaching;
#endif

void VideoConfigValueChangedCallback(IConfigDef* config)
{
    // VR: the headset images are replaced between frames (see Video::Present).
    if (config == &Config::VRRenderScale)
        VR::SetRenderScale(Config::VRRenderScale);

    // Config options that require internal resolution resize
    g_needsResize |=
        config == &Config::AspectRatio ||
        config == &Config::ResolutionScale ||
        config == &Config::AntiAliasing ||
        config == &Config::ShadowResolution;

    if (g_needsResize)
        Video::ComputeViewportDimensions();
        
    // Config options that require pipeline recompilation
    bool shouldRecompile =
        config == &Config::AntiAliasing ||
        config == &Config::TransparencyAntiAliasing ||
        config == &Config::GITextureFiltering;

    if (shouldRecompile)
        EnqueuePipelineTask(PipelineTaskType::RecompilePipelines, {});
}

// SWA::CCsdTexListMirage::SetFilter
PPC_FUNC_IMPL(__imp__sub_825E4300);
PPC_FUNC(sub_825E4300)
{
    g_csdFilterState = ctx.r5.u32 == 0 ? CsdFilterState::On : CsdFilterState::Off;
    ctx.r5.u32 = 1;
    __imp__sub_825E4300(ctx, base);
}

// SWA::CCsdPlatformMirage::EndScene
PPC_FUNC_IMPL(__imp__sub_825E2F78);
PPC_FUNC(sub_825E2F78)
{
    g_csdFilterState = CsdFilterState::Unknown;
    __imp__sub_825E2F78(ctx, base);
}

// Game shares surfaces with identical descriptions. We don't want to share shadow maps,
// so we can set its format to a depth format that still resolves to the same type in recomp,
// but manages to keep the surfaces actually separated in guest code.
void FxShadowMapInitMidAsmHook(PPCRegister& r11)
{
    uint8_t* base = g_memory.base;

    uint32_t surface = PPC_LOAD_U32(PPC_LOAD_U32(PPC_LOAD_U32(r11.u32 + 0x24) + 0x4));
    PPC_STORE_U32(surface + 0x20, D3DFMT_D24FS8);
}

// Re-render objects in the terrain shadow map instead of copying the texture.
static bool g_jumpOverStretchRect;

void FxShadowMapNoTerrainMidAsmHook(PPCRegister& r4, PPCRegister& r30)
{
    // Set the no terrain shadow map as the render target.
    uint8_t* base = g_memory.base;
    r4.u64 = PPC_LOAD_U32(r30.u32 + 0x58);
}

bool FxShadowMapMidAsmHook(PPCRegister& r4, PPCRegister& r5, PPCRegister& r6, PPCRegister& r30)
{
    if (g_jumpOverStretchRect)
    {
        // Reset for the next time shadow maps get rendered.
        g_jumpOverStretchRect = false;

        // Jump over the stretch rect call.
        return false;
    }
    else
    {
        // Mark to jump over the stretch call the next time.
        g_jumpOverStretchRect = true;

        // Jump to the beginning. Set registers accordingly to set the terrain shadow map as the render target.
        uint8_t* base = g_memory.base;
        r6.u64 = 0;
        r5.u64 = 0;
        r4.u64 = PPC_LOAD_U32(r30.u32 + 0x50);

        return true;
    }
}

// There is a bug on AMD where restart indices cause incorrect culling and prevent some triangles from being rendered.
// This seems to happen on both Windows AMD drivers and Mesa. Converting restart indices to degenerate triangles fixes it.
static void ConvertToDegenerateTriangles(uint16_t* indices, uint32_t indexCount, uint16_t*& newIndices, uint32_t& newIndexCount)
{
    newIndices = reinterpret_cast<uint16_t*>(g_userHeap.Alloc(indexCount * sizeof(uint16_t) * 3));
    newIndexCount = 0;

    bool stripStart = true;
    uint32_t stripSize = 0;
    uint16_t lastIndex = 0;

    for (uint32_t i = 0; i < indexCount; i++)
    {
        uint16_t index = indices[i];
        if (index == 0xFFFF)
        {
            if ((stripSize % 2) != 0)
                newIndices[newIndexCount++] = lastIndex;

            stripStart = true;
            stripSize = 0;
        }
        else 
        {
            if (stripStart && newIndexCount != 0)
            {
                newIndices[newIndexCount++] = lastIndex;
                newIndices[newIndexCount++] = index;
            }

            newIndices[newIndexCount++] = index;
            stripStart = false;
            ++stripSize;
            lastIndex = index;
        }
    }
}

struct MeshResource
{
    SWA_INSERT_PADDING(0x4);
    be<uint32_t> indexCount;
    be<uint32_t> indices;
};

static std::vector<uint16_t*> g_newIndicesToFree;

// Hedgehog::Mirage::CMeshData::Make
PPC_FUNC_IMPL(__imp__sub_82E44AF8);
PPC_FUNC(sub_82E44AF8)
{
    uint16_t* newIndicesToFree = nullptr;

    auto databaseData = reinterpret_cast<Hedgehog::Database::CDatabaseData*>(base + ctx.r3.u32);
    if (g_triangleStripWorkaround && !databaseData->IsMadeOne())
    {
        auto meshResource = reinterpret_cast<MeshResource*>(base + ctx.r4.u32);

        if (meshResource->indexCount != 0)
        {
            uint16_t* newIndices;
            uint32_t newIndexCount;

            ConvertToDegenerateTriangles(
                reinterpret_cast<uint16_t*>(base + meshResource->indices),
                meshResource->indexCount,
                newIndices,
                newIndexCount);

            meshResource->indexCount = newIndexCount;
            meshResource->indices = static_cast<uint32_t>(reinterpret_cast<uint8_t*>(newIndices) - base);

            if (PPC_LOAD_U32(0x83396E98) != NULL)
            {
                // If index buffers are getting merged, new indices need to survive until the merge happens.
                g_newIndicesToFree.push_back(newIndices);
            }
            else 
            {
                // Otherwise, we can free it immediately.
                newIndicesToFree = newIndices;
            }
        }
    }

    __imp__sub_82E44AF8(ctx, base);

    if (newIndicesToFree != nullptr)
        g_userHeap.Free(newIndicesToFree);
}

// Hedgehog::Mirage::CShareVertexBuffer::Reset
PPC_FUNC_IMPL(__imp__sub_82E250D0);
PPC_FUNC(sub_82E250D0)
{
    __imp__sub_82E250D0(ctx, base);

    for (auto newIndicesToFree : g_newIndicesToFree)
        g_userHeap.Free(newIndicesToFree);

    g_newIndicesToFree.clear();
}

struct LightAndIndexBufferResourceV1
{
    SWA_INSERT_PADDING(0x4);
    be<uint32_t> indexCount;
    be<uint32_t> indices;
};

// Hedgehog::Mirage::CLightAndIndexBufferData::MakeV1
PPC_FUNC_IMPL(__imp__sub_82E3AFC8);
PPC_FUNC(sub_82E3AFC8)
{
    uint16_t* newIndices = nullptr;

    auto databaseData = reinterpret_cast<Hedgehog::Database::CDatabaseData*>(base + ctx.r3.u32);
    if (g_triangleStripWorkaround && !databaseData->IsMadeOne())
    {
        auto lightAndIndexBufferResource = reinterpret_cast<LightAndIndexBufferResourceV1*>(base + ctx.r4.u32);

        if (lightAndIndexBufferResource->indexCount != 0)
        {
            uint32_t newIndexCount;

            ConvertToDegenerateTriangles(
                reinterpret_cast<uint16_t*>(base + lightAndIndexBufferResource->indices),
                lightAndIndexBufferResource->indexCount,
                newIndices,
                newIndexCount);

            lightAndIndexBufferResource->indexCount = newIndexCount;
            lightAndIndexBufferResource->indices = static_cast<uint32_t>(reinterpret_cast<uint8_t*>(newIndices) - base);
        }
    }

    __imp__sub_82E3AFC8(ctx, base);

    if (newIndices != nullptr)
        g_userHeap.Free(newIndices);
}

struct LightAndIndexBufferResourceV5
{
    SWA_INSERT_PADDING(0x8);
    be<uint32_t> indexCount;
    be<uint32_t> indices;
};

// Hedgehog::Mirage::CLightAndIndexBufferData::MakeV5
PPC_FUNC_IMPL(__imp__sub_82E3B1C0);
PPC_FUNC(sub_82E3B1C0)
{
    uint16_t* newIndices = nullptr;

    auto databaseData = reinterpret_cast<Hedgehog::Database::CDatabaseData*>(base + ctx.r3.u32);
    if (g_triangleStripWorkaround && !databaseData->IsMadeOne())
    {
        auto lightAndIndexBufferResource = reinterpret_cast<LightAndIndexBufferResourceV5*>(base + ctx.r4.u32);

        if (lightAndIndexBufferResource->indexCount != 0)
        {
            uint32_t newIndexCount;

            ConvertToDegenerateTriangles(
                reinterpret_cast<uint16_t*>(base + lightAndIndexBufferResource->indices),
                lightAndIndexBufferResource->indexCount,
                newIndices,
                newIndexCount);

            lightAndIndexBufferResource->indexCount = newIndexCount;
            lightAndIndexBufferResource->indices = static_cast<uint32_t>(reinterpret_cast<uint8_t*>(newIndices) - base);
        }
    }

    __imp__sub_82E3B1C0(ctx, base);

    if (newIndices != nullptr)
        g_userHeap.Free(newIndices);
}

GUEST_FUNCTION_HOOK(sub_82BD99B0, CreateDevice);

GUEST_FUNCTION_HOOK(sub_82BE6230, DestructResource);

GUEST_FUNCTION_HOOK(sub_82BE9300, LockTextureRect);
GUEST_FUNCTION_HOOK(sub_82BE7780, UnlockTextureRect);

GUEST_FUNCTION_HOOK(sub_82BE6B98, LockVertexBuffer);
GUEST_FUNCTION_HOOK(sub_82BE6BE8, UnlockVertexBuffer);
GUEST_FUNCTION_HOOK(sub_82BE61D0, GetVertexBufferDesc);

GUEST_FUNCTION_HOOK(sub_82BE6CA8, LockIndexBuffer);
GUEST_FUNCTION_HOOK(sub_82BE6CF0, UnlockIndexBuffer);
GUEST_FUNCTION_HOOK(sub_82BE6200, GetIndexBufferDesc);

GUEST_FUNCTION_HOOK(sub_82BE96F0, GetSurfaceDesc);

GUEST_FUNCTION_HOOK(sub_82BE04B0, GetVertexDeclaration);
GUEST_FUNCTION_HOOK(sub_82BE0530, HashVertexDeclaration);

GUEST_FUNCTION_HOOK(sub_82BDA8C0, Video::Present);
GUEST_FUNCTION_HOOK(sub_82BDD330, GetBackBuffer);

GUEST_FUNCTION_HOOK(sub_82BE9498, CreateTexture);
GUEST_FUNCTION_HOOK(sub_82BE6AD0, CreateVertexBuffer);
GUEST_FUNCTION_HOOK(sub_82BE6BF8, CreateIndexBuffer);
GUEST_FUNCTION_HOOK(sub_82BE95B8, CreateSurface);

GUEST_FUNCTION_HOOK(sub_82BF6400, StretchRect);

GUEST_FUNCTION_HOOK(sub_82BDD9F0, SetRenderTarget);
GUEST_FUNCTION_HOOK(sub_82BDDD38, SetDepthStencilSurface);

GUEST_FUNCTION_HOOK(sub_82BFE4C8, Clear);

GUEST_FUNCTION_HOOK(sub_82BDD8C0, SetViewport);

GUEST_FUNCTION_HOOK(sub_82BE9818, SetTexture);
GUEST_FUNCTION_HOOK(sub_82BDCFB0, SetScissorRect);

GUEST_FUNCTION_HOOK(sub_82BE5900, DrawPrimitive);
GUEST_FUNCTION_HOOK(sub_82BE5CF0, DrawIndexedPrimitive);
GUEST_FUNCTION_HOOK(sub_82BE52F8, DrawPrimitiveUP);

GUEST_FUNCTION_HOOK(sub_82BE0428, CreateVertexDeclaration);
GUEST_FUNCTION_HOOK(sub_82BE02E0, SetVertexDeclaration);

GUEST_FUNCTION_HOOK(sub_82BE1A80, CreateVertexShader);
GUEST_FUNCTION_HOOK(sub_82BE0110, SetVertexShader);

GUEST_FUNCTION_HOOK(sub_82BDD0F8, SetStreamSource);
GUEST_FUNCTION_HOOK(sub_82BDD218, SetIndices);

GUEST_FUNCTION_HOOK(sub_82BE1990, CreatePixelShader);
GUEST_FUNCTION_HOOK(sub_82BDFE58, SetPixelShader);

GUEST_FUNCTION_HOOK(sub_82C003B8, D3DXFillTexture);
GUEST_FUNCTION_HOOK(sub_82C00910, D3DXFillVolumeTexture);

GUEST_FUNCTION_HOOK(sub_82E43FC8, MakePictureData);

GUEST_FUNCTION_HOOK(sub_82E9EE38, SetResolution);

GUEST_FUNCTION_HOOK(sub_82AE2BF8, ScreenShaderInit);

// This is a buggy function that recreates framebuffers
// if the inverse capture ratio is not 2.0, but the parameter
// is completely unused and not stored, so it ends up
// recreating framebuffers every single frame instead.
GUEST_FUNCTION_STUB(sub_82BAAD38);

GUEST_FUNCTION_STUB(sub_822C15D8);
GUEST_FUNCTION_STUB(sub_822C1810);
GUEST_FUNCTION_STUB(sub_82BD97A8);
GUEST_FUNCTION_STUB(sub_82BD97E8);
GUEST_FUNCTION_STUB(sub_82BDD370); // SetGammaRamp
GUEST_FUNCTION_STUB(sub_82BE05B8);
GUEST_FUNCTION_STUB(sub_82BE9C98);
GUEST_FUNCTION_STUB(sub_82BEA308);
GUEST_FUNCTION_STUB(sub_82CD5D68);
GUEST_FUNCTION_STUB(sub_82BE9B28);
GUEST_FUNCTION_STUB(sub_82BEA018);
GUEST_FUNCTION_STUB(sub_82BEA7C0);
GUEST_FUNCTION_STUB(sub_82BFFF88); // D3DXFilterTexture
GUEST_FUNCTION_STUB(sub_82BD96D0);
