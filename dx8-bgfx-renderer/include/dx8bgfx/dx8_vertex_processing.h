#pragma once

#include "dx8_types.h"
#include "dx8_constants.h"
#include "dx8_math.h"

#include <bgfx/bgfx.h>
#include <vector>

namespace dx8bgfx {

// =============================================================================
// Software Vertex Processing
// =============================================================================
// For CPU-side vertex transformation when needed (e.g., debugging, fallback)

class VertexProcessor {
public:
    VertexProcessor();
    ~VertexProcessor() = default;

    // Set transform matrices
    void SetWorldMatrix(const D3DMATRIX& world);
    void SetViewMatrix(const D3DMATRIX& view);
    void SetProjectionMatrix(const D3DMATRIX& proj);

    // Get combined matrices
    D3DMATRIX GetWorldViewMatrix() const;
    D3DMATRIX GetWorldViewProjMatrix() const;

    // Transform a position from object space to clip space
    void TransformPosition(const float* in, float* out) const;

    // Transform a position from object space to view space
    void TransformToViewSpace(const float* in, float* out) const;

    // Transform a normal from object space to view space
    void TransformNormal(const float* in, float* out) const;

    // Transform vertices in batch
    void TransformVertices(
        const void* srcVertices,
        void* dstVertices,
        uint32_t numVertices,
        DWORD fvf
    );

    // Generate software-lit vertices
    void LightVertices(
        void* vertices,
        uint32_t numVertices,
        DWORD fvf,
        const D3DMATERIAL8& material,
        const D3DLIGHT8* lights,
        const bool* lightEnabled,
        uint32_t numLights,
        D3DCOLOR ambient
    );

    // Apply fog to vertices
    void ApplyVertexFog(
        void* vertices,
        uint32_t numVertices,
        DWORD fvf,
        D3DFOGMODE mode,
        float start, float end, float density,
        bool rangeFog
    );

    // Clip vertices against a plane
    static uint32_t ClipTriangle(
        const float* v0, const float* v1, const float* v2,
        const float* plane,
        float* outVertices,
        uint32_t stride
    );

private:
    D3DMATRIX m_world;
    D3DMATRIX m_view;
    D3DMATRIX m_proj;
    D3DMATRIX m_worldView;
    D3DMATRIX m_worldViewProj;
    D3DMATRIX m_normalMatrix;
    bool m_dirty;

    void UpdateCombinedMatrices();
};

// =============================================================================
// Vertex Cache Optimization
// =============================================================================

class VertexCacheOptimizer {
public:
    // Optimize index buffer for vertex cache efficiency
    static void Optimize(
        uint16_t* indices,
        uint32_t numIndices,
        uint32_t numVertices
    );

    // Calculate ACMR (Average Cache Miss Ratio)
    static float CalculateACMR(
        const uint16_t* indices,
        uint32_t numIndices,
        uint32_t cacheSize = 16
    );
};

// =============================================================================
// Vertex Decompression
// =============================================================================

class VertexDecompressor {
public:
    // Decompress compressed vertex formats to standard floats
    static void DecompressNormals(
        const void* src, float* dst,
        uint32_t count, uint32_t srcStride,
        uint32_t format // 0=UBYTE4, 1=SHORT2, 2=SHORT4
    );

    // Decompress D3DCOLOR to float4
    static void DecompressColors(
        const uint32_t* src, float* dst,
        uint32_t count, uint32_t srcStride
    );

    // Decompress SHORT2 to float2
    static void DecompressShort2(
        const int16_t* src, float* dst,
        uint32_t count, uint32_t srcStride
    );
};

// =============================================================================
// Skinning Support
// =============================================================================

struct SkinningVertex {
    float position[3];
    float blendWeights[4];
    uint8_t blendIndices[4];
};

class SoftwareSkinning {
public:
    // Apply skinning transform to vertices
    static void ApplySkinning(
        const SkinningVertex* srcVertices,
        float* dstPositions,
        float* dstNormals,  // Can be null
        uint32_t numVertices,
        const D3DMATRIX* boneMatrices,
        uint32_t numBones
    );

    // Blend N bones for a single vertex
    static void BlendVertex(
        const float* position,
        const float* blendWeights,
        const uint8_t* blendIndices,
        uint32_t numWeights,
        const D3DMATRIX* boneMatrices,
        float* outPosition
    );
};

// =============================================================================
// Morph Target / Vertex Blending
// =============================================================================

class MorphTargetBlender {
public:
    // Blend between two vertex buffers
    static void Blend(
        const float* src0, const float* src1,
        float* dst,
        uint32_t numFloats,
        float t
    );

    // Blend multiple morph targets with weights
    static void BlendMultiple(
        const float** sources,
        const float* weights,
        uint32_t numSources,
        float* dst,
        uint32_t numFloats
    );
};

} // namespace dx8bgfx
