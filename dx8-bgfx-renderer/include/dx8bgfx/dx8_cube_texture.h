#pragma once

#include "dx8_types.h"
#include "dx8_constants.h"

#include <bgfx/bgfx.h>
#include <vector>
#include <string>

namespace dx8bgfx {

// =============================================================================
// Cube Texture Types
// =============================================================================

// D3D8 cube texture face indices
enum D3DCUBEMAP_FACES {
    D3DCUBEMAP_FACE_POSITIVE_X = 0,
    D3DCUBEMAP_FACE_NEGATIVE_X = 1,
    D3DCUBEMAP_FACE_POSITIVE_Y = 2,
    D3DCUBEMAP_FACE_NEGATIVE_Y = 3,
    D3DCUBEMAP_FACE_POSITIVE_Z = 4,
    D3DCUBEMAP_FACE_NEGATIVE_Z = 5,
    D3DCUBEMAP_FACE_FORCE_DWORD = 0x7FFFFFFF
};

// Cube texture description
struct CubeTextureDesc {
    uint32_t size;           // Width/height of each face
    uint32_t levels;         // Mipmap levels (0 = full chain)
    D3DFORMAT format;        // Texture format
    D3DPOOL pool;            // Memory pool
    bool renderTarget;       // Can be used as render target
    bool dynamic;            // Dynamic texture
};

// =============================================================================
// Cube Texture Utilities
// =============================================================================

class CubeTextureUtils {
public:
    // Create empty cube texture
    static bgfx::TextureHandle CreateCubeTexture(
        uint32_t size,
        bool hasMips,
        uint16_t numLayers,
        bgfx::TextureFormat::Enum format,
        uint64_t flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE
    );

    // Create cube texture from 6 face images
    static bgfx::TextureHandle CreateCubeTextureFromFaces(
        const void* posX, const void* negX,
        const void* posY, const void* negY,
        const void* posZ, const void* negZ,
        uint32_t size,
        bgfx::TextureFormat::Enum format
    );

    // Create cube texture from single cross/strip image
    static bgfx::TextureHandle CreateCubeTextureFromCross(
        const void* data,
        uint32_t width, uint32_t height,
        bgfx::TextureFormat::Enum format
    );

    // Create cube texture from memory (6 faces contiguous)
    static bgfx::TextureHandle CreateCubeTextureFromMemory(
        const void* data,
        uint32_t faceSize,
        uint32_t size,
        bgfx::TextureFormat::Enum format,
        bool hasMips = false
    );

    // Update a single face of a cube texture
    static void UpdateCubeTextureFace(
        bgfx::TextureHandle handle,
        uint8_t face,
        uint8_t mip,
        uint16_t x, uint16_t y,
        uint16_t width, uint16_t height,
        const void* data,
        uint32_t pitch = UINT16_MAX
    );

    // Generate mipmaps for cube texture
    static void GenerateCubeTextureMips(
        bgfx::TextureHandle handle,
        uint32_t size,
        bgfx::TextureFormat::Enum format
    );

    // Convert face index between D3D8 and bgfx conventions
    static uint8_t D3DFaceToBgfx(D3DCUBEMAP_FACES face);
    static D3DCUBEMAP_FACES BgfxFaceToD3D(uint8_t face);

    // Get face direction vectors (for cubemap sampling)
    static void GetFaceDirections(
        D3DCUBEMAP_FACES face,
        float& dirX, float& dirY, float& dirZ,
        float& upX, float& upY, float& upZ
    );
};

// =============================================================================
// Cube Texture Manager
// =============================================================================

class CubeTextureManager {
public:
    CubeTextureManager();
    ~CubeTextureManager();

    // Create cube texture
    bgfx::TextureHandle CreateTexture(const CubeTextureDesc& desc);

    // Destroy cube texture
    void DestroyTexture(bgfx::TextureHandle handle);

    // Lock/unlock faces for editing
    void* LockFace(
        bgfx::TextureHandle handle,
        D3DCUBEMAP_FACES face,
        uint32_t level,
        uint32_t& pitch
    );

    void UnlockFace(
        bgfx::TextureHandle handle,
        D3DCUBEMAP_FACES face,
        uint32_t level
    );

    // Set cube texture to a stage
    void SetCubeTexture(uint32_t stage, bgfx::TextureHandle handle);

    // Get current cube texture
    bgfx::TextureHandle GetCubeTexture(uint32_t stage) const;

    // Check if texture at stage is cube map
    bool IsCubeTexture(uint32_t stage) const;

private:
    struct CubeTextureEntry {
        bgfx::TextureHandle handle;
        CubeTextureDesc desc;
        std::vector<uint8_t> lockBuffer;
        bool locked;
        D3DCUBEMAP_FACES lockedFace;
        uint32_t lockedLevel;
    };

    struct StageBinding {
        bgfx::TextureHandle texture;
        bool isCube;
    };

    std::vector<CubeTextureEntry> m_textures;
    StageBinding m_stages[8];
};

// =============================================================================
// Environment Map Generator
// =============================================================================

class EnvironmentMapGenerator {
public:
    EnvironmentMapGenerator();
    ~EnvironmentMapGenerator();

    // Initialize generator
    void Initialize(uint32_t resolution = 256);
    void Shutdown();

    // Generate environment map by rendering scene from a point
    // Callback is invoked for each face with view/projection matrices
    typedef void (*RenderFaceCallback)(
        D3DCUBEMAP_FACES face,
        const D3DMATRIX& view,
        const D3DMATRIX& projection,
        void* userData
    );

    bgfx::TextureHandle GenerateEnvironmentMap(
        float posX, float posY, float posZ,
        RenderFaceCallback callback,
        void* userData,
        bgfx::TextureFormat::Enum format = bgfx::TextureFormat::RGBA8
    );

    // Get view matrix for a cube face
    static D3DMATRIX GetFaceViewMatrix(
        D3DCUBEMAP_FACES face,
        float posX, float posY, float posZ
    );

    // Get projection matrix for cube face (90 degree FOV)
    static D3DMATRIX GetFaceProjectionMatrix(float nearZ, float farZ);

private:
    uint32_t m_resolution;
    bgfx::FrameBufferHandle m_frameBuffer;
    bgfx::TextureHandle m_depthBuffer;
    bool m_initialized;
};

// =============================================================================
// Spherical Harmonics
// =============================================================================

// For irradiance environment mapping
struct SHCoefficients {
    float coefficients[9][3]; // 9 coefficients for RGB
};

class SphericalHarmonics {
public:
    // Project cube map to spherical harmonics
    static SHCoefficients ProjectCubeMap(
        bgfx::TextureHandle cubeMap,
        uint32_t size
    );

    // Evaluate SH at direction
    static void Evaluate(
        const SHCoefficients& sh,
        float dirX, float dirY, float dirZ,
        float& r, float& g, float& b
    );

    // Create irradiance map from SH coefficients
    static bgfx::TextureHandle CreateIrradianceMap(
        const SHCoefficients& sh,
        uint32_t size
    );
};

// =============================================================================
// Texture Coordinate Generation for Cube Maps
// =============================================================================

class CubeMapTexGen {
public:
    // Generate texture coordinates for reflection mapping
    static void GenerateReflectionCoords(
        const float* positions,    // Array of positions
        const float* normals,      // Array of normals
        const D3DMATRIX& world,    // World matrix
        const D3DVECTOR& eyePos,   // Camera position
        float* texCoords,          // Output texture coords (u,v,w)
        uint32_t numVertices
    );

    // Generate texture coordinates for normal-based cube mapping
    static void GenerateNormalCoords(
        const float* normals,
        const D3DMATRIX& world,
        float* texCoords,
        uint32_t numVertices
    );

    // Generate texture coordinates for position-based cube mapping
    static void GeneratePositionCoords(
        const float* positions,
        const D3DMATRIX& world,
        float* texCoords,
        uint32_t numVertices
    );

    // Transform direction for cube map sampling
    // Applies texture matrix to direction
    static void TransformCubeMapDirection(
        float& u, float& v, float& w,
        const D3DMATRIX& textureMatrix
    );
};

} // namespace dx8bgfx
