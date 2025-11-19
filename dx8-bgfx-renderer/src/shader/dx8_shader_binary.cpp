// =============================================================================
// DX8-BGFX Shader Binary Implementation
// =============================================================================

#include "dx8bgfx/dx8_shader_binary.h"
#include <fstream>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace dx8bgfx {

// =============================================================================
// ShaderBinaryManager Implementation
// =============================================================================

ShaderBinaryManager::ShaderBinaryManager()
    : m_loadedCount(0)
    , m_initialized(false) {
}

ShaderBinaryManager::~ShaderBinaryManager() {
    Shutdown();
}

void ShaderBinaryManager::Initialize() {
    if (m_initialized) {
        return;
    }

    m_programs.clear();
    m_loadedCount = 0;
    m_initialized = true;
}

void ShaderBinaryManager::Shutdown() {
    if (!m_initialized) {
        return;
    }

    // Destroy all loaded programs
    for (auto& pair : m_programs) {
        if (pair.second.loaded && bgfx::isValid(pair.second.program)) {
            bgfx::destroy(pair.second.program);
        }
    }

    m_programs.clear();
    m_loadedCount = 0;
    m_initialized = false;
}

bool ShaderBinaryManager::LoadFromFile(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Read entire file
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);

    return LoadFromMemory(data.data(), static_cast<uint32_t>(size));
}

bool ShaderBinaryManager::LoadFromMemory(const void* data, uint32_t size) {
    if (!data || size < sizeof(ShaderBinaryHeader)) {
        return false;
    }

    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    const uint8_t* end = ptr + size;

    // Read header
    ShaderBinaryHeader header;
    std::memcpy(&header, ptr, sizeof(header));
    ptr += sizeof(header);

    // Validate header
    if (header.magic != SHADER_BINARY_MAGIC) {
        return false;
    }

    if (header.version != SHADER_BINARY_VERSION) {
        return false;
    }

    // Load each variant
    for (uint32_t i = 0; i < header.numVariants; ++i) {
        if (ptr + sizeof(ShaderVariantHeader) > end) {
            return false;
        }

        ShaderVariantHeader variantHeader;
        std::memcpy(&variantHeader, ptr, sizeof(variantHeader));
        ptr += sizeof(variantHeader);

        // Check bounds
        if (ptr + variantHeader.vsSize + variantHeader.fsSize > end) {
            return false;
        }

        // Load vertex shader
        bgfx::ShaderHandle vs = LoadShader(ptr, variantHeader.vsSize);
        ptr += variantHeader.vsSize;

        // Load fragment shader
        bgfx::ShaderHandle fs = LoadShader(ptr, variantHeader.fsSize);
        ptr += variantHeader.fsSize;

        // Create program
        if (bgfx::isValid(vs) && bgfx::isValid(fs)) {
            bgfx::ProgramHandle program = bgfx::createProgram(vs, fs, true);

            if (bgfx::isValid(program)) {
                ShaderEntry entry;
                entry.program = program;
                entry.loaded = true;
                m_programs[variantHeader.keyHash] = entry;
                m_loadedCount++;
            }
        } else {
            // Clean up on failure
            if (bgfx::isValid(vs)) bgfx::destroy(vs);
            if (bgfx::isValid(fs)) bgfx::destroy(fs);
        }
    }

    return true;
}

bool ShaderBinaryManager::SaveToFile(const char* path) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    // Write header
    ShaderBinaryHeader header;
    header.magic = SHADER_BINARY_MAGIC;
    header.version = SHADER_BINARY_VERSION;
    header.numVariants = static_cast<uint32_t>(m_programs.size());
    header.reserved = 0;

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Note: This saves program handles, not actual binary data
    // Real implementation would need to store the original binary data
    // This is a placeholder for the save functionality

    return true;
}

bool ShaderBinaryManager::HasVariant(const ShaderKey& key) const {
    uint64_t hash = key.GetHash();
    return m_programs.find(hash) != m_programs.end();
}

bgfx::ProgramHandle ShaderBinaryManager::GetProgram(const ShaderKey& key) {
    uint64_t hash = key.GetHash();
    auto it = m_programs.find(hash);

    if (it != m_programs.end() && it->second.loaded) {
        return it->second.program;
    }

    return BGFX_INVALID_HANDLE;
}

void ShaderBinaryManager::RegisterEmbeddedShaders(
    const EmbeddedShaderVariant* variants,
    uint32_t count) {

    bgfx::RendererType::Enum currentRenderer = bgfx::getRendererType();

    for (uint32_t i = 0; i < count; ++i) {
        const EmbeddedShaderVariant& variant = variants[i];

        // Check if this variant matches current renderer
        if (variant.vertexShader.renderer != currentRenderer) {
            continue;
        }

        // Load shaders
        bgfx::ShaderHandle vs = LoadShader(
            variant.vertexShader.data,
            variant.vertexShader.size
        );

        bgfx::ShaderHandle fs = LoadShader(
            variant.fragmentShader.data,
            variant.fragmentShader.size
        );

        // Create program
        if (bgfx::isValid(vs) && bgfx::isValid(fs)) {
            bgfx::ProgramHandle program = bgfx::createProgram(vs, fs, true);

            if (bgfx::isValid(program)) {
                uint64_t hash = variant.key.GetHash();
                ShaderEntry entry;
                entry.program = program;
                entry.loaded = true;
                m_programs[hash] = entry;
                m_loadedCount++;
            }
        } else {
            if (bgfx::isValid(vs)) bgfx::destroy(vs);
            if (bgfx::isValid(fs)) bgfx::destroy(fs);
        }
    }
}

bgfx::ShaderHandle ShaderBinaryManager::LoadShader(const uint8_t* data, uint32_t size) {
    if (!data || size == 0) {
        return BGFX_INVALID_HANDLE;
    }

    const bgfx::Memory* mem = bgfx::copy(data, size);
    return bgfx::createShader(mem);
}

// =============================================================================
// ShaderCompilerTool Implementation
// =============================================================================

bool ShaderCompilerTool::CompileAllVariants(
    const char* outputPath,
    bgfx::RendererType::Enum renderer,
    const char* shadercPath) {

    std::vector<ShaderKey> variants = GenerateCommonVariants();
    return CompileVariants(variants.data(), static_cast<uint32_t>(variants.size()),
                          outputPath, renderer);
}

bool ShaderCompilerTool::CompileVariants(
    const ShaderKey* keys,
    uint32_t count,
    const char* outputPath,
    bgfx::RendererType::Enum renderer) {

    std::ofstream file(outputPath, std::ios::binary);
    if (!file) {
        return false;
    }

    // Write header
    ShaderBinaryHeader header;
    header.magic = SHADER_BINARY_MAGIC;
    header.version = SHADER_BINARY_VERSION;
    header.numVariants = count;
    header.reserved = 0;

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Compile and write each variant
    for (uint32_t i = 0; i < count; ++i) {
        const ShaderKey& key = keys[i];

        // Generate shader source (placeholder - would use ShaderGenerator)
        std::string vsSource = "// Vertex shader placeholder\n";
        std::string fsSource = "// Fragment shader placeholder\n";

        // Compile shaders
        std::vector<uint8_t> vsData, fsData;

        if (!CompileShader(vsSource, "vertex", renderer, vsData)) {
            continue;
        }

        if (!CompileShader(fsSource, "fragment", renderer, fsData)) {
            continue;
        }

        // Write variant header
        ShaderVariantHeader variantHeader;
        variantHeader.keyHash = key.GetHash();
        variantHeader.vsSize = static_cast<uint32_t>(vsData.size());
        variantHeader.fsSize = static_cast<uint32_t>(fsData.size());

        file.write(reinterpret_cast<const char*>(&variantHeader), sizeof(variantHeader));
        file.write(reinterpret_cast<const char*>(vsData.data()), vsData.size());
        file.write(reinterpret_cast<const char*>(fsData.data()), fsData.size());
    }

    return true;
}

std::vector<ShaderKey> ShaderCompilerTool::GenerateCommonVariants() {
    std::vector<ShaderKey> variants;

    // Generate common shader variants

    // Basic variants: no textures, with/without lighting
    {
        ShaderKey key;
        key.SetLightingEnabled(false);
        key.SetTextureStageCount(0);
        variants.push_back(key);
    }

    {
        ShaderKey key;
        key.SetLightingEnabled(true);
        key.SetTextureStageCount(0);
        variants.push_back(key);
    }

    // Single texture variants
    for (uint32_t hasLighting = 0; hasLighting <= 1; ++hasLighting) {
        ShaderKey key;
        key.SetLightingEnabled(hasLighting != 0);
        key.SetTextureStageCount(1);
        key.SetTextureOp(0, D3DTOP_MODULATE);
        key.SetColorArg1(0, D3DTA_TEXTURE);
        key.SetColorArg2(0, D3DTA_DIFFUSE);
        variants.push_back(key);
    }

    // Two texture variants (common multitexture setups)
    for (uint32_t hasLighting = 0; hasLighting <= 1; ++hasLighting) {
        // Modulate + Modulate
        {
            ShaderKey key;
            key.SetLightingEnabled(hasLighting != 0);
            key.SetTextureStageCount(2);
            key.SetTextureOp(0, D3DTOP_MODULATE);
            key.SetColorArg1(0, D3DTA_TEXTURE);
            key.SetColorArg2(0, D3DTA_DIFFUSE);
            key.SetTextureOp(1, D3DTOP_MODULATE);
            key.SetColorArg1(1, D3DTA_TEXTURE);
            key.SetColorArg2(1, D3DTA_CURRENT);
            variants.push_back(key);
        }

        // Add + Modulate (for additive detail)
        {
            ShaderKey key;
            key.SetLightingEnabled(hasLighting != 0);
            key.SetTextureStageCount(2);
            key.SetTextureOp(0, D3DTOP_MODULATE);
            key.SetColorArg1(0, D3DTA_TEXTURE);
            key.SetColorArg2(0, D3DTA_DIFFUSE);
            key.SetTextureOp(1, D3DTOP_ADD);
            key.SetColorArg1(1, D3DTA_TEXTURE);
            key.SetColorArg2(1, D3DTA_CURRENT);
            variants.push_back(key);
        }
    }

    // Fog variants
    for (uint32_t fogMode = D3DFOG_LINEAR; fogMode <= D3DFOG_EXP2; ++fogMode) {
        ShaderKey key;
        key.SetLightingEnabled(true);
        key.SetTextureStageCount(1);
        key.SetTextureOp(0, D3DTOP_MODULATE);
        key.SetFogMode(fogMode);
        variants.push_back(key);
    }

    // Alpha test variants
    {
        ShaderKey key;
        key.SetLightingEnabled(true);
        key.SetTextureStageCount(1);
        key.SetTextureOp(0, D3DTOP_MODULATE);
        key.SetAlphaTestEnabled(true);
        key.SetAlphaFunc(D3DCMP_GREATER);
        variants.push_back(key);
    }

    // Specular enabled variants
    {
        ShaderKey key;
        key.SetLightingEnabled(true);
        key.SetSpecularEnabled(true);
        key.SetTextureStageCount(1);
        key.SetTextureOp(0, D3DTOP_MODULATE);
        variants.push_back(key);
    }

    return variants;
}

bool ShaderCompilerTool::CompileShader(
    const std::string& source,
    const char* type,
    bgfx::RendererType::Enum renderer,
    std::vector<uint8_t>& output) {

    // Placeholder implementation
    // Real implementation would invoke shaderc or similar tool
    // For now, just return empty data

    (void)source;
    (void)type;
    (void)renderer;

    output.clear();
    return false;
}

// =============================================================================
// ShaderPaths Implementation
// =============================================================================

std::string ShaderPaths::GetShaderDirectory() {
    std::string path;

#ifdef _WIN32
    // Windows: Use local app data
    char appData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appData))) {
        path = appData;
        path += "\\dx8bgfx\\shaders\\";
    }
#else
    // Unix: Use XDG cache directory or home
    const char* xdgCache = getenv("XDG_CACHE_HOME");
    if (xdgCache) {
        path = xdgCache;
        path += "/dx8bgfx/shaders/";
    } else {
        const char* home = getenv("HOME");
        if (home) {
            path = home;
            path += "/.cache/dx8bgfx/shaders/";
        }
    }
#endif

    return path;
}

std::string ShaderPaths::GetBinaryFilename(bgfx::RendererType::Enum renderer) {
    std::string filename = "shaders_";

    switch (renderer) {
        case bgfx::RendererType::Direct3D11:
            filename += "dx11";
            break;
        case bgfx::RendererType::Direct3D12:
            filename += "dx12";
            break;
        case bgfx::RendererType::OpenGL:
            filename += "gl";
            break;
        case bgfx::RendererType::OpenGLES:
            filename += "gles";
            break;
        case bgfx::RendererType::Vulkan:
            filename += "vk";
            break;
        case bgfx::RendererType::Metal:
            filename += "mtl";
            break;
        default:
            filename += "unknown";
            break;
    }

    filename += ".bin";
    return filename;
}

std::string ShaderPaths::GetBinaryPath(bgfx::RendererType::Enum renderer) {
    return GetShaderDirectory() + GetBinaryFilename(renderer);
}

} // namespace dx8bgfx
