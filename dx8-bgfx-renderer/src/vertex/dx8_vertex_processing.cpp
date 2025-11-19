#include "dx8bgfx/dx8_vertex_processing.h"
#include "dx8bgfx/dx8_buffer_utils.h"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace dx8bgfx {

// =============================================================================
// Vertex Processor Implementation
// =============================================================================

VertexProcessor::VertexProcessor()
    : m_dirty(true)
{
    // Initialize to identity
    MatrixIdentity(&m_world);
    MatrixIdentity(&m_view);
    MatrixIdentity(&m_proj);
    MatrixIdentity(&m_worldView);
    MatrixIdentity(&m_worldViewProj);
    MatrixIdentity(&m_normalMatrix);
}

void VertexProcessor::SetWorldMatrix(const D3DMATRIX& world) {
    m_world = world;
    m_dirty = true;
}

void VertexProcessor::SetViewMatrix(const D3DMATRIX& view) {
    m_view = view;
    m_dirty = true;
}

void VertexProcessor::SetProjectionMatrix(const D3DMATRIX& proj) {
    m_proj = proj;
    m_dirty = true;
}

void VertexProcessor::UpdateCombinedMatrices() {
    if (!m_dirty) return;

    MatrixMultiply(&m_worldView, &m_world, &m_view);
    MatrixMultiply(&m_worldViewProj, &m_worldView, &m_proj);

    // Normal matrix is inverse transpose of worldView (for non-uniform scaling)
    // Simplified: just use worldView if no non-uniform scaling
    m_normalMatrix = m_worldView;

    m_dirty = false;
}

D3DMATRIX VertexProcessor::GetWorldViewMatrix() const {
    const_cast<VertexProcessor*>(this)->UpdateCombinedMatrices();
    return m_worldView;
}

D3DMATRIX VertexProcessor::GetWorldViewProjMatrix() const {
    const_cast<VertexProcessor*>(this)->UpdateCombinedMatrices();
    return m_worldViewProj;
}

void VertexProcessor::TransformPosition(const float* in, float* out) const {
    const_cast<VertexProcessor*>(this)->UpdateCombinedMatrices();

    float x = in[0], y = in[1], z = in[2];
    float w = m_worldViewProj._14 * x + m_worldViewProj._24 * y +
              m_worldViewProj._34 * z + m_worldViewProj._44;

    out[0] = (m_worldViewProj._11 * x + m_worldViewProj._21 * y +
              m_worldViewProj._31 * z + m_worldViewProj._41) / w;
    out[1] = (m_worldViewProj._12 * x + m_worldViewProj._22 * y +
              m_worldViewProj._32 * z + m_worldViewProj._42) / w;
    out[2] = (m_worldViewProj._13 * x + m_worldViewProj._23 * y +
              m_worldViewProj._33 * z + m_worldViewProj._43) / w;
    out[3] = 1.0f / w;  // rhw
}

void VertexProcessor::TransformToViewSpace(const float* in, float* out) const {
    const_cast<VertexProcessor*>(this)->UpdateCombinedMatrices();

    float x = in[0], y = in[1], z = in[2];
    out[0] = m_worldView._11 * x + m_worldView._21 * y + m_worldView._31 * z + m_worldView._41;
    out[1] = m_worldView._12 * x + m_worldView._22 * y + m_worldView._32 * z + m_worldView._42;
    out[2] = m_worldView._13 * x + m_worldView._23 * y + m_worldView._33 * z + m_worldView._43;
}

void VertexProcessor::TransformNormal(const float* in, float* out) const {
    const_cast<VertexProcessor*>(this)->UpdateCombinedMatrices();

    float x = in[0], y = in[1], z = in[2];
    out[0] = m_normalMatrix._11 * x + m_normalMatrix._21 * y + m_normalMatrix._31 * z;
    out[1] = m_normalMatrix._12 * x + m_normalMatrix._22 * y + m_normalMatrix._32 * z;
    out[2] = m_normalMatrix._13 * x + m_normalMatrix._23 * y + m_normalMatrix._33 * z;

    // Normalize
    float len = std::sqrt(out[0]*out[0] + out[1]*out[1] + out[2]*out[2]);
    if (len > 0.0001f) {
        out[0] /= len;
        out[1] /= len;
        out[2] /= len;
    }
}

void VertexProcessor::TransformVertices(
    const void* srcVertices,
    void* dstVertices,
    uint32_t numVertices,
    DWORD fvf
) {
    UpdateCombinedMatrices();

    uint32_t stride = VertexBufferUtils::CalculateVertexStride(fvf);
    const uint8_t* src = static_cast<const uint8_t*>(srcVertices);
    uint8_t* dst = static_cast<uint8_t*>(dstVertices);

    bool hasNormal = (fvf & D3DFVF_NORMAL) != 0;
    uint32_t normalOffset = 12; // After position

    for (uint32_t i = 0; i < numVertices; i++) {
        // Transform position
        const float* srcPos = reinterpret_cast<const float*>(src);
        float* dstPos = reinterpret_cast<float*>(dst);
        TransformPosition(srcPos, dstPos);

        // Transform normal if present
        if (hasNormal) {
            const float* srcNorm = reinterpret_cast<const float*>(src + normalOffset);
            float* dstNorm = reinterpret_cast<float*>(dst + normalOffset);
            TransformNormal(srcNorm, dstNorm);
        }

        src += stride;
        dst += stride;
    }
}

void VertexProcessor::ApplyVertexFog(
    void* vertices,
    uint32_t numVertices,
    DWORD fvf,
    D3DFOGMODE mode,
    float start, float end, float density,
    bool rangeFog
) {
    if (mode == D3DFOG_NONE) return;
    if (!(fvf & D3DFVF_SPECULAR)) return; // Fog stored in specular alpha

    UpdateCombinedMatrices();

    uint32_t stride = VertexBufferUtils::CalculateVertexStride(fvf);
    uint8_t* ptr = static_cast<uint8_t*>(vertices);

    // Calculate specular offset
    uint32_t offset = 12; // Position
    if (fvf & D3DFVF_NORMAL) offset += 12;
    if (fvf & D3DFVF_PSIZE) offset += 4;
    if (fvf & D3DFVF_DIFFUSE) offset += 4;

    for (uint32_t i = 0; i < numVertices; i++) {
        // Get position and transform to view space
        const float* pos = reinterpret_cast<const float*>(ptr);
        float viewPos[3];
        TransformToViewSpace(pos, viewPos);

        // Calculate distance
        float dist;
        if (rangeFog) {
            dist = std::sqrt(viewPos[0]*viewPos[0] + viewPos[1]*viewPos[1] + viewPos[2]*viewPos[2]);
        } else {
            dist = std::abs(viewPos[2]);
        }

        // Calculate fog factor
        float fog = 1.0f;
        switch (mode) {
            case D3DFOG_LINEAR:
                fog = (end != start) ? (end - dist) / (end - start) : 1.0f;
                break;
            case D3DFOG_EXP:
                fog = std::exp(-density * dist);
                break;
            case D3DFOG_EXP2:
                fog = std::exp(-density * density * dist * dist);
                break;
            default:
                break;
        }
        fog = std::max(0.0f, std::min(1.0f, fog));

        // Store in specular alpha
        uint32_t* specular = reinterpret_cast<uint32_t*>(ptr + offset);
        uint32_t fogByte = static_cast<uint32_t>(fog * 255.0f);
        *specular = (*specular & 0x00FFFFFF) | (fogByte << 24);

        ptr += stride;
    }
}

// =============================================================================
// Vertex Cache Optimizer Implementation
// =============================================================================

float VertexCacheOptimizer::CalculateACMR(
    const uint16_t* indices,
    uint32_t numIndices,
    uint32_t cacheSize
) {
    if (numIndices == 0) return 0.0f;

    std::vector<int> cache(cacheSize, -1);
    uint32_t misses = 0;
    uint32_t cachePos = 0;

    for (uint32_t i = 0; i < numIndices; i++) {
        int index = indices[i];
        bool found = false;

        for (uint32_t j = 0; j < cacheSize; j++) {
            if (cache[j] == index) {
                found = true;
                break;
            }
        }

        if (!found) {
            misses++;
            cache[cachePos] = index;
            cachePos = (cachePos + 1) % cacheSize;
        }
    }

    return float(misses) / float(numIndices / 3);
}

void VertexCacheOptimizer::Optimize(
    uint16_t* indices,
    uint32_t numIndices,
    uint32_t numVertices
) {
    // Simple optimization: Forsyth algorithm would be better
    // This is a placeholder for a proper implementation
    (void)indices;
    (void)numIndices;
    (void)numVertices;
}

// =============================================================================
// Vertex Decompressor Implementation
// =============================================================================

void VertexDecompressor::DecompressNormals(
    const void* src, float* dst,
    uint32_t count, uint32_t srcStride,
    uint32_t format
) {
    const uint8_t* ptr = static_cast<const uint8_t*>(src);

    for (uint32_t i = 0; i < count; i++) {
        switch (format) {
            case 0: { // UBYTE4
                const uint8_t* n = ptr;
                dst[0] = (n[0] / 127.5f) - 1.0f;
                dst[1] = (n[1] / 127.5f) - 1.0f;
                dst[2] = (n[2] / 127.5f) - 1.0f;
                break;
            }
            case 1: { // SHORT2
                const int16_t* n = reinterpret_cast<const int16_t*>(ptr);
                dst[0] = n[0] / 32767.0f;
                dst[1] = n[1] / 32767.0f;
                dst[2] = std::sqrt(std::max(0.0f, 1.0f - dst[0]*dst[0] - dst[1]*dst[1]));
                break;
            }
            case 2: { // SHORT4
                const int16_t* n = reinterpret_cast<const int16_t*>(ptr);
                dst[0] = n[0] / 32767.0f;
                dst[1] = n[1] / 32767.0f;
                dst[2] = n[2] / 32767.0f;
                break;
            }
        }

        ptr += srcStride;
        dst += 3;
    }
}

void VertexDecompressor::DecompressColors(
    const uint32_t* src, float* dst,
    uint32_t count, uint32_t srcStride
) {
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(src);

    for (uint32_t i = 0; i < count; i++) {
        uint32_t color = *reinterpret_cast<const uint32_t*>(ptr);
        dst[0] = float((color >> 16) & 0xFF) / 255.0f; // R (BGRA)
        dst[1] = float((color >> 8) & 0xFF) / 255.0f;  // G
        dst[2] = float(color & 0xFF) / 255.0f;         // B
        dst[3] = float((color >> 24) & 0xFF) / 255.0f; // A

        ptr += srcStride;
        dst += 4;
    }
}

void VertexDecompressor::DecompressShort2(
    const int16_t* src, float* dst,
    uint32_t count, uint32_t srcStride
) {
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(src);

    for (uint32_t i = 0; i < count; i++) {
        const int16_t* s = reinterpret_cast<const int16_t*>(ptr);
        dst[0] = s[0] / 32767.0f;
        dst[1] = s[1] / 32767.0f;

        ptr += srcStride;
        dst += 2;
    }
}

// =============================================================================
// Software Skinning Implementation
// =============================================================================

void SoftwareSkinning::BlendVertex(
    const float* position,
    const float* blendWeights,
    const uint8_t* blendIndices,
    uint32_t numWeights,
    const D3DMATRIX* boneMatrices,
    float* outPosition
) {
    outPosition[0] = 0.0f;
    outPosition[1] = 0.0f;
    outPosition[2] = 0.0f;

    float totalWeight = 0.0f;
    for (uint32_t i = 0; i < numWeights; i++) {
        float weight = blendWeights[i];
        if (weight <= 0.0f) continue;

        uint8_t boneIndex = blendIndices[i];
        const D3DMATRIX& bone = boneMatrices[boneIndex];

        float x = position[0], y = position[1], z = position[2];
        outPosition[0] += weight * (bone._11 * x + bone._21 * y + bone._31 * z + bone._41);
        outPosition[1] += weight * (bone._12 * x + bone._22 * y + bone._32 * z + bone._42);
        outPosition[2] += weight * (bone._13 * x + bone._23 * y + bone._33 * z + bone._43);

        totalWeight += weight;
    }

    // Normalize if weights don't sum to 1
    if (totalWeight > 0.0f && std::abs(totalWeight - 1.0f) > 0.0001f) {
        outPosition[0] /= totalWeight;
        outPosition[1] /= totalWeight;
        outPosition[2] /= totalWeight;
    }
}

void SoftwareSkinning::ApplySkinning(
    const SkinningVertex* srcVertices,
    float* dstPositions,
    float* dstNormals,
    uint32_t numVertices,
    const D3DMATRIX* boneMatrices,
    uint32_t numBones
) {
    (void)numBones; // For bounds checking in debug

    for (uint32_t i = 0; i < numVertices; i++) {
        const SkinningVertex& src = srcVertices[i];

        BlendVertex(
            src.position,
            src.blendWeights,
            src.blendIndices,
            4,
            boneMatrices,
            &dstPositions[i * 3]
        );

        // TODO: Transform normals if dstNormals provided
        (void)dstNormals;
    }
}

// =============================================================================
// Morph Target Blender Implementation
// =============================================================================

void MorphTargetBlender::Blend(
    const float* src0, const float* src1,
    float* dst,
    uint32_t numFloats,
    float t
) {
    float oneMinusT = 1.0f - t;
    for (uint32_t i = 0; i < numFloats; i++) {
        dst[i] = src0[i] * oneMinusT + src1[i] * t;
    }
}

void MorphTargetBlender::BlendMultiple(
    const float** sources,
    const float* weights,
    uint32_t numSources,
    float* dst,
    uint32_t numFloats
) {
    // Initialize to zero
    std::memset(dst, 0, numFloats * sizeof(float));

    // Accumulate weighted sources
    for (uint32_t s = 0; s < numSources; s++) {
        float weight = weights[s];
        if (weight == 0.0f) continue;

        const float* src = sources[s];
        for (uint32_t i = 0; i < numFloats; i++) {
            dst[i] += src[i] * weight;
        }
    }
}

} // namespace dx8bgfx
