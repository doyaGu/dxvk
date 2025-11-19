#pragma once

#include "dx8_types.h"
#include "dx8_state_manager.h"

#include <bgfx/bgfx.h>
#include <array>

namespace dx8bgfx {

// =============================================================================
// Uniform Manager
// =============================================================================

class UniformManager {
public:
    UniformManager();
    ~UniformManager();

    // Initialize uniforms
    void Init();
    void Shutdown();

    // Update uniforms from state manager
    void UpdateUniforms(const StateManager& state);

    // Set texture for stage
    void SetTexture(DWORD stage, bgfx::TextureHandle texture);

private:
    // Create uniform handles
    void CreateUniforms();
    void DestroyUniforms();

    // Update specific uniform groups
    void UpdateTransforms(const StateManager& state);
    void UpdateMaterial(const StateManager& state);
    void UpdateLights(const StateManager& state);
    void UpdateFog(const StateManager& state);
    void UpdateTextureStages(const StateManager& state);

    // Helper to convert D3DMATRIX to float[16]
    static void MatrixToFloatArray(const D3DMATRIX& m, float* out);

    // Matrix multiplication helper
    static D3DMATRIX MultiplyMatrix(const D3DMATRIX& a, const D3DMATRIX& b);

private:
    // Transform uniforms
    bgfx::UniformHandle u_worldView;
    bgfx::UniformHandle u_worldViewProj;
    bgfx::UniformHandle u_normalMatrix;
    bgfx::UniformHandle u_invView;

    // Texture matrix uniforms
    std::array<bgfx::UniformHandle, 8> u_texMatrix;

    // Material uniforms
    bgfx::UniformHandle u_materialDiffuse;
    bgfx::UniformHandle u_materialAmbient;
    bgfx::UniformHandle u_materialSpecular;
    bgfx::UniformHandle u_materialEmissive;
    bgfx::UniformHandle u_materialPower;

    // Global ambient
    bgfx::UniformHandle u_globalAmbient;

    // Light uniforms (for each light)
    struct LightUniforms {
        bgfx::UniformHandle diffuse;
        bgfx::UniformHandle specular;
        bgfx::UniformHandle ambient;
        bgfx::UniformHandle position;
        bgfx::UniformHandle direction;
        bgfx::UniformHandle attenuation;
        bgfx::UniformHandle spotParams;
    };
    std::array<LightUniforms, MaxLights> u_lights;

    // Fog uniforms
    bgfx::UniformHandle u_fogParams;
    bgfx::UniformHandle u_fogColor;

    // Alpha test
    bgfx::UniformHandle u_alphaTest;

    // Texture factor
    bgfx::UniformHandle u_textureFactor;

    // Control flags
    bgfx::UniformHandle u_flags;
    bgfx::UniformHandle u_psFlags;

    // Viewport info
    bgfx::UniformHandle u_viewportInvOffset;
    bgfx::UniformHandle u_viewportInvExtent;

    // Tween factor
    bgfx::UniformHandle u_tweenFactor;

    // Texture samplers
    std::array<bgfx::UniformHandle, 8> s_texture;

    // Initialization flag
    bool m_initialized = false;
};

} // namespace dx8bgfx
