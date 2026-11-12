#pragma once

struct ShaderCacheEntry
{
    const uint64_t hash;
    const uint32_t dxilOffset;
    const uint32_t dxilSize;
    const uint32_t spirvOffset;
    const uint32_t spirvSize;
    const uint32_t specConstantsMask;

    // The single-pass stereo variant (VR_STEREO), in its own caches.
    const uint32_t stereoDxilOffset;
    const uint32_t stereoDxilSize;
    const uint32_t stereoSpirvOffset;
    const uint32_t stereoSpirvSize;

    struct GuestShader* guestShader;
};

extern ShaderCacheEntry g_shaderCacheEntries[];
extern const size_t g_shaderCacheEntryCount;

extern const uint8_t g_compressedDxilCache[];
extern const size_t g_dxilCacheCompressedSize;
extern const size_t g_dxilCacheDecompressedSize;

extern const uint8_t g_compressedSpirvCache[];
extern const size_t g_spirvCacheCompressedSize;
extern const size_t g_spirvCacheDecompressedSize;

extern const uint8_t g_compressedStereoDxilCache[];
extern const size_t g_stereoDxilCacheCompressedSize;
extern const size_t g_stereoDxilCacheDecompressedSize;

extern const uint8_t g_compressedStereoSpirvCache[];
extern const size_t g_stereoSpirvCacheCompressedSize;
extern const size_t g_stereoSpirvCacheDecompressedSize;
