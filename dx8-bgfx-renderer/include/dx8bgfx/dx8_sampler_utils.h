#pragma once

#include "dx8_types.h"
#include "dx8_constants.h"
#include "dx8_state_manager.h"

#include <bgfx/bgfx.h>

namespace dx8bgfx {

// =============================================================================
// Sampler State Utilities
// =============================================================================

class SamplerUtils {
public:
    // Convert D3D8 texture address mode to bgfx
    static uint32_t D3DAddressModeToBgfx(D3DTEXTUREADDRESS mode);

    // Convert D3D8 texture filter to bgfx
    static uint32_t D3DFilterToBgfx(D3DTEXTUREFILTERTYPE filter, bool isMip = false);

    // Build complete bgfx sampler flags from state manager
    static uint32_t BuildSamplerFlags(const StateManager& state, uint32_t stage);

    // Build sampler flags from individual settings
    static uint32_t BuildSamplerFlags(
        D3DTEXTUREADDRESS addressU,
        D3DTEXTUREADDRESS addressV,
        D3DTEXTUREADDRESS addressW,
        D3DTEXTUREFILTERTYPE minFilter,
        D3DTEXTUREFILTERTYPE magFilter,
        D3DTEXTUREFILTERTYPE mipFilter,
        uint32_t maxAnisotropy = 1
    );

    // Get anisotropy flags
    static uint32_t GetAnisotropyFlags(uint32_t maxAnisotropy);

    // Check if format supports filtering
    static bool SupportsFiltering(bgfx::TextureFormat::Enum format);
};

// =============================================================================
// Texture Stage Setup
// =============================================================================

class TextureStageSetup {
public:
    // Apply texture stage state to bgfx for rendering
    static void Apply(
        Renderer& renderer,
        uint32_t stage,
        bgfx::TextureHandle texture,
        const StateManager& state
    );

    // Build texture transform matrix
    static D3DMATRIX BuildTextureMatrix(const StateManager& state, uint32_t stage);

    // Get texture coordinate generation mode
    static uint32_t GetTexCoordGenMode(DWORD tci);

    // Check if stage is enabled
    static bool IsStageEnabled(const StateManager& state, uint32_t stage);

    // Get number of active texture stages
    static uint32_t GetActiveStageCount(const StateManager& state);
};

// =============================================================================
// Render Target Management
// =============================================================================

struct RenderTargetDesc {
    uint16_t width;
    uint16_t height;
    bgfx::TextureFormat::Enum format;
    uint64_t flags;
    bool hasMips;
};

struct DepthStencilDesc {
    uint16_t width;
    uint16_t height;
    bgfx::TextureFormat::Enum format;
    uint64_t flags;
};

class RenderTargetManager {
public:
    RenderTargetManager();
    ~RenderTargetManager();

    // Initialize with default backbuffer size
    void Initialize(uint16_t width, uint16_t height);

    // Shutdown and release all resources
    void Shutdown();

    // Create a render target
    bgfx::FrameBufferHandle CreateRenderTarget(const RenderTargetDesc& desc);

    // Create a render target with depth/stencil
    bgfx::FrameBufferHandle CreateRenderTargetWithDepth(
        const RenderTargetDesc& colorDesc,
        const DepthStencilDesc& depthDesc
    );

    // Get render target texture for use as input
    bgfx::TextureHandle GetRenderTargetTexture(bgfx::FrameBufferHandle fb, uint8_t attachment = 0);

    // Set current render target
    void SetRenderTarget(bgfx::ViewId viewId, bgfx::FrameBufferHandle fb);

    // Reset to backbuffer
    void SetBackbuffer(bgfx::ViewId viewId);

    // Destroy a render target
    void DestroyRenderTarget(bgfx::FrameBufferHandle& fb);

    // Get backbuffer dimensions
    uint16_t GetBackbufferWidth() const { return m_backbufferWidth; }
    uint16_t GetBackbufferHeight() const { return m_backbufferHeight; }

    // Resize backbuffer
    void ResizeBackbuffer(uint16_t width, uint16_t height);

private:
    uint16_t m_backbufferWidth;
    uint16_t m_backbufferHeight;
};

// =============================================================================
// Multi-Render Target (MRT) Support
// =============================================================================

class MRTManager {
public:
    static const uint32_t MaxRenderTargets = 4;

    // Create MRT framebuffer
    static bgfx::FrameBufferHandle CreateMRT(
        const RenderTargetDesc* descs,
        uint32_t count,
        const DepthStencilDesc* depthDesc = nullptr
    );

    // Get individual texture from MRT
    static bgfx::TextureHandle GetMRTTexture(
        bgfx::FrameBufferHandle fb,
        uint32_t index
    );
};

// =============================================================================
// Shadow Map Utilities
// =============================================================================

class ShadowMapUtils {
public:
    // Create shadow map framebuffer (depth only)
    static bgfx::FrameBufferHandle CreateShadowMap(
        uint16_t size,
        bgfx::TextureFormat::Enum format = bgfx::TextureFormat::D16
    );

    // Get optimal shadow map format for current renderer
    static bgfx::TextureFormat::Enum GetOptimalShadowFormat();

    // Build shadow map projection matrix
    static D3DMATRIX BuildShadowProjection(
        float left, float right,
        float bottom, float top,
        float nearZ, float farZ
    );

    // Build shadow map view matrix from light
    static D3DMATRIX BuildShadowView(const D3DLIGHT8& light, const D3DVECTOR& target);
};

} // namespace dx8bgfx
