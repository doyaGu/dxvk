#include "dx8bgfx/dx8_shader_compiler.h"

#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <array>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define popen _popen
#define pclose _pclose
#define mkdir(path, mode) _mkdir(path)
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace dx8bgfx {

ShaderCompiler::ShaderCompiler() {
    // Set up temp directory
#ifdef _WIN32
    char temp[MAX_PATH];
    GetTempPathA(MAX_PATH, temp);
    m_tempDir = std::string(temp) + "dx8bgfx_shaders\\";
    mkdir(m_tempDir.c_str(), 0755);
#else
    m_tempDir = "/tmp/dx8bgfx_shaders/";
    mkdir(m_tempDir.c_str(), 0755);
#endif
}

ShaderCompiler::~ShaderCompiler() = default;

bool ShaderCompiler::Init(const std::string& shadercPath) {
    // Try to find shaderc
    if (!shadercPath.empty()) {
        m_shadercPath = shadercPath;
    } else {
        // Try common locations
        std::vector<std::string> searchPaths = {
            "shaderc",
            "./shaderc",
            "../bgfx/.build/linux64_gcc/bin/shadercRelease",
            "../bgfx/.build/win64_vs2019/bin/shadercRelease.exe",
            "/usr/local/bin/shaderc",
            "C:\\bgfx\\tools\\bin\\windows\\shaderc.exe"
        };

        for (const auto& path : searchPaths) {
            // Test if executable exists
            std::string testCmd = path + " --help";
#ifdef _WIN32
            testCmd += " > NUL 2>&1";
#else
            testCmd += " > /dev/null 2>&1";
#endif
            if (system(testCmd.c_str()) == 0) {
                m_shadercPath = path;
                break;
            }
        }
    }

    m_available = !m_shadercPath.empty();

    if (!m_available) {
        m_lastError = "shaderc not found. Please specify path or ensure it's in PATH.";
    }

    return m_available;
}

bgfx::ShaderHandle ShaderCompiler::CompileShader(
    const std::string& source,
    ShaderStage stage,
    const std::string& name,
    const CompileOptions& options)
{
    if (!m_available) {
        m_lastError = "Shader compiler not initialized";
        return BGFX_INVALID_HANDLE;
    }

    // Write source to temp file
    std::string ext = (stage == ShaderStage::Vertex) ? ".vs.sc" : ".fs.sc";
    std::string inputPath = m_tempDir + name + ext;
    std::string outputPath = m_tempDir + name + ".bin";

    if (!WriteFile(inputPath, source)) {
        m_lastError = "Failed to write shader source to temp file";
        return BGFX_INVALID_HANDLE;
    }

    // Build shaderc command
    std::stringstream cmd;
    cmd << m_shadercPath;
    cmd << " -f \"" << inputPath << "\"";
    cmd << " -o \"" << outputPath << "\"";
    cmd << " --type " << ((stage == ShaderStage::Vertex) ? "vertex" : "fragment");
    cmd << " --platform " << GetPlatformString();

    // Get profile
    ShaderProfile profile = options.profile;
    if (profile == ShaderProfile::Auto) {
        // Auto-detect based on bgfx backend
        bgfx::RendererType::Enum renderer = bgfx::getRendererType();
        switch (renderer) {
            case bgfx::RendererType::OpenGL:
            case bgfx::RendererType::OpenGLES:
                profile = ShaderProfile::GLSL;
                break;
            case bgfx::RendererType::Vulkan:
                profile = ShaderProfile::SPIRV;
                break;
            case bgfx::RendererType::Direct3D11:
                profile = ShaderProfile::HLSL_DX11;
                break;
            case bgfx::RendererType::Direct3D12:
                profile = ShaderProfile::HLSL_DX12;
                break;
            case bgfx::RendererType::Metal:
                profile = ShaderProfile::Metal;
                break;
            default:
                profile = ShaderProfile::SPIRV;
                break;
        }
    }

    cmd << " -p " << GetProfileString(profile, stage);

    // Include path
    if (!options.includePath.empty()) {
        cmd << " -i \"" << options.includePath << "\"";
    }

    // Defines
    for (const auto& define : options.defines) {
        cmd << " --define " << define;
    }

    // Debug/optimize
    if (options.debug) {
        cmd << " --debug";
    }
    if (options.optimize) {
        cmd << " -O 3";
    }

    // Execute
    if (!ExecuteShaderc(cmd.str())) {
        return BGFX_INVALID_HANDLE;
    }

    // Load compiled shader
    std::vector<uint8_t> data = ReadBinaryFile(outputPath);
    if (data.empty()) {
        m_lastError = "Failed to read compiled shader";
        return BGFX_INVALID_HANDLE;
    }

    // Create bgfx shader
    const bgfx::Memory* mem = bgfx::copy(data.data(), data.size());
    bgfx::ShaderHandle handle = bgfx::createShader(mem);

    if (!bgfx::isValid(handle)) {
        m_lastError = "Failed to create bgfx shader";
        return BGFX_INVALID_HANDLE;
    }

    // Set debug name
    bgfx::setName(handle, name.c_str());

    return handle;
}

bgfx::ShaderHandle ShaderCompiler::CompileShaderFile(
    const std::string& filePath,
    ShaderStage stage,
    const CompileOptions& options)
{
    // Read file
    std::ifstream file(filePath);
    if (!file) {
        m_lastError = "Failed to open shader file: " + filePath;
        return BGFX_INVALID_HANDLE;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    // Extract name from path
    std::string name = filePath;
    size_t lastSlash = name.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        name = name.substr(lastSlash + 1);
    }
    size_t lastDot = name.find_last_of('.');
    if (lastDot != std::string::npos) {
        name = name.substr(0, lastDot);
    }

    return CompileShader(buffer.str(), stage, name, options);
}

bgfx::ProgramHandle ShaderCompiler::CreateProgram(
    bgfx::ShaderHandle vs,
    bgfx::ShaderHandle fs,
    bool destroyShaders)
{
    if (!bgfx::isValid(vs) || !bgfx::isValid(fs)) {
        m_lastError = "Invalid shader handles";
        return BGFX_INVALID_HANDLE;
    }

    return bgfx::createProgram(vs, fs, destroyShaders);
}

bgfx::ProgramHandle ShaderCompiler::CompileProgram(
    const std::string& vsSource,
    const std::string& fsSource,
    const std::string& name,
    const CompileOptions& options)
{
    bgfx::ShaderHandle vs = CompileShader(vsSource, ShaderStage::Vertex, name + "_vs", options);
    if (!bgfx::isValid(vs)) {
        return BGFX_INVALID_HANDLE;
    }

    bgfx::ShaderHandle fs = CompileShader(fsSource, ShaderStage::Fragment, name + "_fs", options);
    if (!bgfx::isValid(fs)) {
        bgfx::destroy(vs);
        return BGFX_INVALID_HANDLE;
    }

    return CreateProgram(vs, fs, true);
}

std::string ShaderCompiler::GetProfileString(ShaderProfile profile, ShaderStage stage) {
    switch (profile) {
        case ShaderProfile::GLSL:
            return "440";
        case ShaderProfile::SPIRV:
            return "spirv";
        case ShaderProfile::HLSL_DX11:
            return (stage == ShaderStage::Vertex) ? "vs_5_0" : "ps_5_0";
        case ShaderProfile::HLSL_DX12:
            return (stage == ShaderStage::Vertex) ? "vs_5_0" : "ps_5_0";
        case ShaderProfile::Metal:
            return "metal";
        default:
            return "spirv";
    }
}

std::string ShaderCompiler::GetPlatformString() {
#ifdef _WIN32
    return "windows";
#elif defined(__APPLE__)
    return "osx";
#elif defined(__linux__)
    return "linux";
#else
    return "linux";
#endif
}

bool ShaderCompiler::WriteFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file) return false;
    file << content;
    return true;
}

std::vector<uint8_t> ShaderCompiler::ReadBinaryFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return {};

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return {};
    }

    return buffer;
}

bool ShaderCompiler::ExecuteShaderc(const std::string& args) {
    // Redirect stderr to capture errors
    std::string cmd = args;
#ifdef _WIN32
    cmd += " 2>&1";
#else
    cmd += " 2>&1";
#endif

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        m_lastError = "Failed to execute shaderc";
        return false;
    }

    // Read output
    std::array<char, 256> buffer;
    std::string output;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }

    int result = pclose(pipe);

    if (result != 0) {
        m_lastError = "Shader compilation failed:\n" + output;
        return false;
    }

    return true;
}

// =============================================================================
// Embedded Shader Loader
// =============================================================================

bgfx::ShaderHandle EmbeddedShaderLoader::LoadFromMemory(
    const uint8_t* data,
    uint32_t size)
{
    const bgfx::Memory* mem = bgfx::copy(data, size);
    return bgfx::createShader(mem);
}

bgfx::ShaderHandle EmbeddedShaderLoader::LoadFromFile(
    const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        return BGFX_INVALID_HANDLE;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return BGFX_INVALID_HANDLE;
    }

    return LoadFromMemory(buffer.data(), static_cast<uint32_t>(buffer.size()));
}

} // namespace dx8bgfx
