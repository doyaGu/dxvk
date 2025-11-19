#include "dx8bgfx/dx8_sampler_utils.h"
#include "dx8bgfx/dx8_math.h"
#include <cmath>

namespace dx8bgfx {

// =============================================================================
// Sampler Utils Implementation
// =============================================================================

uint32_t SamplerUtils::D3DAddressModeToBgfx(D3DTEXTUREADDRESS mode) {
    switch (mode) {
        case D3DTADDRESS_WRAP:       return BGFX_SAMPLER_NONE;
        case D3DTADDRESS_MIRROR:     return BGFX_SAMPLER_U_MIRROR | BGFX_SAMPLER_V_MIRROR;
        case D3DTADDRESS_CLAMP:      return BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
        case D3DTADDRESS_BORDER:     return BGFX_SAMPLER_U_BORDER | BGFX_SAMPLER_V_BORDER;
        case D3DTADDRESS_MIRRORONCE: return BGFX_SAMPLER_U_MIRROR | BGFX_SAMPLER_V_MIRROR;
        default:                     return BGFX_SAMPLER_NONE;
    }
}

uint32_t SamplerUtils::D3DFilterToBgfx(D3DTEXTUREFILTERTYPE filter, bool isMip) {
    if (isMip) {
        switch (filter) {
            case D3DTEXF_NONE:   return BGFX_SAMPLER_MIP_POINT;
            case D3DTEXF_POINT:  return BGFX_SAMPLER_MIP_POINT;
            case D3DTEXF_LINEAR: return BGFX_SAMPLER_NONE; // Linear mip is default
            default:             return BGFX_SAMPLER_NONE;
        }
    } else {
        switch (filter) {
            case D3DTEXF_POINT:       return BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT;
            case D3DTEXF_LINEAR:      return BGFX_SAMPLER_NONE; // Linear is default
            case D3DTEXF_ANISOTROPIC: return BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC;
            default:                  return BGFX_SAMPLER_NONE;
        }
    }
}

uint32_t SamplerUtils::BuildSamplerFlags(const StateManager& state, uint32_t stage) {
    const SamplerState& sampler = state.GetSampler(stage);

    return BuildSamplerFlags(
        static_cast<D3DTEXTUREADDRESS>(sampler.AddressU),
        static_cast<D3DTEXTUREADDRESS>(sampler.AddressV),
        static_cast<D3DTEXTUREADDRESS>(sampler.AddressW),
        static_cast<D3DTEXTUREFILTERTYPE>(sampler.MinFilter),
        static_cast<D3DTEXTUREFILTERTYPE>(sampler.MagFilter),
        static_cast<D3DTEXTUREFILTERTYPE>(sampler.MipFilter),
        sampler.MaxAnisotropy
    );
}

uint32_t SamplerUtils::BuildSamplerFlags(
    D3DTEXTUREADDRESS addressU,
    D3DTEXTUREADDRESS addressV,
    D3DTEXTUREADDRESS addressW,
    D3DTEXTUREFILTERTYPE minFilter,
    D3DTEXTUREFILTERTYPE magFilter,
    D3DTEXTUREFILTERTYPE mipFilter,
    uint32_t maxAnisotropy
) {
    uint32_t flags = 0;

    // Address modes
    switch (addressU) {
        case D3DTADDRESS_MIRROR: flags |= BGFX_SAMPLER_U_MIRROR; break;
        case D3DTADDRESS_CLAMP:  flags |= BGFX_SAMPLER_U_CLAMP; break;
        case D3DTADDRESS_BORDER: flags |= BGFX_SAMPLER_U_BORDER; break;
        default: break;
    }

    switch (addressV) {
        case D3DTADDRESS_MIRROR: flags |= BGFX_SAMPLER_V_MIRROR; break;
        case D3DTADDRESS_CLAMP:  flags |= BGFX_SAMPLER_V_CLAMP; break;
        case D3DTADDRESS_BORDER: flags |= BGFX_SAMPLER_V_BORDER; break;
        default: break;
    }

    switch (addressW) {
        case D3DTADDRESS_MIRROR: flags |= BGFX_SAMPLER_W_MIRROR; break;
        case D3DTADDRESS_CLAMP:  flags |= BGFX_SAMPLER_W_CLAMP; break;
        case D3DTADDRESS_BORDER: flags |= BGFX_SAMPLER_W_BORDER; break;
        default: break;
    }

    // Min filter
    if (minFilter == D3DTEXF_POINT) {
        flags |= BGFX_SAMPLER_MIN_POINT;
    } else if (minFilter == D3DTEXF_ANISOTROPIC) {
        flags |= BGFX_SAMPLER_MIN_ANISOTROPIC;
    }

    // Mag filter
    if (magFilter == D3DTEXF_POINT) {
        flags |= BGFX_SAMPLER_MAG_POINT;
    } else if (magFilter == D3DTEXF_ANISOTROPIC) {
        flags |= BGFX_SAMPLER_MAG_ANISOTROPIC;
    }

    // Mip filter
    if (mipFilter == D3DTEXF_NONE || mipFilter == D3DTEXF_POINT) {
        flags |= BGFX_SAMPLER_MIP_POINT;
    }

    // Anisotropy
    if (maxAnisotropy > 1 && (minFilter == D3DTEXF_ANISOTROPIC || magFilter == D3DTEXF_ANISOTROPIC)) {
        flags |= GetAnisotropyFlags(maxAnisotropy);
    }

    return flags;
}

uint32_t SamplerUtils::GetAnisotropyFlags(uint32_t maxAnisotropy) {
    // bgfx doesn't have explicit anisotropy level flags
    // The anisotropic flag enables max hardware anisotropy
    if (maxAnisotropy > 1) {
        return BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC;
    }
    return 0;
}

bool SamplerUtils::SupportsFiltering(bgfx::TextureFormat::Enum format) {
    // Most formats support filtering, but some don't
    switch (format) {
        case bgfx::TextureFormat::R32U:
        case bgfx::TextureFormat::R32I:
        case bgfx::TextureFormat::RG32U:
        case bgfx::TextureFormat::RG32I:
        case bgfx::TextureFormat::RGBA32U:
        case bgfx::TextureFormat::RGBA32I:
            return false;
        default:
            return true;
    }
}

// =============================================================================
// Texture Stage Setup Implementation
// =============================================================================

bool TextureStageSetup::IsStageEnabled(const StateManager& state, uint32_t stage) {
    const TextureStageState& tss = state.GetTextureStage(stage);
    return tss.ColorOp != D3DTOP_DISABLE;
}

uint32_t TextureStageSetup::GetActiveStageCount(const StateManager& state) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < MaxTextureStages; i++) {
        if (IsStageEnabled(state, i)) {
            count = i + 1;
        } else {
            break;
        }
    }
    return count;
}

D3DMATRIX TextureStageSetup::BuildTextureMatrix(const StateManager& state, uint32_t stage) {
    return state.GetTextureMatrix(stage);
}

uint32_t TextureStageSetup::GetTexCoordGenMode(DWORD tci) {
    return tci & 0xFFFF0000; // Upper bits contain TCI flags
}

// =============================================================================
// Render Target Manager Implementation
// =============================================================================

RenderTargetManager::RenderTargetManager()
    : m_backbufferWidth(0)
    , m_backbufferHeight(0)
{
}

RenderTargetManager::~RenderTargetManager() {
    Shutdown();
}

void RenderTargetManager::Initialize(uint16_t width, uint16_t height) {
    m_backbufferWidth = width;
    m_backbufferHeight = height;
}

void RenderTargetManager::Shutdown() {
    // Nothing to clean up - framebuffers are managed externally
}

bgfx::FrameBufferHandle RenderTargetManager::CreateRenderTarget(const RenderTargetDesc& desc) {
    bgfx::TextureHandle texture = bgfx::createTexture2D(
        desc.width, desc.height,
        desc.hasMips, 1,
        desc.format,
        BGFX_TEXTURE_RT | desc.flags
    );

    return bgfx::createFrameBuffer(1, &texture, true);
}

bgfx::FrameBufferHandle RenderTargetManager::CreateRenderTargetWithDepth(
    const RenderTargetDesc& colorDesc,
    const DepthStencilDesc& depthDesc
) {
    bgfx::TextureHandle textures[2];

    // Color attachment
    textures[0] = bgfx::createTexture2D(
        colorDesc.width, colorDesc.height,
        colorDesc.hasMips, 1,
        colorDesc.format,
        BGFX_TEXTURE_RT | colorDesc.flags
    );

    // Depth attachment
    textures[1] = bgfx::createTexture2D(
        depthDesc.width, depthDesc.height,
        false, 1,
        depthDesc.format,
        BGFX_TEXTURE_RT | depthDesc.flags
    );

    return bgfx::createFrameBuffer(2, textures, true);
}

bgfx::TextureHandle RenderTargetManager::GetRenderTargetTexture(
    bgfx::FrameBufferHandle fb,
    uint8_t attachment
) {
    return bgfx::getTexture(fb, attachment);
}

void RenderTargetManager::SetRenderTarget(bgfx::ViewId viewId, bgfx::FrameBufferHandle fb) {
    bgfx::setViewFrameBuffer(viewId, fb);
}

void RenderTargetManager::SetBackbuffer(bgfx::ViewId viewId) {
    bgfx::setViewFrameBuffer(viewId, BGFX_INVALID_HANDLE);
}

void RenderTargetManager::DestroyRenderTarget(bgfx::FrameBufferHandle& fb) {
    if (bgfx::isValid(fb)) {
        bgfx::destroy(fb);
        fb = BGFX_INVALID_HANDLE;
    }
}

void RenderTargetManager::ResizeBackbuffer(uint16_t width, uint16_t height) {
    m_backbufferWidth = width;
    m_backbufferHeight = height;
    bgfx::reset(width, height, BGFX_RESET_VSYNC);
}

// =============================================================================
// MRT Manager Implementation
// =============================================================================

bgfx::FrameBufferHandle MRTManager::CreateMRT(
    const RenderTargetDesc* descs,
    uint32_t count,
    const DepthStencilDesc* depthDesc
) {
    if (count == 0 || count > MaxRenderTargets) {
        return BGFX_INVALID_HANDLE;
    }

    bgfx::TextureHandle textures[MaxRenderTargets + 1];
    uint32_t numTextures = 0;

    // Create color attachments
    for (uint32_t i = 0; i < count; i++) {
        textures[numTextures++] = bgfx::createTexture2D(
            descs[i].width, descs[i].height,
            descs[i].hasMips, 1,
            descs[i].format,
            BGFX_TEXTURE_RT | descs[i].flags
        );
    }

    // Create depth attachment if requested
    if (depthDesc) {
        textures[numTextures++] = bgfx::createTexture2D(
            depthDesc->width, depthDesc->height,
            false, 1,
            depthDesc->format,
            BGFX_TEXTURE_RT | depthDesc->flags
        );
    }

    return bgfx::createFrameBuffer(static_cast<uint8_t>(numTextures), textures, true);
}

bgfx::TextureHandle MRTManager::GetMRTTexture(bgfx::FrameBufferHandle fb, uint32_t index) {
    return bgfx::getTexture(fb, static_cast<uint8_t>(index));
}

// =============================================================================
// Shadow Map Utils Implementation
// =============================================================================

bgfx::FrameBufferHandle ShadowMapUtils::CreateShadowMap(
    uint16_t size,
    bgfx::TextureFormat::Enum format
) {
    bgfx::TextureHandle texture = bgfx::createTexture2D(
        size, size,
        false, 1,
        format,
        BGFX_TEXTURE_RT | BGFX_SAMPLER_COMPARE_LEQUAL
    );

    return bgfx::createFrameBuffer(1, &texture, true);
}

bgfx::TextureFormat::Enum ShadowMapUtils::GetOptimalShadowFormat() {
    // D16 is widely supported
    return bgfx::TextureFormat::D16;
}

D3DMATRIX ShadowMapUtils::BuildShadowProjection(
    float left, float right,
    float bottom, float top,
    float nearZ, float farZ
) {
    D3DMATRIX proj;
    MatrixIdentity(&proj);

    // Orthographic projection for directional lights
    proj._11 = 2.0f / (right - left);
    proj._22 = 2.0f / (top - bottom);
    proj._33 = 1.0f / (farZ - nearZ);
    proj._41 = (left + right) / (left - right);
    proj._42 = (top + bottom) / (bottom - top);
    proj._43 = nearZ / (nearZ - farZ);

    return proj;
}

D3DMATRIX ShadowMapUtils::BuildShadowView(const D3DLIGHT8& light, const D3DVECTOR& target) {
    D3DMATRIX view;
    MatrixIdentity(&view);

    if (light.Type == D3DLIGHT_DIRECTIONAL) {
        // Look from target in opposite direction of light
        float dist = 100.0f; // Far enough to encompass scene
        float eyeX = target.x - light.Direction.x * dist;
        float eyeY = target.y - light.Direction.y * dist;
        float eyeZ = target.z - light.Direction.z * dist;

        // Build look-at matrix
        float fx = light.Direction.x;
        float fy = light.Direction.y;
        float fz = light.Direction.z;

        // Normalize forward
        float flen = std::sqrt(fx*fx + fy*fy + fz*fz);
        if (flen > 0.0001f) {
            fx /= flen; fy /= flen; fz /= flen;
        }

        // Up vector
        float ux = 0.0f, uy = 1.0f, uz = 0.0f;
        if (std::abs(fy) > 0.99f) {
            ux = 0.0f; uy = 0.0f; uz = 1.0f;
        }

        // Right = up x forward
        float rx = uy * fz - uz * fy;
        float ry = uz * fx - ux * fz;
        float rz = ux * fy - uy * fx;
        float rlen = std::sqrt(rx*rx + ry*ry + rz*rz);
        if (rlen > 0.0001f) {
            rx /= rlen; ry /= rlen; rz /= rlen;
        }

        // Recalculate up = forward x right
        ux = fy * rz - fz * ry;
        uy = fz * rx - fx * rz;
        uz = fx * ry - fy * rx;

        view._11 = rx; view._12 = ux; view._13 = fx; view._14 = 0;
        view._21 = ry; view._22 = uy; view._23 = fy; view._24 = 0;
        view._31 = rz; view._32 = uz; view._33 = fz; view._34 = 0;
        view._41 = -(rx*eyeX + ry*eyeY + rz*eyeZ);
        view._42 = -(ux*eyeX + uy*eyeY + uz*eyeZ);
        view._43 = -(fx*eyeX + fy*eyeY + fz*eyeZ);
        view._44 = 1;
    }

    return view;
}

} // namespace dx8bgfx
