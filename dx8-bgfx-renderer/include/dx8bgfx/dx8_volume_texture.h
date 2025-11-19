#pragma once

#include "dx8_types.h"
#include "dx8_constants.h"

#include <bgfx/bgfx.h>
#include <vector>

namespace dx8bgfx {

// =============================================================================
// Volume Texture Types
// =============================================================================

// Volume texture description
struct VolumeTextureDesc {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t levels;         // Mipmap levels (0 = full chain)
    D3DFORMAT format;
    D3DPOOL pool;
    bool dynamic;
};

// Locked volume region
struct D3DLOCKED_BOX {
    int32_t RowPitch;
    int32_t SlicePitch;
    void* pBits;
};

// Volume box region
struct D3DBOX {
    uint32_t Left;
    uint32_t Top;
    uint32_t Right;
    uint32_t Bottom;
    uint32_t Front;
    uint32_t Back;
};

// =============================================================================
// Volume Texture Utilities
// =============================================================================

class VolumeTextureUtils {
public:
    // Create empty volume texture
    static bgfx::TextureHandle CreateVolumeTexture(
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        bool hasMips,
        bgfx::TextureFormat::Enum format,
        uint64_t flags = BGFX_TEXTURE_NONE
    );

    // Create volume texture from memory
    static bgfx::TextureHandle CreateVolumeTextureFromMemory(
        const void* data,
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        bgfx::TextureFormat::Enum format,
        bool hasMips = false
    );

    // Update volume texture
    static void UpdateVolumeTexture(
        bgfx::TextureHandle handle,
        uint8_t mip,
        uint16_t x, uint16_t y, uint16_t z,
        uint16_t width, uint16_t height, uint16_t depth,
        const void* data,
        uint32_t pitch = UINT16_MAX
    );

    // Calculate mipmap dimensions
    static void GetMipDimensions(
        uint32_t level,
        uint32_t baseWidth,
        uint32_t baseHeight,
        uint32_t baseDepth,
        uint32_t& mipWidth,
        uint32_t& mipHeight,
        uint32_t& mipDepth
    );

    // Calculate data size for volume texture
    static uint32_t CalculateDataSize(
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        bgfx::TextureFormat::Enum format,
        bool includeMips = false
    );

    // Get bytes per pixel for format
    static uint32_t GetBytesPerPixel(bgfx::TextureFormat::Enum format);
};

// =============================================================================
// Volume Texture Manager
// =============================================================================

class VolumeTextureManager {
public:
    VolumeTextureManager();
    ~VolumeTextureManager();

    // Create volume texture
    bgfx::TextureHandle CreateTexture(const VolumeTextureDesc& desc);

    // Destroy volume texture
    void DestroyTexture(bgfx::TextureHandle handle);

    // Lock region for editing
    bool LockBox(
        bgfx::TextureHandle handle,
        uint32_t level,
        D3DLOCKED_BOX* lockedBox,
        const D3DBOX* box,
        DWORD flags
    );

    // Unlock region
    void UnlockBox(bgfx::TextureHandle handle, uint32_t level);

    // Set volume texture to a stage
    void SetVolumeTexture(uint32_t stage, bgfx::TextureHandle handle);

    // Get current volume texture
    bgfx::TextureHandle GetVolumeTexture(uint32_t stage) const;

    // Check if texture at stage is volume
    bool IsVolumeTexture(uint32_t stage) const;

private:
    struct VolumeTextureEntry {
        bgfx::TextureHandle handle;
        VolumeTextureDesc desc;
        std::vector<uint8_t> lockBuffer;
        bool locked;
        uint32_t lockedLevel;
        D3DBOX lockedBox;
    };

    struct StageBinding {
        bgfx::TextureHandle texture;
        bool isVolume;
    };

    std::vector<VolumeTextureEntry> m_textures;
    StageBinding m_stages[8];
};

// =============================================================================
// Volume Data Generators
// =============================================================================

class VolumeDataGenerator {
public:
    // Generate 3D noise texture
    static std::vector<uint8_t> GeneratePerlinNoise(
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        float scale = 1.0f,
        uint32_t octaves = 4
    );

    // Generate density field for fog/clouds
    static std::vector<uint8_t> GenerateDensityField(
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        float density = 0.5f,
        float falloff = 1.0f
    );

    // Generate gradient texture (for transfer functions)
    static std::vector<uint8_t> GenerateGradient(
        uint32_t size,
        const uint32_t* colors,  // RGBA colors
        const float* positions,  // 0-1 positions
        uint32_t numStops
    );

    // Generate sphere density
    static std::vector<uint8_t> GenerateSphereDensity(
        uint32_t size,
        float radius = 0.5f,
        float falloff = 2.0f
    );

    // Generate box density
    static std::vector<uint8_t> GenerateBoxDensity(
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        float minX, float maxX,
        float minY, float maxY,
        float minZ, float maxZ,
        float falloff = 2.0f
    );
};

// =============================================================================
// Volume Rendering
// =============================================================================

class VolumeRenderer {
public:
    VolumeRenderer();
    ~VolumeRenderer();

    // Initialize renderer
    void Initialize();
    void Shutdown();

    // Set volume texture
    void SetVolume(bgfx::TextureHandle volume);

    // Set transfer function (1D color lookup)
    void SetTransferFunction(bgfx::TextureHandle transfer);

    // Set rendering parameters
    void SetStepSize(float stepSize);
    void SetDensityScale(float scale);
    void SetBrightness(float brightness);

    // Render volume
    void Render(
        const D3DMATRIX& world,
        const D3DMATRIX& view,
        const D3DMATRIX& projection,
        const D3DVECTOR& eyePos
    );

private:
    bgfx::TextureHandle m_volume;
    bgfx::TextureHandle m_transfer;

    float m_stepSize;
    float m_densityScale;
    float m_brightness;

    bgfx::VertexBufferHandle m_cubeVB;
    bgfx::IndexBufferHandle m_cubeIB;

    bool m_initialized;
};

// =============================================================================
// 3D Lookup Table
// =============================================================================

class LUT3D {
public:
    LUT3D();
    ~LUT3D();

    // Create identity LUT
    void CreateIdentity(uint32_t size = 32);

    // Load from .cube file
    bool LoadFromCube(const char* path);

    // Load from raw data
    void LoadFromData(
        const float* data,  // RGB float data
        uint32_t size
    );

    // Get texture handle
    bgfx::TextureHandle GetTexture() const { return m_texture; }

    // Get LUT size
    uint32_t GetSize() const { return m_size; }

    // Apply color grading adjustment
    void SetBrightness(float brightness);
    void SetContrast(float contrast);
    void SetSaturation(float saturation);
    void SetHueShift(float degrees);

    // Rebuild texture from modified data
    void Rebuild();

private:
    bgfx::TextureHandle m_texture;
    std::vector<float> m_data;
    uint32_t m_size;
};

// =============================================================================
// Texture Coordinate Generation for Volume
// =============================================================================

class VolumeTexGen {
public:
    // Generate texture coordinates for object-space volume
    static void GenerateObjectCoords(
        const float* positions,
        const D3DMATRIX& worldInverse,  // Inverse world transform
        const D3DVECTOR& volumeMin,      // Volume AABB min
        const D3DVECTOR& volumeMax,      // Volume AABB max
        float* texCoords,
        uint32_t numVertices
    );

    // Generate animated texture coordinates
    static void GenerateAnimatedCoords(
        const float* positions,
        const D3DMATRIX& world,
        float time,
        float speed,
        float* texCoords,
        uint32_t numVertices
    );

    // Transform UVW coordinates by texture matrix
    static void TransformCoords(
        float* texCoords,
        uint32_t numVertices,
        const D3DMATRIX& textureMatrix
    );
};

} // namespace dx8bgfx
