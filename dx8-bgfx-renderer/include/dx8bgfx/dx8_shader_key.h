#pragma once

#include "dx8_types.h"
#include "dx8_constants.h"
#include <functional>

namespace dx8bgfx {

// =============================================================================
// Vertex Shader Key
// =============================================================================

struct VertexShaderKeyData {
    union {
        struct {
            // First 32 bits
            uint32_t TexcoordIndices : 24;      // 8 stages * 3 bits
            uint32_t HasPositionT : 1;          // Pre-transformed vertices
            uint32_t HasColor0 : 1;             // Has diffuse color
            uint32_t HasColor1 : 1;             // Has specular color
            uint32_t HasPointSize : 1;          // Has point size
            uint32_t UseLighting : 1;           // Lighting enabled
            uint32_t NormalizeNormals : 1;      // Normalize normals
            uint32_t LocalViewer : 1;           // Local viewer mode
            uint32_t RangeFog : 1;              // Range-based fog

            // Second 32 bits
            uint32_t TexcoordFlags : 24;        // TCI flags for 8 stages
            uint32_t DiffuseSource : 2;         // D3DMCS_*
            uint32_t AmbientSource : 2;         // D3DMCS_*
            uint32_t SpecularSource : 2;        // D3DMCS_*
            uint32_t EmissiveSource : 2;        // D3DMCS_*

            // Third 32 bits
            uint32_t TransformFlags : 24;       // Transform flags for 8 stages
            uint32_t LightCount : 4;            // Number of lights (0-8)
            uint32_t SpecularEnabled : 1;       // Specular highlighting
            uint32_t FogMode : 2;               // Fog mode
            uint32_t HasNormal : 1;             // Has normal

            // Fourth 32 bits
            uint32_t TexcoordDeclMask : 24;     // Which texcoords are declared
            uint32_t HasFog : 1;                // Has fog coordinate
            uint32_t VertexBlendMode : 2;       // 0=None, 1=Normal, 2=Tween
            uint32_t VertexBlendIndexed : 1;    // Use blend indices
            uint32_t VertexBlendCount : 3;      // Number of blend weights
            uint32_t Clipping : 1;              // User clip planes enabled
        } bits;

        uint32_t data[4];
    };

    VertexShaderKeyData() {
        std::memset(data, 0, sizeof(data));
    }

    bool operator==(const VertexShaderKeyData& other) const {
        return std::memcmp(data, other.data, sizeof(data)) == 0;
    }

    bool operator!=(const VertexShaderKeyData& other) const {
        return !(*this == other);
    }
};

struct VertexShaderKey {
    VertexShaderKeyData Data;

    VertexShaderKey() = default;

    uint64_t GetHash() const;

    bool operator==(const VertexShaderKey& other) const {
        return Data == other.Data;
    }

    bool operator!=(const VertexShaderKey& other) const {
        return !(*this == other);
    }
};

// =============================================================================
// Fragment Shader Key - Texture Stage
// =============================================================================

struct TextureStageKey {
    union {
        struct {
            // First 32 bits
            uint32_t ColorOp : 5;               // D3DTOP_*
            uint32_t ColorArg0 : 6;             // D3DTA_*
            uint32_t ColorArg1 : 6;             // D3DTA_*
            uint32_t ColorArg2 : 6;             // D3DTA_*
            uint32_t AlphaOp : 5;               // D3DTOP_*
            uint32_t _pad0 : 4;

            // Second 32 bits
            uint32_t AlphaArg0 : 6;             // D3DTA_*
            uint32_t AlphaArg1 : 6;             // D3DTA_*
            uint32_t AlphaArg2 : 6;             // D3DTA_*
            uint32_t ResultIsTemp : 1;          // Write to temp register
            uint32_t TextureType : 2;           // 0=2D, 1=Cube, 2=3D
            uint32_t HasTexture : 1;            // Texture bound
            uint32_t _pad1 : 10;
        } bits;

        uint32_t data[2];
    };

    TextureStageKey() {
        std::memset(data, 0, sizeof(data));
    }

    bool operator==(const TextureStageKey& other) const {
        return data[0] == other.data[0] && data[1] == other.data[1];
    }
};

// =============================================================================
// Fragment Shader Key
// =============================================================================

struct FragmentShaderKeyData {
    TextureStageKey Stages[MaxTextureStages];

    union {
        struct {
            uint32_t AlphaTestEnabled : 1;
            uint32_t AlphaTestFunc : 3;         // D3DCMP_*
            uint32_t FogEnabled : 1;
            uint32_t FogMode : 2;               // D3DFOG_*
            uint32_t SpecularEnabled : 1;
            uint32_t _pad : 24;
        } bits;

        uint32_t flags;
    };

    FragmentShaderKeyData() {
        std::memset(Stages, 0, sizeof(Stages));
        flags = 0;
    }

    bool operator==(const FragmentShaderKeyData& other) const {
        if (flags != other.flags) return false;
        for (int i = 0; i < MaxTextureStages; i++) {
            if (!(Stages[i] == other.Stages[i])) return false;
        }
        return true;
    }

    bool operator!=(const FragmentShaderKeyData& other) const {
        return !(*this == other);
    }
};

struct FragmentShaderKey {
    FragmentShaderKeyData Data;

    FragmentShaderKey() = default;

    uint64_t GetHash() const;

    bool operator==(const FragmentShaderKey& other) const {
        return Data == other.Data;
    }

    bool operator!=(const FragmentShaderKey& other) const {
        return !(*this == other);
    }
};

// =============================================================================
// Combined Shader Key
// =============================================================================

struct ShaderKey {
    VertexShaderKey VS;
    FragmentShaderKey FS;

    uint64_t GetHash() const {
        return VS.GetHash() ^ (FS.GetHash() << 1);
    }

    bool operator==(const ShaderKey& other) const {
        return VS == other.VS && FS == other.FS;
    }
};

// =============================================================================
// Hash Functions
// =============================================================================

struct VertexShaderKeyHash {
    size_t operator()(const VertexShaderKey& key) const {
        return static_cast<size_t>(key.GetHash());
    }
};

struct FragmentShaderKeyHash {
    size_t operator()(const FragmentShaderKey& key) const {
        return static_cast<size_t>(key.GetHash());
    }
};

struct ShaderKeyHash {
    size_t operator()(const ShaderKey& key) const {
        return static_cast<size_t>(key.GetHash());
    }
};

} // namespace dx8bgfx
