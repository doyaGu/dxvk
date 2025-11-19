#include "dx8bgfx/dx8_fog_utils.h"
#include <cmath>
#include <sstream>

namespace dx8bgfx {

// =============================================================================
// Fog Utils Implementation
// =============================================================================

FogParams FogUtils::BuildFogParams(const StateManager& state) {
    FogParams params = {};

    // Get fog color
    DWORD fogColor = 0;
    state.GetRenderState(D3DRS_FOGCOLOR, &fogColor);
    D3DColorToFloat4(fogColor, params.fogColor);

    // Get fog mode and parameters
    DWORD fogTableMode = D3DFOG_NONE;
    DWORD fogVertexMode = D3DFOG_NONE;
    state.GetRenderState(D3DRS_FOGTABLEMODE, &fogTableMode);
    state.GetRenderState(D3DRS_FOGVERTEXMODE, &fogVertexMode);

    // Determine active fog mode
    D3DFOGMODE activeMode = D3DFOG_NONE;
    bool isPixelFog = false;

    if (fogTableMode != D3DFOG_NONE) {
        activeMode = static_cast<D3DFOGMODE>(fogTableMode);
        isPixelFog = true;
    } else if (fogVertexMode != D3DFOG_NONE) {
        activeMode = static_cast<D3DFOGMODE>(fogVertexMode);
        isPixelFog = false;
    }

    // Get fog parameters based on mode
    DWORD fogStartDW = 0, fogEndDW = 0, fogDensityDW = 0;
    state.GetRenderState(D3DRS_FOGSTART, &fogStartDW);
    state.GetRenderState(D3DRS_FOGEND, &fogEndDW);
    state.GetRenderState(D3DRS_FOGDENSITY, &fogDensityDW);

    float fogStart = *reinterpret_cast<float*>(&fogStartDW);
    float fogEnd = *reinterpret_cast<float*>(&fogEndDW);
    float fogDensity = *reinterpret_cast<float*>(&fogDensityDW);

    // Pack fog parameters
    switch (activeMode) {
        case D3DFOG_LINEAR:
            params.fogParams[0] = fogStart;
            params.fogParams[1] = fogEnd;
            params.fogParams[2] = (fogEnd != fogStart) ? 1.0f / (fogEnd - fogStart) : 0.0f;
            params.fogParams[3] = 1.0f; // Linear mode
            break;
        case D3DFOG_EXP:
            params.fogParams[0] = fogDensity;
            params.fogParams[1] = 0.0f;
            params.fogParams[2] = 0.0f;
            params.fogParams[3] = 2.0f; // Exp mode
            break;
        case D3DFOG_EXP2:
            params.fogParams[0] = fogDensity;
            params.fogParams[1] = 0.0f;
            params.fogParams[2] = 0.0f;
            params.fogParams[3] = 3.0f; // Exp2 mode
            break;
        default:
            params.fogParams[3] = 0.0f; // No fog
            break;
    }

    // Range fog
    DWORD rangeFogEnable = FALSE;
    state.GetRenderState(D3DRS_RANGEFOGENABLE, &rangeFogEnable);
    params.fogParams2[0] = rangeFogEnable ? 1.0f : 0.0f;

    // Store modes
    params.fogParams2[1] = static_cast<float>(fogTableMode);
    params.fogParams2[2] = static_cast<float>(fogVertexMode);

    return params;
}

float FogUtils::CalculateLinearFog(float dist, float start, float end) {
    if (end == start) return 1.0f;
    return std::max(0.0f, std::min(1.0f, (end - dist) / (end - start)));
}

float FogUtils::CalculateExpFog(float dist, float density) {
    return std::exp(-density * dist);
}

float FogUtils::CalculateExp2Fog(float dist, float density) {
    float d = density * dist;
    return std::exp(-d * d);
}

void FogUtils::D3DColorToFloat4(D3DCOLOR color, float* out) {
    out[0] = float((color >> 16) & 0xFF) / 255.0f; // R
    out[1] = float((color >> 8) & 0xFF) / 255.0f;  // G
    out[2] = float(color & 0xFF) / 255.0f;         // B
    out[3] = float((color >> 24) & 0xFF) / 255.0f; // A
}

const char* FogUtils::GetFogModeName(D3DFOGMODE mode) {
    switch (mode) {
        case D3DFOG_NONE:   return "NONE";
        case D3DFOG_EXP:    return "EXP";
        case D3DFOG_EXP2:   return "EXP2";
        case D3DFOG_LINEAR: return "LINEAR";
        default:            return "UNKNOWN";
    }
}

bool FogUtils::ShouldUsePixelFog(const StateManager& state) {
    DWORD fogTableMode = D3DFOG_NONE;
    state.GetRenderState(D3DRS_FOGTABLEMODE, &fogTableMode);
    return fogTableMode != D3DFOG_NONE;
}

std::string FogUtils::GenerateFogVertexCode(D3DFOGMODE mode, bool rangeFog) {
    std::ostringstream code;

    if (mode == D3DFOG_NONE) {
        code << "    v_fogFactor = 1.0;\n";
        return code.str();
    }

    // Calculate distance
    if (rangeFog) {
        code << "    float fogDist = length(v_viewPos.xyz);\n";
    } else {
        code << "    float fogDist = abs(v_viewPos.z);\n";
    }

    // Calculate fog factor based on mode
    switch (mode) {
        case D3DFOG_LINEAR:
            code << "    v_fogFactor = clamp((u_fogParams.y - fogDist) * u_fogParams.z, 0.0, 1.0);\n";
            break;
        case D3DFOG_EXP:
            code << "    v_fogFactor = exp(-u_fogParams.x * fogDist);\n";
            break;
        case D3DFOG_EXP2:
            code << "    float fogD = u_fogParams.x * fogDist;\n";
            code << "    v_fogFactor = exp(-fogD * fogD);\n";
            break;
        default:
            code << "    v_fogFactor = 1.0;\n";
            break;
    }

    return code.str();
}

std::string FogUtils::GenerateFogFragmentCode() {
    return "    gl_FragColor.rgb = mix(u_fogColor.rgb, gl_FragColor.rgb, v_fogFactor);\n";
}

// =============================================================================
// Fog Uniform Manager Implementation
// =============================================================================

FogUniformManager::FogUniformManager()
    : m_fogColor(BGFX_INVALID_HANDLE)
    , m_fogParams(BGFX_INVALID_HANDLE)
    , m_fogParams2(BGFX_INVALID_HANDLE)
    , m_params{}
    , m_initialized(false)
{
}

FogUniformManager::~FogUniformManager() {
    Shutdown();
}

void FogUniformManager::Initialize() {
    if (m_initialized) return;

    m_fogColor = bgfx::createUniform("u_fogColor", bgfx::UniformType::Vec4);
    m_fogParams = bgfx::createUniform("u_fogParams", bgfx::UniformType::Vec4);
    m_fogParams2 = bgfx::createUniform("u_fogParams2", bgfx::UniformType::Vec4);

    m_initialized = true;
}

void FogUniformManager::Shutdown() {
    if (!m_initialized) return;

    if (bgfx::isValid(m_fogColor)) bgfx::destroy(m_fogColor);
    if (bgfx::isValid(m_fogParams)) bgfx::destroy(m_fogParams);
    if (bgfx::isValid(m_fogParams2)) bgfx::destroy(m_fogParams2);

    m_fogColor = BGFX_INVALID_HANDLE;
    m_fogParams = BGFX_INVALID_HANDLE;
    m_fogParams2 = BGFX_INVALID_HANDLE;

    m_initialized = false;
}

void FogUniformManager::Update(const StateManager& state) {
    m_params = FogUtils::BuildFogParams(state);
}

void FogUniformManager::Bind() {
    if (!m_initialized) return;

    bgfx::setUniform(m_fogColor, m_params.fogColor);
    bgfx::setUniform(m_fogParams, m_params.fogParams);
    bgfx::setUniform(m_fogParams2, m_params.fogParams2);
}

} // namespace dx8bgfx
