#pragma once

#include "dx8_types.h"
#include "dx8_constants.h"

#include <bgfx/bgfx.h>

namespace dx8bgfx {

// =============================================================================
// Device Capabilities
// =============================================================================

struct DeviceCaps {
    // Device info
    uint32_t vendorId;
    uint16_t deviceId;
    char renderer[256];

    // Texture capabilities
    uint32_t maxTextureWidth;
    uint32_t maxTextureHeight;
    uint32_t maxTextureDepth;
    uint32_t maxTextureLayers;
    uint32_t maxTextureStages;
    uint32_t maxSimultaneousTextures;

    // Vertex capabilities
    uint32_t maxVertexStreams;
    uint32_t maxVertexIndex;
    uint32_t maxPrimitiveCount;

    // Shader capabilities
    bool supportsVertexShaders;
    bool supportsPixelShaders;
    uint32_t maxVertexShaderConsts;
    uint32_t maxPixelShaderConsts;

    // Feature support
    bool supportsHardwareTnL;
    bool supportsMipmaps;
    bool supportsCubeTextures;
    bool supportsVolumeTextures;
    bool supportsAnisotropicFiltering;
    bool supportsMultisampling;
    bool supportsOcclusionQuery;
    bool supportsInstancing;
    bool supportsCompute;

    // Texture format support
    bool supportsBC1;  // DXT1
    bool supportsBC2;  // DXT3
    bool supportsBC3;  // DXT5
    bool supportsBC4;  // ATI1
    bool supportsBC5;  // ATI2
    bool supportsBC6H;
    bool supportsBC7;
    bool supportsETC1;
    bool supportsETC2;
    bool supportsPVRTC;
    bool supportsASTC;

    // Render target capabilities
    uint32_t maxRenderTargets;
    uint32_t maxColorAttachments;
    bool supportsFloatRenderTargets;
    bool supportsSRGBRenderTargets;

    // Limits
    uint32_t maxAnisotropy;
    uint32_t maxLights;
    uint32_t maxClipPlanes;
    uint32_t maxUserClipPlanes;
    float maxPointSize;
    uint32_t maxVertexBlendMatrices;
    uint32_t maxVertexBlendMatrixIndex;

    // Memory info
    uint32_t availableTextureMemory;
    uint32_t availableVideoMemory;

    // Backend info
    bgfx::RendererType::Enum rendererType;
    bool isOriginBottomLeft;
    bool homogeneousDepth;
};

// =============================================================================
// Capability Checker
// =============================================================================

class CapsChecker {
public:
    // Initialize and query device capabilities
    static void Initialize();

    // Get capabilities structure
    static const DeviceCaps& GetCaps();

    // Check specific features
    static bool SupportsTextureFormat(bgfx::TextureFormat::Enum format);
    static bool SupportsVertexAttrib(bgfx::Attrib::Enum attrib);

    // D3D8 style caps queries
    static bool CheckDeviceFormat(
        D3DFORMAT adapterFormat,
        DWORD usage,
        D3DRESOURCETYPE rtype,
        D3DFORMAT checkFormat
    );

    // Get D3D8 caps structure (partial implementation)
    static void GetD3DCaps8(D3DCAPS8& caps);

    // Texture format support queries
    static bool IsTextureFormatSupported(D3DFORMAT format, DWORD usage);
    static bool IsRenderTargetFormatSupported(D3DFORMAT format);
    static bool IsDepthStencilFormatSupported(D3DFORMAT format);

    // Print capabilities to log
    static void PrintCaps();

private:
    static DeviceCaps s_caps;
    static bool s_initialized;

    static void QueryBgfxCaps();
    static void QueryTextureSupport();
};

// =============================================================================
// D3DCAPS8 Structure (Simplified)
// =============================================================================

struct D3DCAPS8 {
    D3DDEVTYPE DeviceType;
    UINT AdapterOrdinal;

    DWORD Caps;
    DWORD Caps2;
    DWORD Caps3;
    DWORD PresentationIntervals;

    DWORD CursorCaps;
    DWORD DevCaps;

    DWORD PrimitiveMiscCaps;
    DWORD RasterCaps;
    DWORD ZCmpCaps;
    DWORD SrcBlendCaps;
    DWORD DestBlendCaps;
    DWORD AlphaCmpCaps;
    DWORD ShadeCaps;
    DWORD TextureCaps;
    DWORD TextureFilterCaps;
    DWORD CubeTextureFilterCaps;
    DWORD VolumeTextureFilterCaps;
    DWORD TextureAddressCaps;
    DWORD VolumeTextureAddressCaps;

    DWORD LineCaps;

    DWORD MaxTextureWidth;
    DWORD MaxTextureHeight;
    DWORD MaxVolumeExtent;
    DWORD MaxTextureRepeat;
    DWORD MaxTextureAspectRatio;
    DWORD MaxAnisotropy;
    float MaxVertexW;

    float GuardBandLeft;
    float GuardBandTop;
    float GuardBandRight;
    float GuardBandBottom;

    float ExtentsAdjust;
    DWORD StencilCaps;

    DWORD FVFCaps;
    DWORD TextureOpCaps;
    DWORD MaxTextureBlendStages;
    DWORD MaxSimultaneousTextures;

    DWORD VertexProcessingCaps;
    DWORD MaxActiveLights;
    DWORD MaxUserClipPlanes;
    DWORD MaxVertexBlendMatrices;
    DWORD MaxVertexBlendMatrixIndex;

    float MaxPointSize;
    DWORD MaxPrimitiveCount;
    DWORD MaxVertexIndex;
    DWORD MaxStreams;
    DWORD MaxStreamStride;

    DWORD VertexShaderVersion;
    DWORD MaxVertexShaderConst;
    DWORD PixelShaderVersion;
    float MaxPixelShaderValue;
};

// Device type
enum D3DDEVTYPE {
    D3DDEVTYPE_HAL = 1,
    D3DDEVTYPE_REF = 2,
    D3DDEVTYPE_SW  = 3
};

// Device caps flags
constexpr DWORD D3DDEVCAPS_EXECUTESYSTEMMEMORY      = 0x00000010;
constexpr DWORD D3DDEVCAPS_EXECUTEVIDEOMEMORY       = 0x00000020;
constexpr DWORD D3DDEVCAPS_TLVERTEXSYSTEMMEMORY     = 0x00000040;
constexpr DWORD D3DDEVCAPS_TLVERTEXVIDEOMEMORY      = 0x00000080;
constexpr DWORD D3DDEVCAPS_TEXTURESYSTEMMEMORY      = 0x00000100;
constexpr DWORD D3DDEVCAPS_TEXTUREVIDEOMEMORY       = 0x00000200;
constexpr DWORD D3DDEVCAPS_DRAWPRIMTLVERTEX         = 0x00000400;
constexpr DWORD D3DDEVCAPS_CANRENDERAFTERFLIP       = 0x00000800;
constexpr DWORD D3DDEVCAPS_TEXTURENONLOCALVIDMEM    = 0x00001000;
constexpr DWORD D3DDEVCAPS_DRAWPRIMITIVES2          = 0x00002000;
constexpr DWORD D3DDEVCAPS_SEPARATETEXTUREMEMORIES  = 0x00004000;
constexpr DWORD D3DDEVCAPS_DRAWPRIMITIVES2EX        = 0x00008000;
constexpr DWORD D3DDEVCAPS_HWTRANSFORMANDLIGHT      = 0x00010000;
constexpr DWORD D3DDEVCAPS_CANBLTSYSTONONLOCAL      = 0x00020000;
constexpr DWORD D3DDEVCAPS_HWRASTERIZATION          = 0x00080000;
constexpr DWORD D3DDEVCAPS_PUREDEVICE               = 0x00100000;
constexpr DWORD D3DDEVCAPS_QUINTICRTPATCHES         = 0x00200000;
constexpr DWORD D3DDEVCAPS_RTPATCHES                = 0x00400000;
constexpr DWORD D3DDEVCAPS_RTPATCHHANDLEZERO        = 0x00800000;
constexpr DWORD D3DDEVCAPS_NPATCHES                 = 0x01000000;

// Texture caps
constexpr DWORD D3DPTEXTURECAPS_POW2                = 0x00000002;
constexpr DWORD D3DPTEXTURECAPS_ALPHA               = 0x00000004;
constexpr DWORD D3DPTEXTURECAPS_SQUAREONLY          = 0x00000020;
constexpr DWORD D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE = 0x00000040;
constexpr DWORD D3DPTEXTURECAPS_ALPHAPALETTE        = 0x00000080;
constexpr DWORD D3DPTEXTURECAPS_NONPOW2CONDITIONAL  = 0x00000100;
constexpr DWORD D3DPTEXTURECAPS_PROJECTED           = 0x00000400;
constexpr DWORD D3DPTEXTURECAPS_CUBEMAP             = 0x00000800;
constexpr DWORD D3DPTEXTURECAPS_VOLUMEMAP           = 0x00002000;
constexpr DWORD D3DPTEXTURECAPS_MIPMAP              = 0x00004000;
constexpr DWORD D3DPTEXTURECAPS_MIPVOLUMEMAP        = 0x00008000;
constexpr DWORD D3DPTEXTURECAPS_MIPCUBEMAP          = 0x00010000;
constexpr DWORD D3DPTEXTURECAPS_CUBEMAP_POW2        = 0x00020000;
constexpr DWORD D3DPTEXTURECAPS_VOLUMEMAP_POW2      = 0x00040000;

} // namespace dx8bgfx
