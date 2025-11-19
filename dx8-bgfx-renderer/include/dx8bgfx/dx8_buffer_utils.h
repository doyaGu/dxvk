#pragma once

#include "dx8_types.h"
#include "dx8_constants.h"

#include <bgfx/bgfx.h>
#include <vector>
#include <cstring>

namespace dx8bgfx {

// =============================================================================
// Vertex Buffer Utilities
// =============================================================================

class VertexBufferUtils {
public:
    // Build bgfx vertex layout from FVF
    static bgfx::VertexLayout BuildLayoutFromFVF(DWORD fvf);

    // Calculate vertex stride from FVF
    static uint32_t CalculateVertexStride(DWORD fvf);

    // Get number of texture coordinates from FVF
    static uint32_t GetTexCoordCount(DWORD fvf);

    // Get texture coordinate dimensions for a specific set
    static uint32_t GetTexCoordDimensions(DWORD fvf, uint32_t index);

    // Check if FVF has specific components
    static bool HasPosition(DWORD fvf);
    static bool HasNormal(DWORD fvf);
    static bool HasDiffuse(DWORD fvf);
    static bool HasSpecular(DWORD fvf);
    static bool HasPointSize(DWORD fvf);

    // Create a dynamic vertex buffer
    static bgfx::DynamicVertexBufferHandle CreateDynamicVertexBuffer(
        uint32_t numVertices,
        const bgfx::VertexLayout& layout,
        uint16_t flags = BGFX_BUFFER_NONE
    );

    // Create a static vertex buffer
    static bgfx::VertexBufferHandle CreateVertexBuffer(
        const void* data,
        uint32_t numVertices,
        const bgfx::VertexLayout& layout,
        uint16_t flags = BGFX_BUFFER_NONE
    );

    // Update dynamic vertex buffer
    static void UpdateDynamicVertexBuffer(
        bgfx::DynamicVertexBufferHandle handle,
        uint32_t startVertex,
        const void* data,
        uint32_t numVertices,
        uint32_t vertexStride
    );

    // Convert D3D8 vertex data to bgfx format (handles color swizzling etc.)
    static void ConvertVertexData(
        const void* srcData,
        void* dstData,
        uint32_t numVertices,
        DWORD fvf
    );
};

// =============================================================================
// Index Buffer Utilities
// =============================================================================

class IndexBufferUtils {
public:
    // Create a static index buffer (16-bit indices)
    static bgfx::IndexBufferHandle CreateIndexBuffer16(
        const uint16_t* indices,
        uint32_t numIndices,
        uint16_t flags = BGFX_BUFFER_NONE
    );

    // Create a static index buffer (32-bit indices)
    static bgfx::IndexBufferHandle CreateIndexBuffer32(
        const uint32_t* indices,
        uint32_t numIndices,
        uint16_t flags = BGFX_BUFFER_NONE
    );

    // Create a dynamic index buffer (16-bit)
    static bgfx::DynamicIndexBufferHandle CreateDynamicIndexBuffer16(
        uint32_t numIndices,
        uint16_t flags = BGFX_BUFFER_NONE
    );

    // Create a dynamic index buffer (32-bit)
    static bgfx::DynamicIndexBufferHandle CreateDynamicIndexBuffer32(
        uint32_t numIndices,
        uint16_t flags = BGFX_BUFFER_NONE
    );

    // Update dynamic index buffer
    static void UpdateDynamicIndexBuffer(
        bgfx::DynamicIndexBufferHandle handle,
        uint32_t startIndex,
        const void* data,
        uint32_t numIndices,
        bool is32Bit
    );

    // Convert 32-bit indices to 16-bit (with optional offset)
    static std::vector<uint16_t> Convert32To16(
        const uint32_t* indices,
        uint32_t numIndices,
        uint32_t baseVertex = 0
    );
};

// =============================================================================
// Transient Buffer Helpers
// =============================================================================

class TransientBufferUtils {
public:
    // Allocate transient vertex buffer
    static bool AllocTransientVertexBuffer(
        bgfx::TransientVertexBuffer* tvb,
        uint32_t numVertices,
        const bgfx::VertexLayout& layout
    );

    // Allocate transient index buffer
    static bool AllocTransientIndexBuffer(
        bgfx::TransientIndexBuffer* tib,
        uint32_t numIndices,
        bool is32Bit = false
    );

    // Check if transient buffers are available
    static bool CheckAvailTransientBuffers(
        uint32_t numVertices,
        const bgfx::VertexLayout& layout,
        uint32_t numIndices,
        bool is32Bit = false
    );
};

// =============================================================================
// Primitive Helpers
// =============================================================================

class PrimitiveUtils {
public:
    // Calculate number of vertices for a primitive type
    static uint32_t CalculateVertexCount(D3DPRIMITIVETYPE type, uint32_t primitiveCount);

    // Calculate number of indices for a primitive type
    static uint32_t CalculateIndexCount(D3DPRIMITIVETYPE type, uint32_t primitiveCount);

    // Generate indices for a triangle list from a triangle strip
    static std::vector<uint16_t> StripToList(uint32_t numStripVertices);

    // Generate indices for a triangle list from a triangle fan
    static std::vector<uint16_t> FanToList(uint32_t numFanVertices);

    // Generate line list indices from a line strip
    static std::vector<uint16_t> LineStripToList(uint32_t numStripVertices);
};

// =============================================================================
// Geometry Generators
// =============================================================================

class GeometryGenerator {
public:
    // Vertex structure for generated geometry
    struct Vertex {
        float position[3];
        float normal[3];
        float texcoord[2];
        uint32_t color;
    };

    // Generate a unit cube
    static void GenerateCube(
        std::vector<Vertex>& outVertices,
        std::vector<uint16_t>& outIndices
    );

    // Generate a sphere
    static void GenerateSphere(
        std::vector<Vertex>& outVertices,
        std::vector<uint16_t>& outIndices,
        uint32_t slices = 16,
        uint32_t stacks = 16
    );

    // Generate a cylinder
    static void GenerateCylinder(
        std::vector<Vertex>& outVertices,
        std::vector<uint16_t>& outIndices,
        uint32_t slices = 16
    );

    // Generate a plane (quad)
    static void GeneratePlane(
        std::vector<Vertex>& outVertices,
        std::vector<uint16_t>& outIndices,
        uint32_t subdivisions = 1
    );

    // Generate a torus
    static void GenerateTorus(
        std::vector<Vertex>& outVertices,
        std::vector<uint16_t>& outIndices,
        float innerRadius = 0.3f,
        float outerRadius = 1.0f,
        uint32_t rings = 16,
        uint32_t sides = 16
    );

    // Get vertex layout for generated geometry
    static bgfx::VertexLayout GetGeneratedVertexLayout();
};

} // namespace dx8bgfx
