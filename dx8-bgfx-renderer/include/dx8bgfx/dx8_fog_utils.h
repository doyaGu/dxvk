#pragma once

#include "dx8_types.h"
#include "dx8_constants.h"
#include "dx8_state_manager.h"

#include <bgfx/bgfx.h>

namespace dx8bgfx {

// =============================================================================
// Fog Utilities
// =============================================================================

// GPU-ready fog parameters
struct FogParams {
    float fogColor[4];      // Fog color (RGBA)
    float fogParams[4];     // x=start/density, y=end, z=1/(end-start), w=mode
    float fogParams2[4];    // x=range fog enabled, y=table mode, z=vertex mode, w=unused
};

class FogUtils {
public:
    // Build fog parameters from state manager
    static FogParams BuildFogParams(const StateManager& state);

    // Calculate linear fog factor: (end - dist) / (end - start)
    static float CalculateLinearFog(float dist, float start, float end);

    // Calculate exponential fog factor: exp(-density * dist)
    static float CalculateExpFog(float dist, float density);

    // Calculate exponential squared fog factor: exp(-(density * dist)^2)
    static float CalculateExp2Fog(float dist, float density);

    // Convert D3D fog color to float array
    static void D3DColorToFloat4(D3DCOLOR color, float* out);

    // Get fog mode name for debugging
    static const char* GetFogModeName(D3DFOGMODE mode);

    // Check if pixel fog should be used (vs vertex fog)
    static bool ShouldUsePixelFog(const StateManager& state);

    // Generate fog shader code snippet
    static std::string GenerateFogVertexCode(D3DFOGMODE mode, bool rangeFog);
    static std::string GenerateFogFragmentCode();
};

// =============================================================================
// Fog Uniform Manager
// =============================================================================

class FogUniformManager {
public:
    FogUniformManager();
    ~FogUniformManager();

    // Initialize uniforms
    void Initialize();

    // Destroy uniforms
    void Shutdown();

    // Update fog uniforms from state
    void Update(const StateManager& state);

    // Bind fog uniforms for rendering
    void Bind();

private:
    bgfx::UniformHandle m_fogColor;
    bgfx::UniformHandle m_fogParams;
    bgfx::UniformHandle m_fogParams2;

    FogParams m_params;
    bool m_initialized;
};

} // namespace dx8bgfx
