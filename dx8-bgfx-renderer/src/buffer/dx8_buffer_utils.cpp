#include "dx8bgfx/dx8_buffer_utils.h"
#include <cmath>

namespace dx8bgfx {

// =============================================================================
// Vertex Buffer Utils Implementation
// =============================================================================

bgfx::VertexLayout VertexBufferUtils::BuildLayoutFromFVF(DWORD fvf) {
    bgfx::VertexLayout layout;
    layout.begin();

    // Position
    DWORD posType = fvf & D3DFVF_POSITION_MASK;
    switch (posType) {
        case D3DFVF_XYZ:
            layout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
            break;
        case D3DFVF_XYZRHW:
            layout.add(bgfx::Attrib::Position, 4, bgfx::AttribType::Float);
            break;
        case D3DFVF_XYZB1:
            layout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
            layout.add(bgfx::Attrib::Weight, 1, bgfx::AttribType::Float);
            break;
        case D3DFVF_XYZB2:
            layout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
            layout.add(bgfx::Attrib::Weight, 2, bgfx::AttribType::Float);
            break;
        case D3DFVF_XYZB3:
            layout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
            layout.add(bgfx::Attrib::Weight, 3, bgfx::AttribType::Float);
            break;
        case D3DFVF_XYZB4:
            layout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
            layout.add(bgfx::Attrib::Weight, 4, bgfx::AttribType::Float);
            break;
        case D3DFVF_XYZB5:
            layout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
            layout.add(bgfx::Attrib::Weight, 4, bgfx::AttribType::Float);
            layout.add(bgfx::Attrib::Indices, 4, bgfx::AttribType::Uint8, true);
            break;
        default:
            break;
    }

    // Normal
    if (fvf & D3DFVF_NORMAL) {
        layout.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float);
    }

    // Point size
    if (fvf & D3DFVF_PSIZE) {
        layout.add(bgfx::Attrib::PointSize, 1, bgfx::AttribType::Float);
    }

    // Diffuse color
    if (fvf & D3DFVF_DIFFUSE) {
        layout.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true);
    }

    // Specular color
    if (fvf & D3DFVF_SPECULAR) {
        layout.add(bgfx::Attrib::Color1, 4, bgfx::AttribType::Uint8, true);
    }

    // Texture coordinates
    DWORD texCount = GetTexCoordCount(fvf);
    for (DWORD i = 0; i < texCount; i++) {
        uint32_t dims = GetTexCoordDimensions(fvf, i);
        bgfx::Attrib::Enum attrib = static_cast<bgfx::Attrib::Enum>(
            bgfx::Attrib::TexCoord0 + i
        );
        layout.add(attrib, dims, bgfx::AttribType::Float);
    }

    layout.end();
    return layout;
}

uint32_t VertexBufferUtils::CalculateVertexStride(DWORD fvf) {
    uint32_t stride = 0;

    // Position
    DWORD posType = fvf & D3DFVF_POSITION_MASK;
    switch (posType) {
        case D3DFVF_XYZ:    stride += 12; break;
        case D3DFVF_XYZRHW: stride += 16; break;
        case D3DFVF_XYZB1:  stride += 16; break;
        case D3DFVF_XYZB2:  stride += 20; break;
        case D3DFVF_XYZB3:  stride += 24; break;
        case D3DFVF_XYZB4:  stride += 28; break;
        case D3DFVF_XYZB5:  stride += 32; break;
        default: break;
    }

    if (fvf & D3DFVF_NORMAL)   stride += 12;
    if (fvf & D3DFVF_PSIZE)    stride += 4;
    if (fvf & D3DFVF_DIFFUSE)  stride += 4;
    if (fvf & D3DFVF_SPECULAR) stride += 4;

    // Texture coordinates
    DWORD texCount = GetTexCoordCount(fvf);
    for (DWORD i = 0; i < texCount; i++) {
        stride += GetTexCoordDimensions(fvf, i) * 4;
    }

    return stride;
}

uint32_t VertexBufferUtils::GetTexCoordCount(DWORD fvf) {
    return (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
}

uint32_t VertexBufferUtils::GetTexCoordDimensions(DWORD fvf, uint32_t index) {
    // Extract texture coord format for this index
    DWORD texFormat = (fvf >> (16 + index * 2)) & 0x3;
    switch (texFormat) {
        case D3DFVF_TEXTUREFORMAT1: return 1;
        case D3DFVF_TEXTUREFORMAT2: return 2;
        case D3DFVF_TEXTUREFORMAT3: return 3;
        case D3DFVF_TEXTUREFORMAT4: return 4;
        default: return 2; // Default to UV
    }
}

bool VertexBufferUtils::HasPosition(DWORD fvf) {
    return (fvf & D3DFVF_POSITION_MASK) != 0;
}

bool VertexBufferUtils::HasNormal(DWORD fvf) {
    return (fvf & D3DFVF_NORMAL) != 0;
}

bool VertexBufferUtils::HasDiffuse(DWORD fvf) {
    return (fvf & D3DFVF_DIFFUSE) != 0;
}

bool VertexBufferUtils::HasSpecular(DWORD fvf) {
    return (fvf & D3DFVF_SPECULAR) != 0;
}

bool VertexBufferUtils::HasPointSize(DWORD fvf) {
    return (fvf & D3DFVF_PSIZE) != 0;
}

bgfx::DynamicVertexBufferHandle VertexBufferUtils::CreateDynamicVertexBuffer(
    uint32_t numVertices,
    const bgfx::VertexLayout& layout,
    uint16_t flags
) {
    return bgfx::createDynamicVertexBuffer(numVertices, layout, flags);
}

bgfx::VertexBufferHandle VertexBufferUtils::CreateVertexBuffer(
    const void* data,
    uint32_t numVertices,
    const bgfx::VertexLayout& layout,
    uint16_t flags
) {
    const bgfx::Memory* mem = bgfx::copy(data, numVertices * layout.getStride());
    return bgfx::createVertexBuffer(mem, layout, flags);
}

void VertexBufferUtils::UpdateDynamicVertexBuffer(
    bgfx::DynamicVertexBufferHandle handle,
    uint32_t startVertex,
    const void* data,
    uint32_t numVertices,
    uint32_t vertexStride
) {
    const bgfx::Memory* mem = bgfx::copy(data, numVertices * vertexStride);
    bgfx::update(handle, startVertex, mem);
}

void VertexBufferUtils::ConvertVertexData(
    const void* srcData,
    void* dstData,
    uint32_t numVertices,
    DWORD fvf
) {
    // D3D8 uses BGRA color format, bgfx expects RGBA
    // This function swizzles colors if needed

    uint32_t stride = CalculateVertexStride(fvf);
    const uint8_t* src = static_cast<const uint8_t*>(srcData);
    uint8_t* dst = static_cast<uint8_t*>(dstData);

    // Calculate color offsets
    uint32_t diffuseOffset = 0;
    uint32_t specularOffset = 0;
    uint32_t offset = 0;

    // Skip position
    DWORD posType = fvf & D3DFVF_POSITION_MASK;
    switch (posType) {
        case D3DFVF_XYZ:    offset += 12; break;
        case D3DFVF_XYZRHW: offset += 16; break;
        case D3DFVF_XYZB1:  offset += 16; break;
        case D3DFVF_XYZB2:  offset += 20; break;
        case D3DFVF_XYZB3:  offset += 24; break;
        case D3DFVF_XYZB4:  offset += 28; break;
        case D3DFVF_XYZB5:  offset += 32; break;
        default: break;
    }

    if (fvf & D3DFVF_NORMAL) offset += 12;
    if (fvf & D3DFVF_PSIZE) offset += 4;

    if (fvf & D3DFVF_DIFFUSE) {
        diffuseOffset = offset;
        offset += 4;
    }
    if (fvf & D3DFVF_SPECULAR) {
        specularOffset = offset;
        offset += 4;
    }

    // Copy and swizzle
    for (uint32_t i = 0; i < numVertices; i++) {
        std::memcpy(dst, src, stride);

        // Swizzle diffuse color (BGRA -> RGBA)
        if (fvf & D3DFVF_DIFFUSE) {
            uint8_t* color = dst + diffuseOffset;
            uint8_t tmp = color[0];
            color[0] = color[2];
            color[2] = tmp;
        }

        // Swizzle specular color
        if (fvf & D3DFVF_SPECULAR) {
            uint8_t* color = dst + specularOffset;
            uint8_t tmp = color[0];
            color[0] = color[2];
            color[2] = tmp;
        }

        src += stride;
        dst += stride;
    }
}

// =============================================================================
// Index Buffer Utils Implementation
// =============================================================================

bgfx::IndexBufferHandle IndexBufferUtils::CreateIndexBuffer16(
    const uint16_t* indices,
    uint32_t numIndices,
    uint16_t flags
) {
    const bgfx::Memory* mem = bgfx::copy(indices, numIndices * sizeof(uint16_t));
    return bgfx::createIndexBuffer(mem, flags);
}

bgfx::IndexBufferHandle IndexBufferUtils::CreateIndexBuffer32(
    const uint32_t* indices,
    uint32_t numIndices,
    uint16_t flags
) {
    const bgfx::Memory* mem = bgfx::copy(indices, numIndices * sizeof(uint32_t));
    return bgfx::createIndexBuffer(mem, flags | BGFX_BUFFER_INDEX32);
}

bgfx::DynamicIndexBufferHandle IndexBufferUtils::CreateDynamicIndexBuffer16(
    uint32_t numIndices,
    uint16_t flags
) {
    return bgfx::createDynamicIndexBuffer(numIndices, flags);
}

bgfx::DynamicIndexBufferHandle IndexBufferUtils::CreateDynamicIndexBuffer32(
    uint32_t numIndices,
    uint16_t flags
) {
    return bgfx::createDynamicIndexBuffer(numIndices, flags | BGFX_BUFFER_INDEX32);
}

void IndexBufferUtils::UpdateDynamicIndexBuffer(
    bgfx::DynamicIndexBufferHandle handle,
    uint32_t startIndex,
    const void* data,
    uint32_t numIndices,
    bool is32Bit
) {
    uint32_t indexSize = is32Bit ? 4 : 2;
    const bgfx::Memory* mem = bgfx::copy(data, numIndices * indexSize);
    bgfx::update(handle, startIndex, mem);
}

std::vector<uint16_t> IndexBufferUtils::Convert32To16(
    const uint32_t* indices,
    uint32_t numIndices,
    uint32_t baseVertex
) {
    std::vector<uint16_t> result(numIndices);
    for (uint32_t i = 0; i < numIndices; i++) {
        result[i] = static_cast<uint16_t>(indices[i] - baseVertex);
    }
    return result;
}

// =============================================================================
// Transient Buffer Utils Implementation
// =============================================================================

bool TransientBufferUtils::AllocTransientVertexBuffer(
    bgfx::TransientVertexBuffer* tvb,
    uint32_t numVertices,
    const bgfx::VertexLayout& layout
) {
    if (bgfx::getAvailTransientVertexBuffer(numVertices, layout) < numVertices) {
        return false;
    }
    bgfx::allocTransientVertexBuffer(tvb, numVertices, layout);
    return true;
}

bool TransientBufferUtils::AllocTransientIndexBuffer(
    bgfx::TransientIndexBuffer* tib,
    uint32_t numIndices,
    bool is32Bit
) {
    if (bgfx::getAvailTransientIndexBuffer(numIndices, is32Bit) < numIndices) {
        return false;
    }
    bgfx::allocTransientIndexBuffer(tib, numIndices, is32Bit);
    return true;
}

bool TransientBufferUtils::CheckAvailTransientBuffers(
    uint32_t numVertices,
    const bgfx::VertexLayout& layout,
    uint32_t numIndices,
    bool is32Bit
) {
    return bgfx::getAvailTransientVertexBuffer(numVertices, layout) >= numVertices &&
           bgfx::getAvailTransientIndexBuffer(numIndices, is32Bit) >= numIndices;
}

// =============================================================================
// Primitive Utils Implementation
// =============================================================================

uint32_t PrimitiveUtils::CalculateVertexCount(D3DPRIMITIVETYPE type, uint32_t primitiveCount) {
    switch (type) {
        case D3DPT_POINTLIST:     return primitiveCount;
        case D3DPT_LINELIST:      return primitiveCount * 2;
        case D3DPT_LINESTRIP:     return primitiveCount + 1;
        case D3DPT_TRIANGLELIST:  return primitiveCount * 3;
        case D3DPT_TRIANGLESTRIP: return primitiveCount + 2;
        case D3DPT_TRIANGLEFAN:   return primitiveCount + 2;
        default:                  return 0;
    }
}

uint32_t PrimitiveUtils::CalculateIndexCount(D3DPRIMITIVETYPE type, uint32_t primitiveCount) {
    switch (type) {
        case D3DPT_POINTLIST:     return primitiveCount;
        case D3DPT_LINELIST:      return primitiveCount * 2;
        case D3DPT_LINESTRIP:     return primitiveCount + 1;
        case D3DPT_TRIANGLELIST:  return primitiveCount * 3;
        case D3DPT_TRIANGLESTRIP: return primitiveCount * 3; // Converted to list
        case D3DPT_TRIANGLEFAN:   return primitiveCount * 3; // Converted to list
        default:                  return 0;
    }
}

std::vector<uint16_t> PrimitiveUtils::StripToList(uint32_t numStripVertices) {
    if (numStripVertices < 3) return {};

    uint32_t numTris = numStripVertices - 2;
    std::vector<uint16_t> indices;
    indices.reserve(numTris * 3);

    for (uint32_t i = 0; i < numTris; i++) {
        if (i % 2 == 0) {
            indices.push_back(static_cast<uint16_t>(i));
            indices.push_back(static_cast<uint16_t>(i + 1));
            indices.push_back(static_cast<uint16_t>(i + 2));
        } else {
            indices.push_back(static_cast<uint16_t>(i));
            indices.push_back(static_cast<uint16_t>(i + 2));
            indices.push_back(static_cast<uint16_t>(i + 1));
        }
    }

    return indices;
}

std::vector<uint16_t> PrimitiveUtils::FanToList(uint32_t numFanVertices) {
    if (numFanVertices < 3) return {};

    uint32_t numTris = numFanVertices - 2;
    std::vector<uint16_t> indices;
    indices.reserve(numTris * 3);

    for (uint32_t i = 0; i < numTris; i++) {
        indices.push_back(0);
        indices.push_back(static_cast<uint16_t>(i + 1));
        indices.push_back(static_cast<uint16_t>(i + 2));
    }

    return indices;
}

std::vector<uint16_t> PrimitiveUtils::LineStripToList(uint32_t numStripVertices) {
    if (numStripVertices < 2) return {};

    uint32_t numLines = numStripVertices - 1;
    std::vector<uint16_t> indices;
    indices.reserve(numLines * 2);

    for (uint32_t i = 0; i < numLines; i++) {
        indices.push_back(static_cast<uint16_t>(i));
        indices.push_back(static_cast<uint16_t>(i + 1));
    }

    return indices;
}

// =============================================================================
// Geometry Generator Implementation
// =============================================================================

bgfx::VertexLayout GeometryGenerator::GetGeneratedVertexLayout() {
    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();
    return layout;
}

void GeometryGenerator::GenerateCube(
    std::vector<Vertex>& outVertices,
    std::vector<uint16_t>& outIndices
) {
    outVertices.clear();
    outIndices.clear();

    // 24 vertices (4 per face for proper normals)
    Vertex vertices[24] = {
        // Front face
        {{-1, -1,  1}, { 0,  0,  1}, {0, 1}, 0xFFFFFFFF},
        {{ 1, -1,  1}, { 0,  0,  1}, {1, 1}, 0xFFFFFFFF},
        {{ 1,  1,  1}, { 0,  0,  1}, {1, 0}, 0xFFFFFFFF},
        {{-1,  1,  1}, { 0,  0,  1}, {0, 0}, 0xFFFFFFFF},
        // Back face
        {{ 1, -1, -1}, { 0,  0, -1}, {0, 1}, 0xFFFFFFFF},
        {{-1, -1, -1}, { 0,  0, -1}, {1, 1}, 0xFFFFFFFF},
        {{-1,  1, -1}, { 0,  0, -1}, {1, 0}, 0xFFFFFFFF},
        {{ 1,  1, -1}, { 0,  0, -1}, {0, 0}, 0xFFFFFFFF},
        // Top face
        {{-1,  1,  1}, { 0,  1,  0}, {0, 1}, 0xFFFFFFFF},
        {{ 1,  1,  1}, { 0,  1,  0}, {1, 1}, 0xFFFFFFFF},
        {{ 1,  1, -1}, { 0,  1,  0}, {1, 0}, 0xFFFFFFFF},
        {{-1,  1, -1}, { 0,  1,  0}, {0, 0}, 0xFFFFFFFF},
        // Bottom face
        {{-1, -1, -1}, { 0, -1,  0}, {0, 1}, 0xFFFFFFFF},
        {{ 1, -1, -1}, { 0, -1,  0}, {1, 1}, 0xFFFFFFFF},
        {{ 1, -1,  1}, { 0, -1,  0}, {1, 0}, 0xFFFFFFFF},
        {{-1, -1,  1}, { 0, -1,  0}, {0, 0}, 0xFFFFFFFF},
        // Right face
        {{ 1, -1,  1}, { 1,  0,  0}, {0, 1}, 0xFFFFFFFF},
        {{ 1, -1, -1}, { 1,  0,  0}, {1, 1}, 0xFFFFFFFF},
        {{ 1,  1, -1}, { 1,  0,  0}, {1, 0}, 0xFFFFFFFF},
        {{ 1,  1,  1}, { 1,  0,  0}, {0, 0}, 0xFFFFFFFF},
        // Left face
        {{-1, -1, -1}, {-1,  0,  0}, {0, 1}, 0xFFFFFFFF},
        {{-1, -1,  1}, {-1,  0,  0}, {1, 1}, 0xFFFFFFFF},
        {{-1,  1,  1}, {-1,  0,  0}, {1, 0}, 0xFFFFFFFF},
        {{-1,  1, -1}, {-1,  0,  0}, {0, 0}, 0xFFFFFFFF},
    };

    outVertices.assign(vertices, vertices + 24);

    uint16_t indices[36] = {
        0, 1, 2, 0, 2, 3,       // Front
        4, 5, 6, 4, 6, 7,       // Back
        8, 9, 10, 8, 10, 11,    // Top
        12, 13, 14, 12, 14, 15, // Bottom
        16, 17, 18, 16, 18, 19, // Right
        20, 21, 22, 20, 22, 23  // Left
    };

    outIndices.assign(indices, indices + 36);
}

void GeometryGenerator::GenerateSphere(
    std::vector<Vertex>& outVertices,
    std::vector<uint16_t>& outIndices,
    uint32_t slices,
    uint32_t stacks
) {
    outVertices.clear();
    outIndices.clear();

    const float pi = 3.14159265358979323846f;

    // Generate vertices
    for (uint32_t i = 0; i <= stacks; i++) {
        float phi = pi * float(i) / float(stacks);
        float y = std::cos(phi);
        float r = std::sin(phi);

        for (uint32_t j = 0; j <= slices; j++) {
            float theta = 2.0f * pi * float(j) / float(slices);
            float x = r * std::cos(theta);
            float z = r * std::sin(theta);

            Vertex v;
            v.position[0] = x;
            v.position[1] = y;
            v.position[2] = z;
            v.normal[0] = x;
            v.normal[1] = y;
            v.normal[2] = z;
            v.texcoord[0] = float(j) / float(slices);
            v.texcoord[1] = float(i) / float(stacks);
            v.color = 0xFFFFFFFF;

            outVertices.push_back(v);
        }
    }

    // Generate indices
    for (uint32_t i = 0; i < stacks; i++) {
        for (uint32_t j = 0; j < slices; j++) {
            uint16_t first = static_cast<uint16_t>(i * (slices + 1) + j);
            uint16_t second = static_cast<uint16_t>(first + slices + 1);

            outIndices.push_back(first);
            outIndices.push_back(second);
            outIndices.push_back(static_cast<uint16_t>(first + 1));

            outIndices.push_back(second);
            outIndices.push_back(static_cast<uint16_t>(second + 1));
            outIndices.push_back(static_cast<uint16_t>(first + 1));
        }
    }
}

void GeometryGenerator::GeneratePlane(
    std::vector<Vertex>& outVertices,
    std::vector<uint16_t>& outIndices,
    uint32_t subdivisions
) {
    outVertices.clear();
    outIndices.clear();

    uint32_t vertsPerSide = subdivisions + 1;
    float step = 2.0f / float(subdivisions);

    // Generate vertices
    for (uint32_t y = 0; y < vertsPerSide; y++) {
        for (uint32_t x = 0; x < vertsPerSide; x++) {
            Vertex v;
            v.position[0] = -1.0f + float(x) * step;
            v.position[1] = 0.0f;
            v.position[2] = -1.0f + float(y) * step;
            v.normal[0] = 0.0f;
            v.normal[1] = 1.0f;
            v.normal[2] = 0.0f;
            v.texcoord[0] = float(x) / float(subdivisions);
            v.texcoord[1] = float(y) / float(subdivisions);
            v.color = 0xFFFFFFFF;

            outVertices.push_back(v);
        }
    }

    // Generate indices
    for (uint32_t y = 0; y < subdivisions; y++) {
        for (uint32_t x = 0; x < subdivisions; x++) {
            uint16_t topLeft = static_cast<uint16_t>(y * vertsPerSide + x);
            uint16_t topRight = static_cast<uint16_t>(topLeft + 1);
            uint16_t bottomLeft = static_cast<uint16_t>((y + 1) * vertsPerSide + x);
            uint16_t bottomRight = static_cast<uint16_t>(bottomLeft + 1);

            outIndices.push_back(topLeft);
            outIndices.push_back(bottomLeft);
            outIndices.push_back(topRight);

            outIndices.push_back(topRight);
            outIndices.push_back(bottomLeft);
            outIndices.push_back(bottomRight);
        }
    }
}

void GeometryGenerator::GenerateCylinder(
    std::vector<Vertex>& outVertices,
    std::vector<uint16_t>& outIndices,
    uint32_t slices
) {
    outVertices.clear();
    outIndices.clear();

    const float pi = 3.14159265358979323846f;

    // Side vertices
    for (uint32_t i = 0; i <= slices; i++) {
        float theta = 2.0f * pi * float(i) / float(slices);
        float x = std::cos(theta);
        float z = std::sin(theta);

        // Bottom
        Vertex vb;
        vb.position[0] = x; vb.position[1] = -1.0f; vb.position[2] = z;
        vb.normal[0] = x; vb.normal[1] = 0.0f; vb.normal[2] = z;
        vb.texcoord[0] = float(i) / float(slices); vb.texcoord[1] = 1.0f;
        vb.color = 0xFFFFFFFF;
        outVertices.push_back(vb);

        // Top
        Vertex vt;
        vt.position[0] = x; vt.position[1] = 1.0f; vt.position[2] = z;
        vt.normal[0] = x; vt.normal[1] = 0.0f; vt.normal[2] = z;
        vt.texcoord[0] = float(i) / float(slices); vt.texcoord[1] = 0.0f;
        vt.color = 0xFFFFFFFF;
        outVertices.push_back(vt);
    }

    // Side indices
    for (uint32_t i = 0; i < slices; i++) {
        uint16_t bl = static_cast<uint16_t>(i * 2);
        uint16_t tl = static_cast<uint16_t>(bl + 1);
        uint16_t br = static_cast<uint16_t>((i + 1) * 2);
        uint16_t tr = static_cast<uint16_t>(br + 1);

        outIndices.push_back(bl);
        outIndices.push_back(br);
        outIndices.push_back(tl);

        outIndices.push_back(tl);
        outIndices.push_back(br);
        outIndices.push_back(tr);
    }

    // Top cap center
    uint16_t topCenter = static_cast<uint16_t>(outVertices.size());
    Vertex tc;
    tc.position[0] = 0.0f; tc.position[1] = 1.0f; tc.position[2] = 0.0f;
    tc.normal[0] = 0.0f; tc.normal[1] = 1.0f; tc.normal[2] = 0.0f;
    tc.texcoord[0] = 0.5f; tc.texcoord[1] = 0.5f;
    tc.color = 0xFFFFFFFF;
    outVertices.push_back(tc);

    // Bottom cap center
    uint16_t bottomCenter = static_cast<uint16_t>(outVertices.size());
    Vertex bc;
    bc.position[0] = 0.0f; bc.position[1] = -1.0f; bc.position[2] = 0.0f;
    bc.normal[0] = 0.0f; bc.normal[1] = -1.0f; bc.normal[2] = 0.0f;
    bc.texcoord[0] = 0.5f; bc.texcoord[1] = 0.5f;
    bc.color = 0xFFFFFFFF;
    outVertices.push_back(bc);

    // Cap indices
    for (uint32_t i = 0; i < slices; i++) {
        // Top cap
        outIndices.push_back(topCenter);
        outIndices.push_back(static_cast<uint16_t>(i * 2 + 1));
        outIndices.push_back(static_cast<uint16_t>((i + 1) * 2 + 1));

        // Bottom cap
        outIndices.push_back(bottomCenter);
        outIndices.push_back(static_cast<uint16_t>((i + 1) * 2));
        outIndices.push_back(static_cast<uint16_t>(i * 2));
    }
}

void GeometryGenerator::GenerateTorus(
    std::vector<Vertex>& outVertices,
    std::vector<uint16_t>& outIndices,
    float innerRadius,
    float outerRadius,
    uint32_t rings,
    uint32_t sides
) {
    outVertices.clear();
    outIndices.clear();

    const float pi = 3.14159265358979323846f;
    float ringRadius = (outerRadius - innerRadius) * 0.5f;
    float centerRadius = innerRadius + ringRadius;

    // Generate vertices
    for (uint32_t i = 0; i <= rings; i++) {
        float u = float(i) / float(rings);
        float theta = u * 2.0f * pi;
        float cosTheta = std::cos(theta);
        float sinTheta = std::sin(theta);

        for (uint32_t j = 0; j <= sides; j++) {
            float v = float(j) / float(sides);
            float phi = v * 2.0f * pi;
            float cosPhi = std::cos(phi);
            float sinPhi = std::sin(phi);

            float x = (centerRadius + ringRadius * cosPhi) * cosTheta;
            float y = ringRadius * sinPhi;
            float z = (centerRadius + ringRadius * cosPhi) * sinTheta;

            Vertex vert;
            vert.position[0] = x;
            vert.position[1] = y;
            vert.position[2] = z;
            vert.normal[0] = cosPhi * cosTheta;
            vert.normal[1] = sinPhi;
            vert.normal[2] = cosPhi * sinTheta;
            vert.texcoord[0] = u;
            vert.texcoord[1] = v;
            vert.color = 0xFFFFFFFF;

            outVertices.push_back(vert);
        }
    }

    // Generate indices
    for (uint32_t i = 0; i < rings; i++) {
        for (uint32_t j = 0; j < sides; j++) {
            uint16_t first = static_cast<uint16_t>(i * (sides + 1) + j);
            uint16_t second = static_cast<uint16_t>((i + 1) * (sides + 1) + j);

            outIndices.push_back(first);
            outIndices.push_back(second);
            outIndices.push_back(static_cast<uint16_t>(first + 1));

            outIndices.push_back(second);
            outIndices.push_back(static_cast<uint16_t>(second + 1));
            outIndices.push_back(static_cast<uint16_t>(first + 1));
        }
    }
}

} // namespace dx8bgfx
