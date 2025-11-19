#include "dx8bgfx/dx8_shader_cache.h"
#include <algorithm>
#include <vector>

namespace dx8bgfx {

ShaderCache::ShaderCache() = default;

ShaderCache::~ShaderCache() {
    Shutdown();
}

void ShaderCache::Init(uint32_t maxVariants, bool asyncCompilation) {
    m_maxVariants = maxVariants;
    m_asyncCompilation = asyncCompilation;

    // Compile ubershader
    std::string vsSource = ShaderGenerator::GetUbershaderVertexSource();
    std::string fsSource = ShaderGenerator::GetUbershaderFragmentSource();

    // Note: In a real implementation, we would compile these sources
    // using bgfx's shader compiler (shaderc) at runtime or use pre-compiled binaries.
    // For now, we'll use a placeholder.
    // m_ubershader = CompileFromSource(vsSource, fsSource);

    // Start async compilation thread if enabled
    if (m_asyncCompilation) {
        m_running = true;
        m_compileThread = std::thread(&ShaderCache::CompileWorker, this);
    }
}

void ShaderCache::Shutdown() {
    // Stop async thread
    if (m_asyncCompilation && m_running) {
        m_running = false;
        m_queueCondition.notify_all();
        if (m_compileThread.joinable()) {
            m_compileThread.join();
        }
    }

    // Destroy all cached programs
    Clear();

    // Destroy ubershader
    if (bgfx::isValid(m_ubershader)) {
        bgfx::destroy(m_ubershader);
        m_ubershader = BGFX_INVALID_HANDLE;
    }
}

bgfx::ProgramHandle ShaderCache::GetProgram(const VertexShaderKey& vsKey, const FragmentShaderKey& fsKey) {
    uint64_t hash = ComputeHash(vsKey, fsKey);

    // Check cache
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        auto it = m_cache.find(hash);
        if (it != m_cache.end()) {
            it->second.lastUsedFrame = m_currentFrame;
            if (it->second.ready) {
                return it->second.program;
            } else {
                // Still compiling, return ubershader
                return m_ubershader;
            }
        }
    }

    // Not in cache - need to compile
    if (m_asyncCompilation) {
        // Queue for async compilation
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_compileQueue.push({vsKey, fsKey, hash});
        }
        m_queueCondition.notify_one();

        // Add placeholder to cache
        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            ShaderProgram& entry = m_cache[hash];
            entry.program = BGFX_INVALID_HANDLE;
            entry.lastUsedFrame = m_currentFrame;
            entry.ready = false;
        }

        // Return ubershader while compiling
        return m_ubershader;
    } else {
        // Compile synchronously
        bgfx::ProgramHandle program = CompileProgram(vsKey, fsKey);

        // Add to cache
        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);

            // Evict if necessary
            if (m_cache.size() >= m_maxVariants) {
                EvictLRU();
            }

            ShaderProgram& entry = m_cache[hash];
            entry.program = program;
            entry.lastUsedFrame = m_currentFrame;
            entry.ready = true;
        }

        return program;
    }
}

bgfx::ProgramHandle ShaderCache::GetUbershader() {
    return m_ubershader;
}

void ShaderCache::OnFrame(uint64_t frameNumber) {
    m_currentFrame = frameNumber;
}

uint32_t ShaderCache::GetCachedProgramCount() const {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    return static_cast<uint32_t>(m_cache.size());
}

uint32_t ShaderCache::GetPendingCompileCount() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return static_cast<uint32_t>(m_compileQueue.size());
}

void ShaderCache::Clear() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    for (auto& pair : m_cache) {
        if (bgfx::isValid(pair.second.program)) {
            bgfx::destroy(pair.second.program);
        }
    }
    m_cache.clear();
}

bgfx::ProgramHandle ShaderCache::CompileProgram(const VertexShaderKey& vsKey, const FragmentShaderKey& fsKey) {
    // Generate shader source
    std::string vsSource = m_generator.GenerateVertexShader(vsKey);
    std::string fsSource = m_generator.GenerateFragmentShader(fsKey);

    // In a real implementation, we would:
    // 1. Write sources to temp files
    // 2. Call shaderc to compile them
    // 3. Load the compiled binaries
    // 4. Create bgfx shader handles

    // For now, return invalid handle as placeholder
    // The actual compilation would look something like:
    /*
    bgfx::ShaderHandle vsh = CompileShaderFromSource(vsSource, "vertex");
    bgfx::ShaderHandle fsh = CompileShaderFromSource(fsSource, "fragment");

    if (bgfx::isValid(vsh) && bgfx::isValid(fsh)) {
        return bgfx::createProgram(vsh, fsh, true);
    }
    */

    return BGFX_INVALID_HANDLE;
}

void ShaderCache::CompileWorker() {
    while (m_running) {
        CompileRequest request;
        bool hasRequest = false;

        // Get request from queue
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCondition.wait(lock, [this] {
                return !m_compileQueue.empty() || !m_running;
            });

            if (!m_running) {
                break;
            }

            if (!m_compileQueue.empty()) {
                request = m_compileQueue.front();
                m_compileQueue.pop();
                hasRequest = true;
            }
        }

        // Compile shader
        if (hasRequest) {
            bgfx::ProgramHandle program = CompileProgram(request.vsKey, request.fsKey);

            // Update cache
            {
                std::lock_guard<std::mutex> lock(m_cacheMutex);
                auto it = m_cache.find(request.hash);
                if (it != m_cache.end()) {
                    it->second.program = program;
                    it->second.ready = true;
                }
            }
        }
    }
}

void ShaderCache::EvictLRU() {
    // Find entries to evict (oldest 10%)
    if (m_cache.size() < 10) return;

    std::vector<std::pair<uint64_t, uint64_t>> entries; // hash, lastUsedFrame
    entries.reserve(m_cache.size());

    for (const auto& pair : m_cache) {
        entries.emplace_back(pair.first, pair.second.lastUsedFrame);
    }

    // Sort by last used frame
    std::sort(entries.begin(), entries.end(),
        [](const auto& a, const auto& b) {
            return a.second < b.second;
        });

    // Evict oldest 10%
    size_t evictCount = m_cache.size() / 10;
    for (size_t i = 0; i < evictCount && i < entries.size(); i++) {
        auto it = m_cache.find(entries[i].first);
        if (it != m_cache.end()) {
            if (bgfx::isValid(it->second.program)) {
                bgfx::destroy(it->second.program);
            }
            m_cache.erase(it);
        }
    }
}

uint64_t ShaderCache::ComputeHash(const VertexShaderKey& vsKey, const FragmentShaderKey& fsKey) const {
    return vsKey.GetHash() ^ (fsKey.GetHash() << 1);
}

} // namespace dx8bgfx
