#pragma once

#include "dx8_types.h"
#include "dx8_shader_key.h"

#include <bgfx/bgfx.h>
#include <vector>
#include <unordered_map>
#include <string>

namespace dx8bgfx {

// =============================================================================
// Embedded Shader Data
// =============================================================================

struct EmbeddedShaderData {
    const uint8_t* data;
    uint32_t size;
    bgfx::RendererType::Enum renderer;
};

struct EmbeddedShaderVariant {
    ShaderKey key;
    EmbeddedShaderData vertexShader;
    EmbeddedShaderData fragmentShader;
};

// =============================================================================
// Shader Binary Manager
// =============================================================================

class ShaderBinaryManager {
public:
    ShaderBinaryManager();
    ~ShaderBinaryManager();

    // Initialize with embedded shaders
    void Initialize();
    void Shutdown();

    // Load shader binaries from file
    bool LoadFromFile(const char* path);

    // Load shader binaries from memory
    bool LoadFromMemory(const void* data, uint32_t size);

    // Save current shader cache to file
    bool SaveToFile(const char* path);

    // Check if a shader variant exists
    bool HasVariant(const ShaderKey& key) const;

    // Get shader program for a key
    bgfx::ProgramHandle GetProgram(const ShaderKey& key);

    // Register embedded shaders (for static linking)
    void RegisterEmbeddedShaders(const EmbeddedShaderVariant* variants, uint32_t count);

    // Get statistics
    uint32_t GetVariantCount() const { return static_cast<uint32_t>(m_programs.size()); }
    uint32_t GetLoadedCount() const { return m_loadedCount; }

private:
    struct ShaderEntry {
        bgfx::ProgramHandle program;
        bool loaded;
    };

    std::unordered_map<uint64_t, ShaderEntry> m_programs;
    uint32_t m_loadedCount;
    bool m_initialized;

    // Load shader from binary data
    bgfx::ShaderHandle LoadShader(const uint8_t* data, uint32_t size);
};

// =============================================================================
// Shader Binary File Format
// =============================================================================

// File format:
// Header (16 bytes)
// - Magic: "DX8B" (4 bytes)
// - Version: uint32_t
// - NumVariants: uint32_t
// - Reserved: uint32_t
//
// For each variant:
// - KeyHash: uint64_t
// - VSSize: uint32_t
// - FSSize: uint32_t
// - VSData: uint8_t[VSSize]
// - FSData: uint8_t[FSSize]

constexpr uint32_t SHADER_BINARY_MAGIC = 0x42385844; // "DX8B"
constexpr uint32_t SHADER_BINARY_VERSION = 1;

struct ShaderBinaryHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t numVariants;
    uint32_t reserved;
};

struct ShaderVariantHeader {
    uint64_t keyHash;
    uint32_t vsSize;
    uint32_t fsSize;
};

// =============================================================================
// Shader Compiler Tool Interface
// =============================================================================

class ShaderCompilerTool {
public:
    // Compile all shader variants and save to file
    static bool CompileAllVariants(
        const char* outputPath,
        bgfx::RendererType::Enum renderer,
        const char* shadercPath = nullptr
    );

    // Compile specific variants
    static bool CompileVariants(
        const ShaderKey* keys,
        uint32_t count,
        const char* outputPath,
        bgfx::RendererType::Enum renderer
    );

    // Generate common shader variants
    static std::vector<ShaderKey> GenerateCommonVariants();

private:
    static bool CompileShader(
        const std::string& source,
        const char* type,
        bgfx::RendererType::Enum renderer,
        std::vector<uint8_t>& output
    );
};

// =============================================================================
// Platform-Specific Shader Paths
// =============================================================================

class ShaderPaths {
public:
    // Get platform-specific shader directory
    static std::string GetShaderDirectory();

    // Get shader binary filename for current renderer
    static std::string GetBinaryFilename(bgfx::RendererType::Enum renderer);

    // Get full path to shader binary
    static std::string GetBinaryPath(bgfx::RendererType::Enum renderer);
};

} // namespace dx8bgfx
