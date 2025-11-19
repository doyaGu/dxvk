#include "dx8bgfx/dx8_caps.h"
#include <cstring>
#include <cstdio>

namespace dx8bgfx {

// Static members
DeviceCaps CapsChecker::s_caps = {};
bool CapsChecker::s_initialized = false;

void CapsChecker::Initialize() {
    if (s_initialized) return;

    std::memset(&s_caps, 0, sizeof(s_caps));
    QueryBgfxCaps();
    QueryTextureSupport();
    s_initialized = true;
}

const DeviceCaps& CapsChecker::GetCaps() {
    if (!s_initialized) {
        Initialize();
    }
    return s_caps;
}

void CapsChecker::QueryBgfxCaps() {
    const bgfx::Caps* caps = bgfx::getCaps();
    if (!caps) return;

    // Device info
    s_caps.vendorId = caps->vendorId;
    s_caps.deviceId = caps->deviceId;
    s_caps.rendererType = caps->rendererType;

    // Get renderer name
    const char* rendererName = bgfx::getRendererName(caps->rendererType);
    if (rendererName) {
        std::strncpy(s_caps.renderer, rendererName, sizeof(s_caps.renderer) - 1);
    }

    // Texture limits
    s_caps.maxTextureWidth = caps->limits.maxTextureSize;
    s_caps.maxTextureHeight = caps->limits.maxTextureSize;
    s_caps.maxTextureDepth = caps->limits.maxTexture3DSize;
    s_caps.maxTextureLayers = caps->limits.maxTextureLayers;
    s_caps.maxTextureStages = 8;
    s_caps.maxSimultaneousTextures = 8;

    // Vertex limits
    s_caps.maxVertexStreams = caps->limits.maxVertexStreams;
    s_caps.maxVertexIndex = 0xFFFFFF; // 24-bit
    s_caps.maxPrimitiveCount = caps->limits.maxDrawCalls;

    // Shader support
    s_caps.supportsVertexShaders = true;
    s_caps.supportsPixelShaders = true;
    s_caps.maxVertexShaderConsts = caps->limits.maxUniforms;
    s_caps.maxPixelShaderConsts = caps->limits.maxUniforms;

    // Features
    s_caps.supportsHardwareTnL = true;
    s_caps.supportsMipmaps = true;
    s_caps.supportsCubeTextures = (caps->supported & BGFX_CAPS_TEXTURE_CUBE_ARRAY) != 0;
    s_caps.supportsVolumeTextures = (caps->supported & BGFX_CAPS_TEXTURE_3D) != 0;
    s_caps.supportsAnisotropicFiltering = true;
    s_caps.supportsMultisampling = (caps->supported & BGFX_CAPS_SWAP_CHAIN) != 0;
    s_caps.supportsOcclusionQuery = (caps->supported & BGFX_CAPS_OCCLUSION_QUERY) != 0;
    s_caps.supportsInstancing = (caps->supported & BGFX_CAPS_INSTANCING) != 0;
    s_caps.supportsCompute = (caps->supported & BGFX_CAPS_COMPUTE) != 0;

    // Render targets
    s_caps.maxRenderTargets = caps->limits.maxFBAttachments;
    s_caps.maxColorAttachments = caps->limits.maxFBAttachments;
    s_caps.supportsFloatRenderTargets = true;
    s_caps.supportsSRGBRenderTargets = true;

    // Limits
    s_caps.maxAnisotropy = 16;
    s_caps.maxLights = 8;
    s_caps.maxClipPlanes = 6;
    s_caps.maxUserClipPlanes = 6;
    s_caps.maxPointSize = 256.0f;
    s_caps.maxVertexBlendMatrices = 4;
    s_caps.maxVertexBlendMatrixIndex = 255;

    // Coordinate system
    s_caps.isOriginBottomLeft = caps->originBottomLeft;
    s_caps.homogeneousDepth = caps->homogeneousDepth;
}

void CapsChecker::QueryTextureSupport() {
    const bgfx::Caps* caps = bgfx::getCaps();
    if (!caps) return;

    // Check texture format support
    s_caps.supportsBC1 = (caps->formats[bgfx::TextureFormat::BC1] & BGFX_CAPS_FORMAT_TEXTURE_2D) != 0;
    s_caps.supportsBC2 = (caps->formats[bgfx::TextureFormat::BC2] & BGFX_CAPS_FORMAT_TEXTURE_2D) != 0;
    s_caps.supportsBC3 = (caps->formats[bgfx::TextureFormat::BC3] & BGFX_CAPS_FORMAT_TEXTURE_2D) != 0;
    s_caps.supportsBC4 = (caps->formats[bgfx::TextureFormat::BC4] & BGFX_CAPS_FORMAT_TEXTURE_2D) != 0;
    s_caps.supportsBC5 = (caps->formats[bgfx::TextureFormat::BC5] & BGFX_CAPS_FORMAT_TEXTURE_2D) != 0;
    s_caps.supportsBC6H = (caps->formats[bgfx::TextureFormat::BC6H] & BGFX_CAPS_FORMAT_TEXTURE_2D) != 0;
    s_caps.supportsBC7 = (caps->formats[bgfx::TextureFormat::BC7] & BGFX_CAPS_FORMAT_TEXTURE_2D) != 0;
    s_caps.supportsETC1 = (caps->formats[bgfx::TextureFormat::ETC1] & BGFX_CAPS_FORMAT_TEXTURE_2D) != 0;
    s_caps.supportsETC2 = (caps->formats[bgfx::TextureFormat::ETC2] & BGFX_CAPS_FORMAT_TEXTURE_2D) != 0;
    s_caps.supportsPVRTC = false; // Not commonly supported
    s_caps.supportsASTC = (caps->formats[bgfx::TextureFormat::ASTC4x4] & BGFX_CAPS_FORMAT_TEXTURE_2D) != 0;
}

bool CapsChecker::SupportsTextureFormat(bgfx::TextureFormat::Enum format) {
    const bgfx::Caps* caps = bgfx::getCaps();
    if (!caps) return false;
    return (caps->formats[format] & BGFX_CAPS_FORMAT_TEXTURE_2D) != 0;
}

bool CapsChecker::SupportsVertexAttrib(bgfx::Attrib::Enum attrib) {
    (void)attrib;
    return true; // All standard attribs are supported
}

bool CapsChecker::IsTextureFormatSupported(D3DFORMAT format, DWORD usage) {
    (void)usage;

    switch (format) {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
        case D3DFMT_R5G6B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
        case D3DFMT_A8:
        case D3DFMT_L8:
        case D3DFMT_A8L8:
            return true;

        case D3DFMT_DXT1: return s_caps.supportsBC1;
        case D3DFMT_DXT3: return s_caps.supportsBC2;
        case D3DFMT_DXT5: return s_caps.supportsBC3;

        default:
            return true;
    }
}

bool CapsChecker::IsRenderTargetFormatSupported(D3DFORMAT format) {
    switch (format) {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
        case D3DFMT_R5G6B5:
            return true;
        default:
            return false;
    }
}

bool CapsChecker::IsDepthStencilFormatSupported(D3DFORMAT format) {
    switch (format) {
        case D3DFMT_D16:
        case D3DFMT_D24S8:
        case D3DFMT_D24X8:
        case D3DFMT_D32:
            return true;
        default:
            return false;
    }
}

void CapsChecker::GetD3DCaps8(D3DCAPS8& caps) {
    if (!s_initialized) Initialize();

    std::memset(&caps, 0, sizeof(caps));

    caps.DeviceType = D3DDEVTYPE_HAL;
    caps.AdapterOrdinal = 0;

    // Device caps
    caps.DevCaps = D3DDEVCAPS_HWTRANSFORMANDLIGHT |
                   D3DDEVCAPS_DRAWPRIMTLVERTEX |
                   D3DDEVCAPS_HWRASTERIZATION |
                   D3DDEVCAPS_TEXTUREVIDEOMEMORY |
                   D3DDEVCAPS_DRAWPRIMITIVES2EX;

    // Texture caps
    caps.TextureCaps = D3DPTEXTURECAPS_ALPHA |
                       D3DPTEXTURECAPS_MIPMAP |
                       D3DPTEXTURECAPS_PROJECTED;

    if (s_caps.supportsCubeTextures) {
        caps.TextureCaps |= D3DPTEXTURECAPS_CUBEMAP | D3DPTEXTURECAPS_MIPCUBEMAP;
    }
    if (s_caps.supportsVolumeTextures) {
        caps.TextureCaps |= D3DPTEXTURECAPS_VOLUMEMAP | D3DPTEXTURECAPS_MIPVOLUMEMAP;
    }

    // Limits
    caps.MaxTextureWidth = s_caps.maxTextureWidth;
    caps.MaxTextureHeight = s_caps.maxTextureHeight;
    caps.MaxVolumeExtent = s_caps.maxTextureDepth;
    caps.MaxTextureRepeat = 8192;
    caps.MaxTextureAspectRatio = 8192;
    caps.MaxAnisotropy = s_caps.maxAnisotropy;
    caps.MaxVertexW = 1e10f;

    // Stencil caps
    caps.StencilCaps = 0xFF;

    // Texture stage caps
    caps.MaxTextureBlendStages = s_caps.maxTextureStages;
    caps.MaxSimultaneousTextures = s_caps.maxSimultaneousTextures;

    // Vertex processing
    caps.MaxActiveLights = s_caps.maxLights;
    caps.MaxUserClipPlanes = s_caps.maxUserClipPlanes;
    caps.MaxVertexBlendMatrices = s_caps.maxVertexBlendMatrices;
    caps.MaxVertexBlendMatrixIndex = s_caps.maxVertexBlendMatrixIndex;

    // Primitive limits
    caps.MaxPointSize = s_caps.maxPointSize;
    caps.MaxPrimitiveCount = s_caps.maxPrimitiveCount;
    caps.MaxVertexIndex = s_caps.maxVertexIndex;
    caps.MaxStreams = s_caps.maxVertexStreams;
    caps.MaxStreamStride = 256;

    // Shader versions (D3D8 style)
    caps.VertexShaderVersion = 0x0101; // vs_1_1
    caps.MaxVertexShaderConst = s_caps.maxVertexShaderConsts;
    caps.PixelShaderVersion = 0x0104;  // ps_1_4
    caps.MaxPixelShaderValue = 1.0f;
}

void CapsChecker::PrintCaps() {
    if (!s_initialized) Initialize();

    printf("=== Device Capabilities ===\n");
    printf("Renderer: %s\n", s_caps.renderer);
    printf("Vendor ID: 0x%04X, Device ID: 0x%04X\n", s_caps.vendorId, s_caps.deviceId);
    printf("\nTexture Limits:\n");
    printf("  Max Size: %u x %u\n", s_caps.maxTextureWidth, s_caps.maxTextureHeight);
    printf("  Max 3D: %u\n", s_caps.maxTextureDepth);
    printf("  Max Stages: %u\n", s_caps.maxTextureStages);
    printf("\nFeature Support:\n");
    printf("  Hardware T&L: %s\n", s_caps.supportsHardwareTnL ? "Yes" : "No");
    printf("  Cube Textures: %s\n", s_caps.supportsCubeTextures ? "Yes" : "No");
    printf("  Volume Textures: %s\n", s_caps.supportsVolumeTextures ? "Yes" : "No");
    printf("  Instancing: %s\n", s_caps.supportsInstancing ? "Yes" : "No");
    printf("  Compute: %s\n", s_caps.supportsCompute ? "Yes" : "No");
    printf("\nCompressed Formats:\n");
    printf("  BC1 (DXT1): %s\n", s_caps.supportsBC1 ? "Yes" : "No");
    printf("  BC2 (DXT3): %s\n", s_caps.supportsBC2 ? "Yes" : "No");
    printf("  BC3 (DXT5): %s\n", s_caps.supportsBC3 ? "Yes" : "No");
    printf("  BC7: %s\n", s_caps.supportsBC7 ? "Yes" : "No");
    printf("\nLimits:\n");
    printf("  Max Anisotropy: %u\n", s_caps.maxAnisotropy);
    printf("  Max Lights: %u\n", s_caps.maxLights);
    printf("  Max Clip Planes: %u\n", s_caps.maxClipPlanes);
    printf("  Max Render Targets: %u\n", s_caps.maxRenderTargets);
    printf("===========================\n");
}

} // namespace dx8bgfx
