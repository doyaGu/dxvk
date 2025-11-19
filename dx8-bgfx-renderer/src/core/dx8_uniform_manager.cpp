#include "dx8bgfx/dx8_uniform_manager.h"
#include <cstring>
#include <cmath>

namespace dx8bgfx {

UniformManager::UniformManager() = default;

UniformManager::~UniformManager() {
    Shutdown();
}

void UniformManager::Init() {
    if (m_initialized) return;

    CreateUniforms();
    m_initialized = true;
}

void UniformManager::Shutdown() {
    if (!m_initialized) return;

    DestroyUniforms();
    m_initialized = false;
}

void UniformManager::CreateUniforms() {
    // Transform uniforms
    u_worldView = bgfx::createUniform("u_worldView", bgfx::UniformType::Mat4);
    u_worldViewProj = bgfx::createUniform("u_worldViewProj", bgfx::UniformType::Mat4);
    u_normalMatrix = bgfx::createUniform("u_normalMatrix", bgfx::UniformType::Mat4);
    u_invView = bgfx::createUniform("u_invView", bgfx::UniformType::Mat4);

    // Texture matrices
    for (int i = 0; i < 8; i++) {
        char name[32];
        snprintf(name, sizeof(name), "u_texMatrix%d", i);
        u_texMatrix[i] = bgfx::createUniform(name, bgfx::UniformType::Mat4);
    }

    // Material uniforms
    u_materialDiffuse = bgfx::createUniform("u_materialDiffuse", bgfx::UniformType::Vec4);
    u_materialAmbient = bgfx::createUniform("u_materialAmbient", bgfx::UniformType::Vec4);
    u_materialSpecular = bgfx::createUniform("u_materialSpecular", bgfx::UniformType::Vec4);
    u_materialEmissive = bgfx::createUniform("u_materialEmissive", bgfx::UniformType::Vec4);
    u_materialPower = bgfx::createUniform("u_materialPower", bgfx::UniformType::Vec4);

    // Global ambient
    u_globalAmbient = bgfx::createUniform("u_globalAmbient", bgfx::UniformType::Vec4);

    // Light uniforms
    for (int i = 0; i < MaxLights; i++) {
        char name[64];

        snprintf(name, sizeof(name), "u_light%dDiffuse", i);
        u_lights[i].diffuse = bgfx::createUniform(name, bgfx::UniformType::Vec4);

        snprintf(name, sizeof(name), "u_light%dSpecular", i);
        u_lights[i].specular = bgfx::createUniform(name, bgfx::UniformType::Vec4);

        snprintf(name, sizeof(name), "u_light%dAmbient", i);
        u_lights[i].ambient = bgfx::createUniform(name, bgfx::UniformType::Vec4);

        snprintf(name, sizeof(name), "u_light%dPosition", i);
        u_lights[i].position = bgfx::createUniform(name, bgfx::UniformType::Vec4);

        snprintf(name, sizeof(name), "u_light%dDirection", i);
        u_lights[i].direction = bgfx::createUniform(name, bgfx::UniformType::Vec4);

        snprintf(name, sizeof(name), "u_light%dAttenuation", i);
        u_lights[i].attenuation = bgfx::createUniform(name, bgfx::UniformType::Vec4);

        snprintf(name, sizeof(name), "u_light%dSpotParams", i);
        u_lights[i].spotParams = bgfx::createUniform(name, bgfx::UniformType::Vec4);
    }

    // Fog uniforms
    u_fogParams = bgfx::createUniform("u_fogParams", bgfx::UniformType::Vec4);
    u_fogColor = bgfx::createUniform("u_fogColor", bgfx::UniformType::Vec4);

    // Alpha test
    u_alphaTest = bgfx::createUniform("u_alphaTest", bgfx::UniformType::Vec4);

    // Texture factor
    u_textureFactor = bgfx::createUniform("u_textureFactor", bgfx::UniformType::Vec4);

    // Control flags
    u_flags = bgfx::createUniform("u_flags", bgfx::UniformType::Vec4);
    u_psFlags = bgfx::createUniform("u_psFlags", bgfx::UniformType::Vec4);

    // Viewport
    u_viewportInvOffset = bgfx::createUniform("u_viewportInvOffset", bgfx::UniformType::Vec4);
    u_viewportInvExtent = bgfx::createUniform("u_viewportInvExtent", bgfx::UniformType::Vec4);

    // Tween
    u_tweenFactor = bgfx::createUniform("u_tweenFactor", bgfx::UniformType::Vec4);

    // Texture samplers
    for (int i = 0; i < 8; i++) {
        char name[32];
        snprintf(name, sizeof(name), "s_texture%d", i);
        s_texture[i] = bgfx::createUniform(name, bgfx::UniformType::Sampler);
    }
}

void UniformManager::DestroyUniforms() {
    bgfx::destroy(u_worldView);
    bgfx::destroy(u_worldViewProj);
    bgfx::destroy(u_normalMatrix);
    bgfx::destroy(u_invView);

    for (int i = 0; i < 8; i++) {
        bgfx::destroy(u_texMatrix[i]);
    }

    bgfx::destroy(u_materialDiffuse);
    bgfx::destroy(u_materialAmbient);
    bgfx::destroy(u_materialSpecular);
    bgfx::destroy(u_materialEmissive);
    bgfx::destroy(u_materialPower);
    bgfx::destroy(u_globalAmbient);

    for (int i = 0; i < MaxLights; i++) {
        bgfx::destroy(u_lights[i].diffuse);
        bgfx::destroy(u_lights[i].specular);
        bgfx::destroy(u_lights[i].ambient);
        bgfx::destroy(u_lights[i].position);
        bgfx::destroy(u_lights[i].direction);
        bgfx::destroy(u_lights[i].attenuation);
        bgfx::destroy(u_lights[i].spotParams);
    }

    bgfx::destroy(u_fogParams);
    bgfx::destroy(u_fogColor);
    bgfx::destroy(u_alphaTest);
    bgfx::destroy(u_textureFactor);
    bgfx::destroy(u_flags);
    bgfx::destroy(u_psFlags);
    bgfx::destroy(u_viewportInvOffset);
    bgfx::destroy(u_viewportInvExtent);
    bgfx::destroy(u_tweenFactor);

    for (int i = 0; i < 8; i++) {
        bgfx::destroy(s_texture[i]);
    }
}

void UniformManager::UpdateUniforms(const StateManager& state) {
    UpdateTransforms(state);
    UpdateMaterial(state);
    UpdateLights(state);
    UpdateFog(state);
    UpdateTextureStages(state);

    // Control flags
    float flags[4] = {
        state.IsLightingEnabled() ? 1.0f : 0.0f,
        state.IsSpecularEnabled() ? 1.0f : 0.0f,
        state.ShouldNormalizeNormals() ? 1.0f : 0.0f,
        state.IsLocalViewerEnabled() ? 1.0f : 0.0f
    };
    bgfx::setUniform(u_flags, flags);

    float psFlags[4] = {
        state.IsFogEnabled() ? 1.0f : 0.0f,
        state.IsSpecularEnabled() ? 1.0f : 0.0f,
        0.0f,
        0.0f
    };
    bgfx::setUniform(u_psFlags, psFlags);

    // Alpha test
    DWORD alphaRef = 0;
    state.GetRenderState(D3DRS_ALPHAREF, &alphaRef);
    float alphaTest[4] = {
        state.IsAlphaTestEnabled() ? 1.0f : 0.0f,
        static_cast<float>(alphaRef) / 255.0f,
        0.0f,
        0.0f
    };
    bgfx::setUniform(u_alphaTest, alphaTest);

    // Texture factor
    DWORD texFactor = 0;
    state.GetRenderState(D3DRS_TEXTUREFACTOR, &texFactor);
    D3DCOLORVALUE tfColor = ColorFromD3DCOLOR(texFactor);
    float textureFactor[4] = {tfColor.r, tfColor.g, tfColor.b, tfColor.a};
    bgfx::setUniform(u_textureFactor, textureFactor);

    // Tween factor
    float tweenFactor[4] = {state.GetTweenFactor(), 0.0f, 0.0f, 0.0f};
    bgfx::setUniform(u_tweenFactor, tweenFactor);
}

void UniformManager::UpdateTransforms(const StateManager& state) {
    const D3DMATRIX& world = state.GetWorldMatrix();
    const D3DMATRIX& view = state.GetViewMatrix();
    const D3DMATRIX& proj = state.GetProjectionMatrix();

    // WorldView = World * View
    D3DMATRIX worldView = MultiplyMatrix(world, view);
    float wvData[16];
    MatrixToFloatArray(worldView, wvData);
    bgfx::setUniform(u_worldView, wvData);

    // WorldViewProj = WorldView * Proj
    D3DMATRIX worldViewProj = MultiplyMatrix(worldView, proj);
    float wvpData[16];
    MatrixToFloatArray(worldViewProj, wvpData);
    bgfx::setUniform(u_worldViewProj, wvpData);

    // Normal matrix (upper-left 3x3 of inverse transpose of WorldView)
    // For simplicity, we use WorldView directly (works for uniform scale)
    float nmData[16];
    MatrixToFloatArray(worldView, nmData);
    bgfx::setUniform(u_normalMatrix, nmData);

    // Inverse view (for texture coordinate generation)
    // For simplicity, just pass view matrix (should be inverse)
    float ivData[16];
    MatrixToFloatArray(view, ivData);
    bgfx::setUniform(u_invView, ivData);

    // Texture matrices
    for (int i = 0; i < 8; i++) {
        const D3DMATRIX& texMat = state.GetTextureMatrix(i);
        float tmData[16];
        MatrixToFloatArray(texMat, tmData);
        bgfx::setUniform(u_texMatrix[i], tmData);
    }

    // Viewport info for pre-transformed vertices
    const D3DVIEWPORT8& vp = state.GetViewport();
    float vpInvOffset[4] = {
        -static_cast<float>(vp.X) * 2.0f / vp.Width - 1.0f,
        static_cast<float>(vp.Y) * 2.0f / vp.Height + 1.0f,
        -vp.MinZ,
        0.0f
    };
    float vpInvExtent[4] = {
        2.0f / vp.Width,
        -2.0f / vp.Height,
        1.0f / (vp.MaxZ - vp.MinZ),
        0.0f
    };
    bgfx::setUniform(u_viewportInvOffset, vpInvOffset);
    bgfx::setUniform(u_viewportInvExtent, vpInvExtent);
}

void UniformManager::UpdateMaterial(const StateManager& state) {
    const D3DMATERIAL8& mat = state.GetMaterial();

    float diffuse[4] = {mat.Diffuse.r, mat.Diffuse.g, mat.Diffuse.b, mat.Diffuse.a};
    float ambient[4] = {mat.Ambient.r, mat.Ambient.g, mat.Ambient.b, mat.Ambient.a};
    float specular[4] = {mat.Specular.r, mat.Specular.g, mat.Specular.b, mat.Specular.a};
    float emissive[4] = {mat.Emissive.r, mat.Emissive.g, mat.Emissive.b, mat.Emissive.a};
    float power[4] = {mat.Power, 0.0f, 0.0f, 0.0f};

    bgfx::setUniform(u_materialDiffuse, diffuse);
    bgfx::setUniform(u_materialAmbient, ambient);
    bgfx::setUniform(u_materialSpecular, specular);
    bgfx::setUniform(u_materialEmissive, emissive);
    bgfx::setUniform(u_materialPower, power);

    // Global ambient
    D3DCOLORVALUE globalAmbient = state.GetGlobalAmbient();
    float ga[4] = {globalAmbient.r, globalAmbient.g, globalAmbient.b, globalAmbient.a};
    bgfx::setUniform(u_globalAmbient, ga);
}

void UniformManager::UpdateLights(const StateManager& state) {
    const D3DMATRIX& view = state.GetViewMatrix();

    for (DWORD i = 0; i < MaxLights; i++) {
        if (!state.IsLightEnabled(i)) {
            // Set to zero
            float zero[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            bgfx::setUniform(u_lights[i].diffuse, zero);
            bgfx::setUniform(u_lights[i].specular, zero);
            bgfx::setUniform(u_lights[i].ambient, zero);
            bgfx::setUniform(u_lights[i].position, zero);
            bgfx::setUniform(u_lights[i].direction, zero);
            bgfx::setUniform(u_lights[i].attenuation, zero);
            bgfx::setUniform(u_lights[i].spotParams, zero);
            continue;
        }

        const D3DLIGHT8& light = state.GetLight(i);

        float diffuse[4] = {light.Diffuse.r, light.Diffuse.g, light.Diffuse.b, light.Diffuse.a};
        float specular[4] = {light.Specular.r, light.Specular.g, light.Specular.b, light.Specular.a};
        float ambient[4] = {light.Ambient.r, light.Ambient.g, light.Ambient.b, light.Ambient.a};

        // Transform position and direction to view space
        float pos[4] = {light.Position.x, light.Position.y, light.Position.z, 1.0f};
        float dir[4] = {light.Direction.x, light.Direction.y, light.Direction.z, 0.0f};

        // Transform to view space
        float viewPos[4], viewDir[4];
        // Simple matrix-vector multiply
        for (int j = 0; j < 4; j++) {
            viewPos[j] = view.m[0][j] * pos[0] + view.m[1][j] * pos[1] +
                         view.m[2][j] * pos[2] + view.m[3][j] * pos[3];
            viewDir[j] = view.m[0][j] * dir[0] + view.m[1][j] * dir[1] +
                         view.m[2][j] * dir[2];
        }

        // Normalize direction
        float len = std::sqrt(viewDir[0]*viewDir[0] + viewDir[1]*viewDir[1] + viewDir[2]*viewDir[2]);
        if (len > 0.0001f) {
            viewDir[0] /= len;
            viewDir[1] /= len;
            viewDir[2] /= len;
        }

        // Position.w = light type
        viewPos[3] = static_cast<float>(light.Type);

        // Direction.w = range
        viewDir[3] = light.Range;

        float attenuation[4] = {
            light.Attenuation0,
            light.Attenuation1,
            light.Attenuation2,
            light.Falloff
        };

        float spotParams[4] = {
            std::cos(light.Theta / 2.0f),
            std::cos(light.Phi / 2.0f),
            0.0f,
            0.0f
        };

        bgfx::setUniform(u_lights[i].diffuse, diffuse);
        bgfx::setUniform(u_lights[i].specular, specular);
        bgfx::setUniform(u_lights[i].ambient, ambient);
        bgfx::setUniform(u_lights[i].position, viewPos);
        bgfx::setUniform(u_lights[i].direction, viewDir);
        bgfx::setUniform(u_lights[i].attenuation, attenuation);
        bgfx::setUniform(u_lights[i].spotParams, spotParams);
    }
}

void UniformManager::UpdateFog(const StateManager& state) {
    DWORD fogStart = 0, fogEnd = 0, fogDensity = 0, fogColor = 0;
    state.GetRenderState(D3DRS_FOGSTART, &fogStart);
    state.GetRenderState(D3DRS_FOGEND, &fogEnd);
    state.GetRenderState(D3DRS_FOGDENSITY, &fogDensity);
    state.GetRenderState(D3DRS_FOGCOLOR, &fogColor);

    float fogParams[4] = {
        *reinterpret_cast<float*>(&fogStart),
        *reinterpret_cast<float*>(&fogEnd),
        *reinterpret_cast<float*>(&fogDensity),
        static_cast<float>(state.GetVertexFogMode())
    };
    bgfx::setUniform(u_fogParams, fogParams);

    D3DCOLORVALUE fc = ColorFromD3DCOLOR(fogColor);
    float fogColorData[4] = {fc.r, fc.g, fc.b, fc.a};
    bgfx::setUniform(u_fogColor, fogColorData);
}

void UniformManager::UpdateTextureStages(const StateManager& state) {
    // Texture stage states are passed through shader key and generated code
    // This function handles per-stage constants like bump env matrices
    // For now, we don't set these as the ubershader doesn't use them
}

void UniformManager::SetTexture(DWORD stage, bgfx::TextureHandle texture) {
    if (stage < 8) {
        bgfx::setTexture(stage, s_texture[stage], texture);
    }
}

void UniformManager::MatrixToFloatArray(const D3DMATRIX& m, float* out) {
    // D3D uses row-major, bgfx uses column-major, so transpose
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            out[j * 4 + i] = m.m[i][j];
        }
    }
}

D3DMATRIX UniformManager::MultiplyMatrix(const D3DMATRIX& a, const D3DMATRIX& b) {
    D3DMATRIX result;
    std::memset(&result, 0, sizeof(result));

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }

    return result;
}

} // namespace dx8bgfx
