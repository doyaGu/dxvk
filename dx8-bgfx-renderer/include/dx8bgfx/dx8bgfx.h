#pragma once

// =============================================================================
// DX8-BGFX Renderer - Main Include Header
// =============================================================================
//
// A bgfx-based renderer that emulates DirectX 8 fixed-function pipeline
//
// Basic usage:
//   #include <dx8bgfx/dx8bgfx.h>
//
//   dx8bgfx::Renderer renderer;
//   renderer.Initialize(width, height);
//
//   // Set transforms
//   renderer.SetTransform(D3DTS_WORLD, &worldMatrix);
//   renderer.SetTransform(D3DTS_VIEW, &viewMatrix);
//   renderer.SetTransform(D3DTS_PROJECTION, &projMatrix);
//
//   // Set material and lights
//   renderer.SetMaterial(&material);
//   renderer.SetLight(0, &light);
//   renderer.LightEnable(0, TRUE);
//   renderer.SetRenderState(D3DRS_LIGHTING, TRUE);
//
//   // Draw
//   renderer.DrawPrimitive(D3DPT_TRIANGLELIST, 0, numTriangles);
//
//   renderer.EndFrame();
//

// Core types and constants
#include "dx8_types.h"
#include "dx8_constants.h"
#include "dx8_math.h"

// State management
#include "dx8_state_manager.h"
#include "dx8_stencil_utils.h"
#include "dx8_fog_utils.h"

// Shader system
#include "dx8_shader_key.h"
#include "dx8_shader_generator.h"
#include "dx8_shader_cache.h"
#include "dx8_shader_compiler.h"
#include "dx8_shader_binary.h"

// Core renderer
#include "dx8_renderer.h"
#include "dx8_uniform_manager.h"

// Texture utilities
#include "dx8_texture_utils.h"
#include "dx8_sampler_utils.h"

// Buffer utilities
#include "dx8_buffer_utils.h"

// Sprite rendering
#include "dx8_sprite_batch.h"

// Vertex processing
#include "dx8_vertex_processing.h"

// Device capabilities
#include "dx8_caps.h"

// Debug utilities
#include "dx8_debug.h"

namespace dx8bgfx {

// =============================================================================
// Version Information
// =============================================================================

constexpr int VERSION_MAJOR = 0;
constexpr int VERSION_MINOR = 1;
constexpr int VERSION_PATCH = 0;

inline const char* GetVersionString() {
    return "0.1.0";
}

// =============================================================================
// Quick Setup Helpers
// =============================================================================

// Initialize bgfx with common settings for DX8 emulation
inline bool InitializeBgfx(
    void* nativeWindowHandle,
    uint32_t width,
    uint32_t height,
    bgfx::RendererType::Enum preferredRenderer = bgfx::RendererType::Count
) {
    bgfx::Init init;
    init.type = preferredRenderer;
    init.resolution.width = width;
    init.resolution.height = height;
    init.resolution.reset = BGFX_RESET_VSYNC;
    init.platformData.nwh = nativeWindowHandle;

    return bgfx::init(init);
}

// Create a default material (white, no specular)
inline D3DMATERIAL8 CreateDefaultMaterial() {
    D3DMATERIAL8 mat = {};
    mat.Diffuse = {1.0f, 1.0f, 1.0f, 1.0f};
    mat.Ambient = {1.0f, 1.0f, 1.0f, 1.0f};
    mat.Specular = {0.0f, 0.0f, 0.0f, 0.0f};
    mat.Emissive = {0.0f, 0.0f, 0.0f, 0.0f};
    mat.Power = 0.0f;
    return mat;
}

// Create a directional light
inline D3DLIGHT8 CreateDirectionalLight(
    float dirX, float dirY, float dirZ,
    float r = 1.0f, float g = 1.0f, float b = 1.0f
) {
    D3DLIGHT8 light = {};
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Diffuse = {r, g, b, 1.0f};
    light.Specular = {r, g, b, 1.0f};
    light.Ambient = {0.0f, 0.0f, 0.0f, 1.0f};
    light.Direction = {dirX, dirY, dirZ};
    return light;
}

// Create a point light
inline D3DLIGHT8 CreatePointLight(
    float posX, float posY, float posZ,
    float r = 1.0f, float g = 1.0f, float b = 1.0f,
    float range = 1000.0f,
    float attenuation0 = 1.0f,
    float attenuation1 = 0.0f,
    float attenuation2 = 0.0f
) {
    D3DLIGHT8 light = {};
    light.Type = D3DLIGHT_POINT;
    light.Diffuse = {r, g, b, 1.0f};
    light.Specular = {r, g, b, 1.0f};
    light.Ambient = {0.0f, 0.0f, 0.0f, 1.0f};
    light.Position = {posX, posY, posZ};
    light.Range = range;
    light.Attenuation0 = attenuation0;
    light.Attenuation1 = attenuation1;
    light.Attenuation2 = attenuation2;
    return light;
}

// Create a spot light
inline D3DLIGHT8 CreateSpotLight(
    float posX, float posY, float posZ,
    float dirX, float dirY, float dirZ,
    float r = 1.0f, float g = 1.0f, float b = 1.0f,
    float range = 1000.0f,
    float innerCone = 0.5f,  // radians
    float outerCone = 0.7f,  // radians
    float falloff = 1.0f
) {
    D3DLIGHT8 light = {};
    light.Type = D3DLIGHT_SPOT;
    light.Diffuse = {r, g, b, 1.0f};
    light.Specular = {r, g, b, 1.0f};
    light.Ambient = {0.0f, 0.0f, 0.0f, 1.0f};
    light.Position = {posX, posY, posZ};
    light.Direction = {dirX, dirY, dirZ};
    light.Range = range;
    light.Theta = innerCone;
    light.Phi = outerCone;
    light.Falloff = falloff;
    light.Attenuation0 = 1.0f;
    light.Attenuation1 = 0.0f;
    light.Attenuation2 = 0.0f;
    return light;
}

// =============================================================================
// Common Render State Presets
// =============================================================================

// Apply common states for solid geometry rendering
inline void ApplySolidRenderStates(Renderer& renderer) {
    renderer.SetRenderState(D3DRS_ZENABLE, TRUE);
    renderer.SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    renderer.SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    renderer.SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    renderer.SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
}

// Apply common states for transparent geometry
inline void ApplyTransparentRenderStates(Renderer& renderer) {
    renderer.SetRenderState(D3DRS_ZENABLE, TRUE);
    renderer.SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    renderer.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    renderer.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    renderer.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    renderer.SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
}

// Apply additive blending (for particles, glows)
inline void ApplyAdditiveBlending(Renderer& renderer) {
    renderer.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    renderer.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    renderer.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    renderer.SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
}

// Apply alpha testing (for masked textures like foliage)
inline void ApplyAlphaTestStates(Renderer& renderer, DWORD alphaRef = 128) {
    renderer.SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    renderer.SetRenderState(D3DRS_ALPHAREF, alphaRef);
    renderer.SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
}

} // namespace dx8bgfx
