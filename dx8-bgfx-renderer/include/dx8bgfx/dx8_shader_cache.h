#pragma once

#include "dx8_shader_key.h"
#include "dx8_shader_generator.h"

#include <bgfx/bgfx.h>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

namespace dx8bgfx {

// =============================================================================
// Shader Program Entry
// =============================================================================

struct ShaderProgram {
    bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
    uint64_t lastUsedFrame = 0;
    bool ready = false;
};

// =============================================================================
// Compile Request
// =============================================================================

struct CompileRequest {
    VertexShaderKey vsKey;
    FragmentShaderKey fsKey;
    uint64_t hash;
};

// =============================================================================
// Shader Cache
// =============================================================================

class ShaderCache {
public:
    ShaderCache();
    ~ShaderCache();

    // Initialize cache
    void Init(uint32_t maxVariants = 5000, bool asyncCompilation = true);
    void Shutdown();

    // Get or compile shader for given keys
    // Returns BGFX_INVALID_HANDLE if not ready (async compilation)
    bgfx::ProgramHandle GetProgram(const VertexShaderKey& vsKey, const FragmentShaderKey& fsKey);

    // Get ubershader for immediate fallback
    bgfx::ProgramHandle GetUbershader();

    // Frame update - for LRU tracking
    void OnFrame(uint64_t frameNumber);

    // Statistics
    uint32_t GetCachedProgramCount() const;
    uint32_t GetPendingCompileCount() const;

    // Clear all cached shaders
    void Clear();

private:
    // Compile shader synchronously
    bgfx::ProgramHandle CompileProgram(const VertexShaderKey& vsKey, const FragmentShaderKey& fsKey);

    // Async compilation worker
    void CompileWorker();

    // Evict least recently used shaders
    void EvictLRU();

    // Compute combined hash
    uint64_t ComputeHash(const VertexShaderKey& vsKey, const FragmentShaderKey& fsKey) const;

private:
    // Shader generator
    ShaderGenerator m_generator;

    // Cache storage
    std::unordered_map<uint64_t, ShaderProgram> m_cache;
    mutable std::mutex m_cacheMutex;

    // Ubershader
    bgfx::ProgramHandle m_ubershader = BGFX_INVALID_HANDLE;

    // Configuration
    uint32_t m_maxVariants = 5000;
    bool m_asyncCompilation = true;

    // Async compilation
    std::queue<CompileRequest> m_compileQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    std::thread m_compileThread;
    std::atomic<bool> m_running{false};

    // Frame tracking
    uint64_t m_currentFrame = 0;
};

} // namespace dx8bgfx
