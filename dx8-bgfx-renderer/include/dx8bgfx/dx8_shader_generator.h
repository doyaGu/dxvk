#pragma once

#include "dx8_shader_key.h"
#include <string>
#include <sstream>

namespace dx8bgfx {

// =============================================================================
// Shader Generator
// =============================================================================

class ShaderGenerator {
public:
    ShaderGenerator() = default;
    ~ShaderGenerator() = default;

    // Generate complete shaders
    std::string GenerateVertexShader(const VertexShaderKey& key);
    std::string GenerateFragmentShader(const FragmentShaderKey& key);

    // Get ubershader source (for fallback)
    static std::string GetUbershaderVertexSource();
    static std::string GetUbershaderFragmentSource();

private:
    // Vertex shader generation
    void EmitVSHeader();
    void EmitVSInputs(const VertexShaderKey& key);
    void EmitVSOutputs(const VertexShaderKey& key);
    void EmitVSUniforms(const VertexShaderKey& key);
    void EmitVSHelpers(const VertexShaderKey& key);
    void EmitVSMain(const VertexShaderKey& key);

    // Vertex shader components
    void EmitVertexTransform(const VertexShaderKey& key);
    void EmitVertexBlending(const VertexShaderKey& key);
    void EmitLighting(const VertexShaderKey& key);
    void EmitTexCoordGen(const VertexShaderKey& key);
    void EmitFog(const VertexShaderKey& key);

    // Fragment shader generation
    void EmitFSHeader();
    void EmitFSInputs(const FragmentShaderKey& key);
    void EmitFSUniforms(const FragmentShaderKey& key);
    void EmitFSHelpers(const FragmentShaderKey& key);
    void EmitFSMain(const FragmentShaderKey& key);

    // Fragment shader components
    void EmitTextureStage(const FragmentShaderKey& key, int stage);
    void EmitAlphaTest(const FragmentShaderKey& key);
    void EmitFogBlend(const FragmentShaderKey& key);

    // Helper methods
    std::string GetTextureArgCode(uint8_t arg, int stage);
    std::string GetColorOpCode(uint8_t op, const std::string& arg0,
                               const std::string& arg1, const std::string& arg2);
    std::string GetAlphaOpCode(uint8_t op, const std::string& arg0,
                               const std::string& arg1, const std::string& arg2);

    // Output stream
    std::stringstream m_code;

    // Helper to add line
    void Line(const std::string& code) {
        m_code << code << "\n";
    }

    void Line() {
        m_code << "\n";
    }
};

} // namespace dx8bgfx
