#pragma once

#include <bgfx/bgfx.h>
#include <string>
#include <vector>

namespace dx8bgfx {

// =============================================================================
// Shader Compiler
// =============================================================================

enum class ShaderStage {
    Vertex,
    Fragment
};

enum class ShaderProfile {
    GLSL,       // OpenGL
    SPIRV,      // Vulkan
    HLSL_DX11,  // DirectX 11
    HLSL_DX12,  // DirectX 12
    Metal,      // Metal
    Auto        // Auto-detect based on bgfx backend
};

struct CompileOptions {
    ShaderProfile profile = ShaderProfile::Auto;
    bool debug = false;
    bool optimize = true;
    std::string includePath;
    std::vector<std::string> defines;
};

class ShaderCompiler {
public:
    ShaderCompiler();
    ~ShaderCompiler();

    // Initialize compiler (find shaderc executable)
    bool Init(const std::string& shadercPath = "");

    // Compile shader from source string
    bgfx::ShaderHandle CompileShader(
        const std::string& source,
        ShaderStage stage,
        const std::string& name,
        const CompileOptions& options = {});

    // Compile shader from file
    bgfx::ShaderHandle CompileShaderFile(
        const std::string& filePath,
        ShaderStage stage,
        const CompileOptions& options = {});

    // Create program from two shaders
    bgfx::ProgramHandle CreateProgram(
        bgfx::ShaderHandle vs,
        bgfx::ShaderHandle fs,
        bool destroyShaders = true);

    // Compile and create program in one call
    bgfx::ProgramHandle CompileProgram(
        const std::string& vsSource,
        const std::string& fsSource,
        const std::string& name,
        const CompileOptions& options = {});

    // Get last error message
    const std::string& GetLastError() const { return m_lastError; }

    // Check if compiler is available
    bool IsAvailable() const { return m_available; }

private:
    // Get profile string for shaderc
    std::string GetProfileString(ShaderProfile profile, ShaderStage stage);

    // Get platform string
    std::string GetPlatformString();

    // Write temp file
    bool WriteFile(const std::string& path, const std::string& content);

    // Read binary file
    std::vector<uint8_t> ReadBinaryFile(const std::string& path);

    // Execute shaderc
    bool ExecuteShaderc(const std::string& args);

private:
    std::string m_shadercPath;
    std::string m_lastError;
    std::string m_tempDir;
    bool m_available = false;
};

// =============================================================================
// Embedded Shader Compiler (for platforms without shaderc)
// =============================================================================

// Simple in-memory shader for testing
// In production, you would pre-compile shaders offline
class EmbeddedShaderLoader {
public:
    // Load pre-compiled shader from memory
    static bgfx::ShaderHandle LoadFromMemory(
        const uint8_t* data,
        uint32_t size);

    // Load pre-compiled shader from file
    static bgfx::ShaderHandle LoadFromFile(
        const std::string& filePath);
};

} // namespace dx8bgfx
