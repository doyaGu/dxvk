#pragma once

#include "dx8_types.h"
#include "dx8_constants.h"

#include <bgfx/bgfx.h>
#include <cstdint>
#include <vector>

namespace dx8bgfx {

// =============================================================================
// D3D8 Format Definitions
// =============================================================================

enum D3DFORMAT {
    D3DFMT_UNKNOWN              = 0,

    // RGB formats
    D3DFMT_R8G8B8               = 20,
    D3DFMT_A8R8G8B8             = 21,
    D3DFMT_X8R8G8B8             = 22,
    D3DFMT_R5G6B5               = 23,
    D3DFMT_X1R5G5B5             = 24,
    D3DFMT_A1R5G5B5             = 25,
    D3DFMT_A4R4G4B4             = 26,
    D3DFMT_R3G3B2               = 27,
    D3DFMT_A8                   = 28,
    D3DFMT_A8R3G3B2             = 29,
    D3DFMT_X4R4G4B4             = 30,
    D3DFMT_A2B10G10R10          = 31,
    D3DFMT_G16R16               = 34,

    // Palette formats
    D3DFMT_A8P8                 = 40,
    D3DFMT_P8                   = 41,

    // Luminance formats
    D3DFMT_L8                   = 50,
    D3DFMT_A8L8                 = 51,
    D3DFMT_A4L4                 = 52,

    // Bump mapping formats
    D3DFMT_V8U8                 = 60,
    D3DFMT_L6V5U5               = 61,
    D3DFMT_X8L8V8U8             = 62,
    D3DFMT_Q8W8V8U8             = 63,
    D3DFMT_V16U16               = 64,

    // Depth/Stencil formats
    D3DFMT_D16_LOCKABLE         = 70,
    D3DFMT_D32                  = 71,
    D3DFMT_D15S1                = 73,
    D3DFMT_D24S8                = 75,
    D3DFMT_D24X8                = 77,
    D3DFMT_D24X4S4              = 79,
    D3DFMT_D16                  = 80,

    // DXT compressed formats
    D3DFMT_DXT1                 = MAKEFOURCC('D', 'X', 'T', '1'),
    D3DFMT_DXT2                 = MAKEFOURCC('D', 'X', 'T', '2'),
    D3DFMT_DXT3                 = MAKEFOURCC('D', 'X', 'T', '3'),
    D3DFMT_DXT4                 = MAKEFOURCC('D', 'X', 'T', '4'),
    D3DFMT_DXT5                 = MAKEFOURCC('D', 'X', 'T', '5'),

    // Vertex data formats
    D3DFMT_VERTEXDATA           = 100,
    D3DFMT_INDEX16              = 101,
    D3DFMT_INDEX32              = 102,
};

// Helper macro for FOURCC codes
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
    ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) | \
    ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24))
#endif

// =============================================================================
// Texture Utilities
// =============================================================================

class TextureUtils {
public:
    // Format conversion
    static bgfx::TextureFormat::Enum D3DFormatToBgfx(D3DFORMAT format);
    static D3DFORMAT BgfxFormatToD3D(bgfx::TextureFormat::Enum format);

    // Format information
    static uint32_t GetBitsPerPixel(D3DFORMAT format);
    static uint32_t GetBlockSize(D3DFORMAT format);
    static bool IsCompressed(D3DFORMAT format);
    static bool HasAlpha(D3DFORMAT format);
    static bool IsDepthFormat(D3DFORMAT format);

    // Pitch calculation
    static uint32_t CalculatePitch(D3DFORMAT format, uint32_t width);
    static uint32_t CalculateSlicePitch(D3DFORMAT format, uint32_t width, uint32_t height);

    // Mipmap calculation
    static uint32_t CalculateMipLevels(uint32_t width, uint32_t height);
    static void CalculateMipDimensions(uint32_t level, uint32_t& width, uint32_t& height);

    // Pixel conversion
    static void ConvertPixels(
        const void* srcData, D3DFORMAT srcFormat,
        void* dstData, D3DFORMAT dstFormat,
        uint32_t width, uint32_t height
    );

    // BGRA <-> RGBA swizzle
    static void SwizzleBGRAtoRGBA(void* data, uint32_t width, uint32_t height, uint32_t bpp);
    static void SwizzleRGBAtoBGRA(void* data, uint32_t width, uint32_t height, uint32_t bpp);

    // Create bgfx texture from D3D8 data
    static bgfx::TextureHandle CreateTexture2D(
        uint32_t width, uint32_t height,
        bool hasMips, uint16_t numLayers,
        D3DFORMAT format,
        uint64_t flags,
        const void* data, uint32_t dataSize
    );

    // Lock/unlock helper structures
    struct LockedRect {
        int pitch;
        void* pBits;
    };

    struct LockedBox {
        int rowPitch;
        int slicePitch;
        void* pBits;
    };

    // Surface operations
    static bool CopyRects(
        const void* srcData, D3DFORMAT srcFormat, uint32_t srcPitch,
        void* dstData, D3DFORMAT dstFormat, uint32_t dstPitch,
        uint32_t width, uint32_t height,
        uint32_t srcX, uint32_t srcY,
        uint32_t dstX, uint32_t dstY
    );

    // Color key support (for D3DX-style color keying)
    static void ApplyColorKey(
        void* data, D3DFORMAT format,
        uint32_t width, uint32_t height,
        uint32_t colorKey
    );

    // Generate mipmaps
    static std::vector<uint8_t> GenerateMipmaps(
        const void* data, D3DFORMAT format,
        uint32_t width, uint32_t height,
        uint32_t& outMipLevels
    );

private:
    // Internal conversion helpers
    static uint32_t ConvertPixelToRGBA8(const uint8_t* src, D3DFORMAT format);
    static void ConvertRGBA8ToPixel(uint32_t rgba, uint8_t* dst, D3DFORMAT format);

    // Box filter for mipmap generation
    static void BoxFilter2D(
        const uint8_t* src, uint8_t* dst,
        uint32_t srcWidth, uint32_t srcHeight,
        uint32_t bpp
    );
};

// =============================================================================
// Surface Description
// =============================================================================

struct D3DSURFACE_DESC {
    D3DFORMAT Format;
    D3DRESOURCETYPE Type;
    DWORD Usage;
    D3DPOOL Pool;
    UINT Size;
    D3DMULTISAMPLE_TYPE MultiSampleType;
    UINT Width;
    UINT Height;
};

enum D3DRESOURCETYPE {
    D3DRTYPE_SURFACE        = 1,
    D3DRTYPE_VOLUME         = 2,
    D3DRTYPE_TEXTURE        = 3,
    D3DRTYPE_VOLUMETEXTURE  = 4,
    D3DRTYPE_CUBETEXTURE    = 5,
    D3DRTYPE_VERTEXBUFFER   = 6,
    D3DRTYPE_INDEXBUFFER    = 7
};

enum D3DPOOL {
    D3DPOOL_DEFAULT     = 0,
    D3DPOOL_MANAGED     = 1,
    D3DPOOL_SYSTEMMEM   = 2,
    D3DPOOL_SCRATCH     = 3
};

enum D3DMULTISAMPLE_TYPE {
    D3DMULTISAMPLE_NONE         = 0,
    D3DMULTISAMPLE_2_SAMPLES    = 2,
    D3DMULTISAMPLE_3_SAMPLES    = 3,
    D3DMULTISAMPLE_4_SAMPLES    = 4,
    D3DMULTISAMPLE_5_SAMPLES    = 5,
    D3DMULTISAMPLE_6_SAMPLES    = 6,
    D3DMULTISAMPLE_7_SAMPLES    = 7,
    D3DMULTISAMPLE_8_SAMPLES    = 8,
    D3DMULTISAMPLE_9_SAMPLES    = 9,
    D3DMULTISAMPLE_10_SAMPLES   = 10,
    D3DMULTISAMPLE_11_SAMPLES   = 11,
    D3DMULTISAMPLE_12_SAMPLES   = 12,
    D3DMULTISAMPLE_13_SAMPLES   = 13,
    D3DMULTISAMPLE_14_SAMPLES   = 14,
    D3DMULTISAMPLE_15_SAMPLES   = 15,
    D3DMULTISAMPLE_16_SAMPLES   = 16
};

// Usage flags
constexpr DWORD D3DUSAGE_RENDERTARGET       = 0x00000001;
constexpr DWORD D3DUSAGE_DEPTHSTENCIL       = 0x00000002;
constexpr DWORD D3DUSAGE_DYNAMIC            = 0x00000200;
constexpr DWORD D3DUSAGE_AUTOGENMIPMAP      = 0x00000400;

} // namespace dx8bgfx
