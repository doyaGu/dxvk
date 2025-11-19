#pragma once

#include "dx8_types.h"
#include "dx8_constants.h"
#include "dx8_shader_key.h"
#include <array>
#include <bitset>

namespace dx8bgfx {

// =============================================================================
// Texture Stage State
// =============================================================================

struct TextureStageState {
    DWORD ColorOp = D3DTOP_DISABLE;
    DWORD ColorArg0 = D3DTA_CURRENT;
    DWORD ColorArg1 = D3DTA_TEXTURE;
    DWORD ColorArg2 = D3DTA_CURRENT;
    DWORD AlphaOp = D3DTOP_DISABLE;
    DWORD AlphaArg0 = D3DTA_CURRENT;
    DWORD AlphaArg1 = D3DTA_TEXTURE;
    DWORD AlphaArg2 = D3DTA_CURRENT;
    DWORD ResultArg = D3DTA_CURRENT;
    DWORD TexCoordIndex = 0;
    DWORD TextureTransformFlags = D3DTTFF_DISABLE;

    // Bump mapping
    float BumpEnvMat00 = 0.0f;
    float BumpEnvMat01 = 0.0f;
    float BumpEnvMat10 = 0.0f;
    float BumpEnvMat11 = 0.0f;
    float BumpEnvLScale = 0.0f;
    float BumpEnvLOffset = 0.0f;

    // Stage constant (D3D9 feature, but useful)
    D3DCOLORVALUE Constant = {0, 0, 0, 0};
};

// =============================================================================
// Sampler State
// =============================================================================

struct SamplerState {
    DWORD AddressU = D3DTADDRESS_WRAP;
    DWORD AddressV = D3DTADDRESS_WRAP;
    DWORD AddressW = D3DTADDRESS_WRAP;
    DWORD MagFilter = D3DTEXF_POINT;
    DWORD MinFilter = D3DTEXF_POINT;
    DWORD MipFilter = D3DTEXF_NONE;
    float MipMapLODBias = 0.0f;
    DWORD MaxMipLevel = 0;
    DWORD MaxAnisotropy = 1;
    D3DCOLOR BorderColor = 0;
};

// =============================================================================
// State Manager
// =============================================================================

class StateManager {
public:
    StateManager();
    ~StateManager() = default;

    // Reset to default state
    void Reset();

    // Transform management
    HRESULT SetTransform(D3DTRANSFORMSTATETYPE type, const D3DMATRIX* matrix);
    HRESULT GetTransform(D3DTRANSFORMSTATETYPE type, D3DMATRIX* matrix) const;

    // Light management
    HRESULT SetLight(DWORD index, const D3DLIGHT8* light);
    HRESULT GetLight(DWORD index, D3DLIGHT8* light) const;
    HRESULT LightEnable(DWORD index, BOOL enable);
    HRESULT GetLightEnable(DWORD index, BOOL* enable) const;

    // Material
    HRESULT SetMaterial(const D3DMATERIAL8* material);
    HRESULT GetMaterial(D3DMATERIAL8* material) const;

    // Render states
    HRESULT SetRenderState(D3DRENDERSTATETYPE state, DWORD value);
    HRESULT GetRenderState(D3DRENDERSTATETYPE state, DWORD* value) const;

    // Texture stage states
    HRESULT SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value);
    HRESULT GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD* value) const;

    // Sampler states
    HRESULT SetSamplerState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value);
    HRESULT GetSamplerState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD* value) const;

    // Texture binding
    void SetTextureHandle(DWORD stage, uint16_t handle);
    uint16_t GetTextureHandle(DWORD stage) const;
    bool HasTexture(DWORD stage) const;

    // Viewport
    void SetViewport(const D3DVIEWPORT8& viewport);
    const D3DVIEWPORT8& GetViewport() const;

    // FVF
    void SetFVF(DWORD fvf);
    DWORD GetFVF() const;

    // Clip planes
    HRESULT SetClipPlane(DWORD index, const float* plane);
    HRESULT GetClipPlane(DWORD index, float* plane) const;

    // Generate shader keys from current state
    VertexShaderKey BuildVertexShaderKey() const;
    FragmentShaderKey BuildFragmentShaderKey() const;
    ShaderKey BuildShaderKey() const;

    // State dirty tracking
    bool IsStateDirty() const { return m_dirty; }
    void ClearDirty() { m_dirty = false; }

    // Accessors for GPU data preparation
    const D3DMATRIX& GetWorldMatrix() const { return m_transforms[D3DTS_WORLD]; }
    const D3DMATRIX& GetViewMatrix() const { return m_transforms[D3DTS_VIEW]; }
    const D3DMATRIX& GetProjectionMatrix() const { return m_transforms[D3DTS_PROJECTION]; }
    const D3DMATRIX& GetTextureMatrix(DWORD stage) const;

    const D3DMATERIAL8& GetMaterial() const { return m_material; }
    const D3DLIGHT8& GetLight(DWORD index) const { return m_lights[index]; }
    bool IsLightEnabled(DWORD index) const { return m_lightEnabled[index]; }
    UINT GetEnabledLightCount() const;

    const TextureStageState& GetTextureStage(DWORD stage) const;
    const SamplerState& GetSampler(DWORD stage) const;

    // Helper methods
    D3DCOLORVALUE GetGlobalAmbient() const;
    float GetTweenFactor() const;

    // Check specific features
    bool IsLightingEnabled() const;
    bool IsSpecularEnabled() const;
    bool IsFogEnabled() const;
    bool IsAlphaTestEnabled() const;
    bool IsAlphaBlendEnabled() const;
    D3DFOGMODE GetFogMode() const;
    D3DFOGMODE GetVertexFogMode() const;
    bool IsRangeFogEnabled() const;
    bool IsLocalViewerEnabled() const;
    bool ShouldNormalizeNormals() const;
    D3DVERTEXBLENDFLAGS GetVertexBlendMode() const;

private:
    // Transforms
    std::array<D3DMATRIX, 512> m_transforms;

    // Lighting
    std::array<D3DLIGHT8, MaxLights> m_lights;
    std::bitset<MaxLights> m_lightEnabled;
    D3DMATERIAL8 m_material;

    // Render states
    std::array<DWORD, D3DRS_MAX> m_renderStates;

    // Texture stages
    std::array<TextureStageState, MaxTextureStages> m_textureStages;
    std::array<SamplerState, MaxTextureStages> m_samplers;
    std::array<uint16_t, MaxTextureStages> m_textureHandles;

    // Viewport
    D3DVIEWPORT8 m_viewport;

    // FVF
    DWORD m_fvf;

    // Clip planes
    std::array<float[4], MaxClipPlanes> m_clipPlanes;

    // State tracking
    bool m_dirty;

    // Initialize default states
    void InitializeDefaults();
};

} // namespace dx8bgfx
