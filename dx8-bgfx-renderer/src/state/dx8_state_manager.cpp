#include "dx8bgfx/dx8_state_manager.h"
#include <cmath>

namespace dx8bgfx {

StateManager::StateManager() {
    InitializeDefaults();
}

void StateManager::InitializeDefaults() {
    m_dirty = true;
    m_fvf = 0;

    // Initialize identity matrices
    D3DMATRIX identity;
    for (auto& m : m_transforms) {
        m = identity;
    }

    // Initialize viewport
    m_viewport = {0, 0, 640, 480, 0.0f, 1.0f};

    // Initialize default render states
    std::fill(m_renderStates.begin(), m_renderStates.end(), 0);

    m_renderStates[D3DRS_ZENABLE] = DefaultRenderState::ZEnable;
    m_renderStates[D3DRS_FILLMODE] = DefaultRenderState::FillMode;
    m_renderStates[D3DRS_SHADEMODE] = DefaultRenderState::ShadeMode;
    m_renderStates[D3DRS_ZWRITEENABLE] = DefaultRenderState::ZWriteEnable;
    m_renderStates[D3DRS_ALPHATESTENABLE] = DefaultRenderState::AlphaTestEnable;
    m_renderStates[D3DRS_SRCBLEND] = DefaultRenderState::SrcBlend;
    m_renderStates[D3DRS_DESTBLEND] = DefaultRenderState::DestBlend;
    m_renderStates[D3DRS_CULLMODE] = DefaultRenderState::CullMode;
    m_renderStates[D3DRS_ZFUNC] = DefaultRenderState::ZFunc;
    m_renderStates[D3DRS_ALPHAREF] = DefaultRenderState::AlphaRef;
    m_renderStates[D3DRS_ALPHAFUNC] = DefaultRenderState::AlphaFunc;
    m_renderStates[D3DRS_ALPHABLENDENABLE] = DefaultRenderState::AlphaBlendEnable;
    m_renderStates[D3DRS_FOGENABLE] = DefaultRenderState::FogEnable;
    m_renderStates[D3DRS_SPECULARENABLE] = DefaultRenderState::SpecularEnable;
    m_renderStates[D3DRS_FOGCOLOR] = DefaultRenderState::FogColor;
    m_renderStates[D3DRS_FOGTABLEMODE] = DefaultRenderState::FogTableMode;
    m_renderStates[D3DRS_FOGVERTEXMODE] = DefaultRenderState::FogVertexMode;
    m_renderStates[D3DRS_RANGEFOGENABLE] = DefaultRenderState::RangeFogEnable;
    m_renderStates[D3DRS_LIGHTING] = DefaultRenderState::Lighting;
    m_renderStates[D3DRS_AMBIENT] = DefaultRenderState::Ambient;
    m_renderStates[D3DRS_COLORVERTEX] = DefaultRenderState::ColorVertex;
    m_renderStates[D3DRS_LOCALVIEWER] = DefaultRenderState::LocalViewer;
    m_renderStates[D3DRS_NORMALIZENORMALS] = DefaultRenderState::NormalizeNormals;
    m_renderStates[D3DRS_DIFFUSEMATERIALSOURCE] = DefaultRenderState::DiffuseMaterialSource;
    m_renderStates[D3DRS_SPECULARMATERIALSOURCE] = DefaultRenderState::SpecularMaterialSource;
    m_renderStates[D3DRS_AMBIENTMATERIALSOURCE] = DefaultRenderState::AmbientMaterialSource;
    m_renderStates[D3DRS_EMISSIVEMATERIALSOURCE] = DefaultRenderState::EmissiveMaterialSource;
    m_renderStates[D3DRS_VERTEXBLEND] = DefaultRenderState::VertexBlend;
    m_renderStates[D3DRS_CLIPPING] = DefaultRenderState::Clipping;
    m_renderStates[D3DRS_BLENDOP] = D3DBLENDOP_ADD;

    // Default fog parameters
    float fogStart = 0.0f, fogEnd = 1.0f, fogDensity = 1.0f;
    m_renderStates[D3DRS_FOGSTART] = *reinterpret_cast<DWORD*>(&fogStart);
    m_renderStates[D3DRS_FOGEND] = *reinterpret_cast<DWORD*>(&fogEnd);
    m_renderStates[D3DRS_FOGDENSITY] = *reinterpret_cast<DWORD*>(&fogDensity);

    // Default point size
    float pointSize = 1.0f;
    m_renderStates[D3DRS_POINTSIZE] = *reinterpret_cast<DWORD*>(&pointSize);
    m_renderStates[D3DRS_POINTSIZE_MIN] = *reinterpret_cast<DWORD*>(&pointSize);
    float pointSizeMax = 64.0f;
    m_renderStates[D3DRS_POINTSIZE_MAX] = *reinterpret_cast<DWORD*>(&pointSizeMax);

    // Initialize texture stages
    for (DWORD i = 0; i < MaxTextureStages; i++) {
        m_textureStages[i] = TextureStageState();

        if (i == 0) {
            m_textureStages[i].ColorOp = D3DTOP_MODULATE;
            m_textureStages[i].AlphaOp = D3DTOP_SELECTARG1;
        }

        m_textureStages[i].TexCoordIndex = i;
        m_textureHandles[i] = UINT16_MAX;
        m_samplers[i] = SamplerState();
    }

    // Initialize lights
    for (DWORD i = 0; i < MaxLights; i++) {
        m_lights[i] = D3DLIGHT8{};
        m_lights[i].Type = D3DLIGHT_DIRECTIONAL;
        m_lights[i].Direction = {0.0f, 0.0f, 1.0f};
        m_lights[i].Diffuse = {1.0f, 1.0f, 1.0f, 1.0f};
        m_lights[i].Range = std::sqrt(FLT_MAX);
    }
    m_lightEnabled.reset();

    // Initialize material
    m_material = D3DMATERIAL8{};
    m_material.Diffuse = {1.0f, 1.0f, 1.0f, 1.0f};
    m_material.Ambient = {0.0f, 0.0f, 0.0f, 1.0f};
    m_material.Power = 0.0f;

    // Initialize clip planes
    for (auto& plane : m_clipPlanes) {
        plane[0] = plane[1] = plane[2] = plane[3] = 0.0f;
    }
}

void StateManager::Reset() {
    InitializeDefaults();
}

// =============================================================================
// Transform Management
// =============================================================================

HRESULT StateManager::SetTransform(D3DTRANSFORMSTATETYPE type, const D3DMATRIX* matrix) {
    if (!matrix) return D3DERR_INVALIDCALL;

    UINT index = static_cast<UINT>(type);
    if (index >= m_transforms.size()) return D3DERR_INVALIDCALL;

    m_transforms[index] = *matrix;
    m_dirty = true;
    return D3D_OK;
}

HRESULT StateManager::GetTransform(D3DTRANSFORMSTATETYPE type, D3DMATRIX* matrix) const {
    if (!matrix) return D3DERR_INVALIDCALL;

    UINT index = static_cast<UINT>(type);
    if (index >= m_transforms.size()) return D3DERR_INVALIDCALL;

    *matrix = m_transforms[index];
    return D3D_OK;
}

const D3DMATRIX& StateManager::GetTextureMatrix(DWORD stage) const {
    return m_transforms[D3DTS_TEXTURE0 + stage];
}

// =============================================================================
// Light Management
// =============================================================================

HRESULT StateManager::SetLight(DWORD index, const D3DLIGHT8* light) {
    if (index >= MaxLights || !light) return D3DERR_INVALIDCALL;

    m_lights[index] = *light;
    m_dirty = true;
    return D3D_OK;
}

HRESULT StateManager::GetLight(DWORD index, D3DLIGHT8* light) const {
    if (index >= MaxLights || !light) return D3DERR_INVALIDCALL;

    *light = m_lights[index];
    return D3D_OK;
}

HRESULT StateManager::LightEnable(DWORD index, BOOL enable) {
    if (index >= MaxLights) return D3DERR_INVALIDCALL;

    m_lightEnabled[index] = enable != FALSE;
    m_dirty = true;
    return D3D_OK;
}

HRESULT StateManager::GetLightEnable(DWORD index, BOOL* enable) const {
    if (index >= MaxLights || !enable) return D3DERR_INVALIDCALL;

    *enable = m_lightEnabled[index] ? TRUE : FALSE;
    return D3D_OK;
}

UINT StateManager::GetEnabledLightCount() const {
    return static_cast<UINT>(m_lightEnabled.count());
}

// =============================================================================
// Material
// =============================================================================

HRESULT StateManager::SetMaterial(const D3DMATERIAL8* material) {
    if (!material) return D3DERR_INVALIDCALL;

    m_material = *material;
    m_dirty = true;
    return D3D_OK;
}

HRESULT StateManager::GetMaterial(D3DMATERIAL8* material) const {
    if (!material) return D3DERR_INVALIDCALL;

    *material = m_material;
    return D3D_OK;
}

// =============================================================================
// Render States
// =============================================================================

HRESULT StateManager::SetRenderState(D3DRENDERSTATETYPE state, DWORD value) {
    if (state >= D3DRS_MAX) return D3DERR_INVALIDCALL;

    m_renderStates[state] = value;
    m_dirty = true;
    return D3D_OK;
}

HRESULT StateManager::GetRenderState(D3DRENDERSTATETYPE state, DWORD* value) const {
    if (state >= D3DRS_MAX || !value) return D3DERR_INVALIDCALL;

    *value = m_renderStates[state];
    return D3D_OK;
}

// =============================================================================
// Texture Stage States
// =============================================================================

HRESULT StateManager::SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value) {
    if (stage >= MaxTextureStages) return D3DERR_INVALIDCALL;

    TextureStageState& tss = m_textureStages[stage];

    switch (type) {
        case D3DTSS_COLOROP:        tss.ColorOp = value; break;
        case D3DTSS_COLORARG1:      tss.ColorArg1 = value; break;
        case D3DTSS_COLORARG2:      tss.ColorArg2 = value; break;
        case D3DTSS_COLORARG0:      tss.ColorArg0 = value; break;
        case D3DTSS_ALPHAOP:        tss.AlphaOp = value; break;
        case D3DTSS_ALPHAARG1:      tss.AlphaArg1 = value; break;
        case D3DTSS_ALPHAARG2:      tss.AlphaArg2 = value; break;
        case D3DTSS_ALPHAARG0:      tss.AlphaArg0 = value; break;
        case D3DTSS_RESULTARG:      tss.ResultArg = value; break;
        case D3DTSS_TEXCOORDINDEX:  tss.TexCoordIndex = value; break;
        case D3DTSS_TEXTURETRANSFORMFLAGS: tss.TextureTransformFlags = value; break;

        case D3DTSS_BUMPENVMAT00:   tss.BumpEnvMat00 = *reinterpret_cast<float*>(&value); break;
        case D3DTSS_BUMPENVMAT01:   tss.BumpEnvMat01 = *reinterpret_cast<float*>(&value); break;
        case D3DTSS_BUMPENVMAT10:   tss.BumpEnvMat10 = *reinterpret_cast<float*>(&value); break;
        case D3DTSS_BUMPENVMAT11:   tss.BumpEnvMat11 = *reinterpret_cast<float*>(&value); break;
        case D3DTSS_BUMPENVLSCALE:  tss.BumpEnvLScale = *reinterpret_cast<float*>(&value); break;
        case D3DTSS_BUMPENVLOFFSET: tss.BumpEnvLOffset = *reinterpret_cast<float*>(&value); break;

        // Sampler states in D3D8
        case D3DTSS_ADDRESSU:
        case D3DTSS_ADDRESSV:
        case D3DTSS_ADDRESSW:
        case D3DTSS_MAGFILTER:
        case D3DTSS_MINFILTER:
        case D3DTSS_MIPFILTER:
        case D3DTSS_MIPMAPLODBIAS:
        case D3DTSS_MAXMIPLEVEL:
        case D3DTSS_MAXANISOTROPY:
        case D3DTSS_BORDERCOLOR:
            return SetSamplerState(stage, type, value);

        default:
            return D3DERR_INVALIDCALL;
    }

    m_dirty = true;
    return D3D_OK;
}

HRESULT StateManager::GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD* value) const {
    if (stage >= MaxTextureStages || !value) return D3DERR_INVALIDCALL;

    const TextureStageState& tss = m_textureStages[stage];

    switch (type) {
        case D3DTSS_COLOROP:        *value = tss.ColorOp; break;
        case D3DTSS_COLORARG1:      *value = tss.ColorArg1; break;
        case D3DTSS_COLORARG2:      *value = tss.ColorArg2; break;
        case D3DTSS_COLORARG0:      *value = tss.ColorArg0; break;
        case D3DTSS_ALPHAOP:        *value = tss.AlphaOp; break;
        case D3DTSS_ALPHAARG1:      *value = tss.AlphaArg1; break;
        case D3DTSS_ALPHAARG2:      *value = tss.AlphaArg2; break;
        case D3DTSS_ALPHAARG0:      *value = tss.AlphaArg0; break;
        case D3DTSS_RESULTARG:      *value = tss.ResultArg; break;
        case D3DTSS_TEXCOORDINDEX:  *value = tss.TexCoordIndex; break;
        case D3DTSS_TEXTURETRANSFORMFLAGS: *value = tss.TextureTransformFlags; break;

        case D3DTSS_BUMPENVMAT00:   *value = *reinterpret_cast<const DWORD*>(&tss.BumpEnvMat00); break;
        case D3DTSS_BUMPENVMAT01:   *value = *reinterpret_cast<const DWORD*>(&tss.BumpEnvMat01); break;
        case D3DTSS_BUMPENVMAT10:   *value = *reinterpret_cast<const DWORD*>(&tss.BumpEnvMat10); break;
        case D3DTSS_BUMPENVMAT11:   *value = *reinterpret_cast<const DWORD*>(&tss.BumpEnvMat11); break;
        case D3DTSS_BUMPENVLSCALE:  *value = *reinterpret_cast<const DWORD*>(&tss.BumpEnvLScale); break;
        case D3DTSS_BUMPENVLOFFSET: *value = *reinterpret_cast<const DWORD*>(&tss.BumpEnvLOffset); break;

        // Sampler states
        case D3DTSS_ADDRESSU:
        case D3DTSS_ADDRESSV:
        case D3DTSS_ADDRESSW:
        case D3DTSS_MAGFILTER:
        case D3DTSS_MINFILTER:
        case D3DTSS_MIPFILTER:
        case D3DTSS_MIPMAPLODBIAS:
        case D3DTSS_MAXMIPLEVEL:
        case D3DTSS_MAXANISOTROPY:
        case D3DTSS_BORDERCOLOR:
            return GetSamplerState(stage, type, value);

        default:
            return D3DERR_INVALIDCALL;
    }

    return D3D_OK;
}

// =============================================================================
// Sampler States
// =============================================================================

HRESULT StateManager::SetSamplerState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value) {
    if (stage >= MaxTextureStages) return D3DERR_INVALIDCALL;

    SamplerState& ss = m_samplers[stage];

    switch (type) {
        case D3DTSS_ADDRESSU:      ss.AddressU = value; break;
        case D3DTSS_ADDRESSV:      ss.AddressV = value; break;
        case D3DTSS_ADDRESSW:      ss.AddressW = value; break;
        case D3DTSS_MAGFILTER:     ss.MagFilter = value; break;
        case D3DTSS_MINFILTER:     ss.MinFilter = value; break;
        case D3DTSS_MIPFILTER:     ss.MipFilter = value; break;
        case D3DTSS_MIPMAPLODBIAS: ss.MipMapLODBias = *reinterpret_cast<float*>(&value); break;
        case D3DTSS_MAXMIPLEVEL:   ss.MaxMipLevel = value; break;
        case D3DTSS_MAXANISOTROPY: ss.MaxAnisotropy = value; break;
        case D3DTSS_BORDERCOLOR:   ss.BorderColor = value; break;
        default: return D3DERR_INVALIDCALL;
    }

    m_dirty = true;
    return D3D_OK;
}

HRESULT StateManager::GetSamplerState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD* value) const {
    if (stage >= MaxTextureStages || !value) return D3DERR_INVALIDCALL;

    const SamplerState& ss = m_samplers[stage];

    switch (type) {
        case D3DTSS_ADDRESSU:      *value = ss.AddressU; break;
        case D3DTSS_ADDRESSV:      *value = ss.AddressV; break;
        case D3DTSS_ADDRESSW:      *value = ss.AddressW; break;
        case D3DTSS_MAGFILTER:     *value = ss.MagFilter; break;
        case D3DTSS_MINFILTER:     *value = ss.MinFilter; break;
        case D3DTSS_MIPFILTER:     *value = ss.MipFilter; break;
        case D3DTSS_MIPMAPLODBIAS: *value = *reinterpret_cast<const DWORD*>(&ss.MipMapLODBias); break;
        case D3DTSS_MAXMIPLEVEL:   *value = ss.MaxMipLevel; break;
        case D3DTSS_MAXANISOTROPY: *value = ss.MaxAnisotropy; break;
        case D3DTSS_BORDERCOLOR:   *value = ss.BorderColor; break;
        default: return D3DERR_INVALIDCALL;
    }

    return D3D_OK;
}

// =============================================================================
// Texture Handles
// =============================================================================

void StateManager::SetTextureHandle(DWORD stage, uint16_t handle) {
    if (stage < MaxTextureStages) {
        m_textureHandles[stage] = handle;
        m_dirty = true;
    }
}

uint16_t StateManager::GetTextureHandle(DWORD stage) const {
    return (stage < MaxTextureStages) ? m_textureHandles[stage] : UINT16_MAX;
}

bool StateManager::HasTexture(DWORD stage) const {
    return (stage < MaxTextureStages) && (m_textureHandles[stage] != UINT16_MAX);
}

// =============================================================================
// Viewport
// =============================================================================

void StateManager::SetViewport(const D3DVIEWPORT8& viewport) {
    m_viewport = viewport;
    m_dirty = true;
}

const D3DVIEWPORT8& StateManager::GetViewport() const {
    return m_viewport;
}

// =============================================================================
// FVF
// =============================================================================

void StateManager::SetFVF(DWORD fvf) {
    m_fvf = fvf;
    m_dirty = true;
}

DWORD StateManager::GetFVF() const {
    return m_fvf;
}

// =============================================================================
// Clip Planes
// =============================================================================

HRESULT StateManager::SetClipPlane(DWORD index, const float* plane) {
    if (index >= MaxClipPlanes || !plane) return D3DERR_INVALIDCALL;

    std::memcpy(m_clipPlanes[index], plane, sizeof(float) * 4);
    m_dirty = true;
    return D3D_OK;
}

HRESULT StateManager::GetClipPlane(DWORD index, float* plane) const {
    if (index >= MaxClipPlanes || !plane) return D3DERR_INVALIDCALL;

    std::memcpy(plane, m_clipPlanes[index], sizeof(float) * 4);
    return D3D_OK;
}

// =============================================================================
// Accessors
// =============================================================================

const TextureStageState& StateManager::GetTextureStage(DWORD stage) const {
    return m_textureStages[stage < MaxTextureStages ? stage : 0];
}

const SamplerState& StateManager::GetSampler(DWORD stage) const {
    return m_samplers[stage < MaxTextureStages ? stage : 0];
}

D3DCOLORVALUE StateManager::GetGlobalAmbient() const {
    return ColorFromD3DCOLOR(m_renderStates[D3DRS_AMBIENT]);
}

float StateManager::GetTweenFactor() const {
    return *reinterpret_cast<const float*>(&m_renderStates[D3DRS_TWEENFACTOR]);
}

// =============================================================================
// State Query Helpers
// =============================================================================

bool StateManager::IsLightingEnabled() const {
    return m_renderStates[D3DRS_LIGHTING] != FALSE;
}

bool StateManager::IsSpecularEnabled() const {
    return m_renderStates[D3DRS_SPECULARENABLE] != FALSE;
}

bool StateManager::IsFogEnabled() const {
    return m_renderStates[D3DRS_FOGENABLE] != FALSE;
}

bool StateManager::IsAlphaTestEnabled() const {
    return m_renderStates[D3DRS_ALPHATESTENABLE] != FALSE;
}

bool StateManager::IsAlphaBlendEnabled() const {
    return m_renderStates[D3DRS_ALPHABLENDENABLE] != FALSE;
}

D3DFOGMODE StateManager::GetFogMode() const {
    return static_cast<D3DFOGMODE>(m_renderStates[D3DRS_FOGTABLEMODE]);
}

D3DFOGMODE StateManager::GetVertexFogMode() const {
    return static_cast<D3DFOGMODE>(m_renderStates[D3DRS_FOGVERTEXMODE]);
}

bool StateManager::IsRangeFogEnabled() const {
    return m_renderStates[D3DRS_RANGEFOGENABLE] != FALSE;
}

bool StateManager::IsLocalViewerEnabled() const {
    return m_renderStates[D3DRS_LOCALVIEWER] != FALSE;
}

bool StateManager::ShouldNormalizeNormals() const {
    return m_renderStates[D3DRS_NORMALIZENORMALS] != FALSE;
}

D3DVERTEXBLENDFLAGS StateManager::GetVertexBlendMode() const {
    return static_cast<D3DVERTEXBLENDFLAGS>(m_renderStates[D3DRS_VERTEXBLEND]);
}

// =============================================================================
// Shader Key Building
// =============================================================================

VertexShaderKey StateManager::BuildVertexShaderKey() const {
    VertexShaderKey key;

    // FVF analysis
    DWORD fvf = m_fvf;
    key.Data.bits.HasPositionT = HasPositionT(fvf) ? 1 : 0;
    key.Data.bits.HasColor0 = (fvf & D3DFVF_DIFFUSE) ? 1 : 0;
    key.Data.bits.HasColor1 = (fvf & D3DFVF_SPECULAR) ? 1 : 0;
    key.Data.bits.HasPointSize = (fvf & D3DFVF_PSIZE) ? 1 : 0;
    key.Data.bits.HasNormal = (fvf & D3DFVF_NORMAL) ? 1 : 0;

    // Lighting
    bool useLighting = IsLightingEnabled() && !HasPositionT(fvf) && (fvf & D3DFVF_NORMAL);
    key.Data.bits.UseLighting = useLighting ? 1 : 0;

    if (useLighting) {
        key.Data.bits.LightCount = GetEnabledLightCount();
        key.Data.bits.LocalViewer = IsLocalViewerEnabled() ? 1 : 0;
        key.Data.bits.NormalizeNormals = ShouldNormalizeNormals() ? 1 : 0;
        key.Data.bits.SpecularEnabled = IsSpecularEnabled() ? 1 : 0;

        // Material color sources
        key.Data.bits.DiffuseSource = m_renderStates[D3DRS_DIFFUSEMATERIALSOURCE];
        key.Data.bits.AmbientSource = m_renderStates[D3DRS_AMBIENTMATERIALSOURCE];
        key.Data.bits.SpecularSource = m_renderStates[D3DRS_SPECULARMATERIALSOURCE];
        key.Data.bits.EmissiveSource = m_renderStates[D3DRS_EMISSIVEMATERIALSOURCE];
    }

    // Fog
    key.Data.bits.RangeFog = IsRangeFogEnabled() ? 1 : 0;
    key.Data.bits.FogMode = GetVertexFogMode();
    key.Data.bits.HasFog = (fvf & D3DFVF_SPECULAR) ? 1 : 0;  // Fog in specular.w

    // Vertex blending
    D3DVERTEXBLENDFLAGS blendFlags = GetVertexBlendMode();
    if (blendFlags == D3DVBF_TWEENING) {
        key.Data.bits.VertexBlendMode = 2;
    } else if (blendFlags != D3DVBF_DISABLE) {
        key.Data.bits.VertexBlendMode = 1;
        key.Data.bits.VertexBlendCount = blendFlags;
        key.Data.bits.VertexBlendIndexed = (m_renderStates[D3DRS_INDEXEDVERTEXBLENDENABLE] != FALSE) ? 1 : 0;
    }

    // Texture coordinate generation
    UINT texCount = GetTexCoordCount(fvf);
    for (DWORD i = 0; i < MaxTextureStages; i++) {
        DWORD tci = m_textureStages[i].TexCoordIndex;
        DWORD tciIndex = tci & 0xFFFF;
        DWORD tciGen = tci & 0xFFFF0000;

        // Store TCI index (3 bits per stage)
        key.Data.bits.TexcoordIndices |= (tciIndex & 0x7) << (i * 3);

        // Store TCI generation mode (3 bits per stage)
        DWORD genMode = (tciGen >> 16) & 0x7;
        key.Data.bits.TexcoordFlags |= genMode << (i * 3);

        // Store transform flags (3 bits per stage)
        DWORD transformFlags = m_textureStages[i].TextureTransformFlags & 0x7;
        key.Data.bits.TransformFlags |= transformFlags << (i * 3);

        // Mark which texcoords are declared
        if (tciIndex < texCount) {
            key.Data.bits.TexcoordDeclMask |= (1 << i);
        }
    }

    // Clipping
    key.Data.bits.Clipping = (m_renderStates[D3DRS_CLIPPLANEENABLE] != 0) ? 1 : 0;

    return key;
}

FragmentShaderKey StateManager::BuildFragmentShaderKey() const {
    FragmentShaderKey key;

    // Texture stages
    for (DWORD i = 0; i < MaxTextureStages; i++) {
        const TextureStageState& tss = m_textureStages[i];

        key.Data.Stages[i].bits.ColorOp = tss.ColorOp;
        key.Data.Stages[i].bits.ColorArg0 = tss.ColorArg0 & 0x3F;
        key.Data.Stages[i].bits.ColorArg1 = tss.ColorArg1 & 0x3F;
        key.Data.Stages[i].bits.ColorArg2 = tss.ColorArg2 & 0x3F;
        key.Data.Stages[i].bits.AlphaOp = tss.AlphaOp;
        key.Data.Stages[i].bits.AlphaArg0 = tss.AlphaArg0 & 0x3F;
        key.Data.Stages[i].bits.AlphaArg1 = tss.AlphaArg1 & 0x3F;
        key.Data.Stages[i].bits.AlphaArg2 = tss.AlphaArg2 & 0x3F;
        key.Data.Stages[i].bits.ResultIsTemp = (tss.ResultArg == D3DTA_TEMP) ? 1 : 0;
        key.Data.Stages[i].bits.HasTexture = HasTexture(i) ? 1 : 0;
    }

    // Global flags
    key.Data.bits.AlphaTestEnabled = IsAlphaTestEnabled() ? 1 : 0;
    key.Data.bits.AlphaTestFunc = m_renderStates[D3DRS_ALPHAFUNC] & 0x7;
    key.Data.bits.FogEnabled = IsFogEnabled() ? 1 : 0;
    key.Data.bits.FogMode = GetFogMode() & 0x3;
    key.Data.bits.SpecularEnabled = IsSpecularEnabled() ? 1 : 0;

    return key;
}

ShaderKey StateManager::BuildShaderKey() const {
    return {BuildVertexShaderKey(), BuildFragmentShaderKey()};
}

} // namespace dx8bgfx
