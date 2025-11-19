#pragma once

#include "dx8_types.h"
#include "dx8_constants.h"
#include "dx8_state_manager.h"

#include <bgfx/bgfx.h>
#include <string>
#include <vector>

namespace dx8bgfx {

// =============================================================================
// Debug Utilities
// =============================================================================

class DebugUtils {
public:
    // State inspection
    static std::string RenderStateToString(D3DRENDERSTATETYPE state);
    static std::string TextureStageStateToString(D3DTEXTURESTAGESTATETYPE state);
    static std::string PrimitiveTypeToString(D3DPRIMITIVETYPE type);
    static std::string BlendModeToString(D3DBLEND blend);
    static std::string CmpFuncToString(D3DCMPFUNC func);
    static std::string FogModeToString(D3DFOGMODE mode);
    static std::string TextureOpToString(DWORD op);
    static std::string FVFToString(DWORD fvf);

    // Matrix debugging
    static std::string MatrixToString(const D3DMATRIX& m);
    static void PrintMatrix(const char* name, const D3DMATRIX& m);

    // Color debugging
    static std::string ColorToString(D3DCOLOR color);
    static std::string ColorValueToString(const D3DCOLORVALUE& cv);

    // Light debugging
    static std::string LightTypeToString(D3DLIGHTTYPE type);
    static std::string LightToString(const D3DLIGHT8& light);

    // Dump complete state
    static void DumpRenderStates(const StateManager& state);
    static void DumpTextureStageState(const StateManager& state, uint32_t stage);
    static void DumpMaterial(const D3DMATERIAL8& material);
    static void DumpLight(const D3DLIGHT8& light, uint32_t index);
};

// =============================================================================
// Performance Profiling
// =============================================================================

struct ProfilerStats {
    uint64_t frameTime;          // microseconds
    uint64_t cpuTime;            // CPU time per frame
    uint64_t gpuTime;            // GPU time per frame

    uint32_t drawCalls;
    uint32_t primitives;
    uint32_t vertices;
    uint32_t indices;

    uint32_t stateChanges;
    uint32_t textureChanges;
    uint32_t shaderChanges;

    uint32_t transientVbUsed;
    uint32_t transientIbUsed;

    float fps;
    float avgFrameTime;
};

class Profiler {
public:
    Profiler();
    ~Profiler();

    // Enable/disable profiling
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    // Frame markers
    void BeginFrame();
    void EndFrame();

    // Manual counters
    void AddDrawCall(uint32_t primitives, uint32_t vertices, uint32_t indices);
    void AddStateChange();
    void AddTextureChange();
    void AddShaderChange();

    // Get current stats
    const ProfilerStats& GetStats() const { return m_stats; }

    // Get averaged stats
    ProfilerStats GetAverageStats(uint32_t numFrames = 60) const;

    // Reset counters
    void Reset();

    // Print stats to debug output
    void PrintStats();

private:
    bool m_enabled;
    ProfilerStats m_stats;
    std::vector<ProfilerStats> m_history;

    uint64_t m_frameStartTime;
    uint32_t m_maxHistory;
};

// =============================================================================
// Debug Drawing
// =============================================================================

class DebugDraw {
public:
    DebugDraw();
    ~DebugDraw();

    // Initialize debug drawing
    void Initialize();
    void Shutdown();

    // Begin/end frame
    void Begin(const D3DMATRIX& viewProj);
    void End();

    // Line drawing
    void DrawLine(const D3DVECTOR& start, const D3DVECTOR& end, uint32_t color);
    void DrawLineList(const D3DVECTOR* points, uint32_t count, uint32_t color);

    // Shapes
    void DrawBox(const D3DVECTOR& min, const D3DVECTOR& max, uint32_t color);
    void DrawWireBox(const D3DVECTOR& min, const D3DVECTOR& max, uint32_t color);
    void DrawSphere(const D3DVECTOR& center, float radius, uint32_t color, uint32_t segments = 16);
    void DrawWireSphere(const D3DVECTOR& center, float radius, uint32_t color, uint32_t segments = 16);
    void DrawCone(const D3DVECTOR& apex, const D3DVECTOR& base, float radius, uint32_t color, uint32_t segments = 16);

    // Coordinate axes
    void DrawAxes(const D3DMATRIX& transform, float size = 1.0f);
    void DrawGrid(float size, uint32_t divisions, uint32_t color);

    // Frustum
    void DrawFrustum(const D3DMATRIX& viewProj, uint32_t color);

    // Light visualization
    void DrawLight(const D3DLIGHT8& light, uint32_t color);

    // Text (screen space)
    void DrawText(float x, float y, const char* text, uint32_t color = 0xFFFFFFFF);

private:
    struct DebugVertex {
        float pos[3];
        uint32_t color;
    };

    std::vector<DebugVertex> m_lines;
    bgfx::VertexLayout m_layout;
    bool m_initialized;
    D3DMATRIX m_viewProj;
};

// =============================================================================
// Shader Debug
// =============================================================================

class ShaderDebug {
public:
    // Dump generated shader code
    static void DumpVertexShader(const std::string& code, const char* filename = nullptr);
    static void DumpFragmentShader(const std::string& code, const char* filename = nullptr);

    // Validate shader key
    static bool ValidateShaderKey(const ShaderKey& key);

    // Get shader statistics
    static void GetShaderCacheStats(
        uint32_t& numVariants,
        uint32_t& numHits,
        uint32_t& numMisses
    );
};

// =============================================================================
// Memory Tracking
// =============================================================================

class MemoryTracker {
public:
    static void BeginTracking();
    static void EndTracking();

    static size_t GetCurrentAllocation();
    static size_t GetPeakAllocation();
    static size_t GetTotalAllocated();
    static size_t GetTotalFreed();

    static void PrintMemoryReport();
};

// =============================================================================
// Validation
// =============================================================================

class Validator {
public:
    // Validate render state combinations
    static bool ValidateRenderStates(const StateManager& state, std::string& errorMsg);

    // Validate FVF
    static bool ValidateFVF(DWORD fvf, std::string& errorMsg);

    // Validate texture stage setup
    static bool ValidateTextureStages(const StateManager& state, std::string& errorMsg);

    // Validate draw call parameters
    static bool ValidateDrawCall(
        D3DPRIMITIVETYPE primType,
        UINT startVertex,
        UINT primitiveCount,
        std::string& errorMsg
    );

    // Validate indexed draw call
    static bool ValidateIndexedDrawCall(
        D3DPRIMITIVETYPE primType,
        UINT minIndex,
        UINT numVertices,
        UINT startIndex,
        UINT primitiveCount,
        std::string& errorMsg
    );
};

} // namespace dx8bgfx
