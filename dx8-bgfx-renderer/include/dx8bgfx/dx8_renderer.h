#pragma once

#include "dx8_types.h"
#include "dx8_constants.h"
#include "dx8_state_manager.h"
#include "dx8_shader_key.h"

#include <bgfx/bgfx.h>
#include <memory>

namespace dx8bgfx {

// Forward declarations
class ShaderCache;
class ShaderGenerator;
class UniformManager;

// =============================================================================
// Renderer Configuration
// =============================================================================

struct RendererConfig {
    uint32_t maxShaderVariants = 5000;      // Max cached shader variants
    bool useUbershaderFallback = true;      // Use ubershader when compiling
    bool asyncShaderCompilation = true;     // Compile shaders in background
    bool enableValidation = false;          // Enable debug validation
};

// =============================================================================
// Vertex Buffer Handle
// =============================================================================

struct VertexBufferHandle {
    bgfx::VertexBufferHandle handle = BGFX_INVALID_HANDLE;
    bgfx::VertexLayout layout;
    DWORD fvf = 0;
    uint32_t vertexCount = 0;
};

// =============================================================================
// Index Buffer Handle
// =============================================================================

struct IndexBufferHandle {
    bgfx::IndexBufferHandle handle = BGFX_INVALID_HANDLE;
    bool is32Bit = false;
    uint32_t indexCount = 0;
};

// =============================================================================
// Texture Handle
// =============================================================================

struct TextureHandle {
    bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 0;
    uint8_t numMips = 0;
    bool isCube = false;
    bool is3D = false;
};

// =============================================================================
// Main Renderer Class
// =============================================================================

class Renderer {
public:
    Renderer();
    ~Renderer();

    // Initialization
    HRESULT Init(uint32_t width, uint32_t height, const RendererConfig& config = {});
    void Shutdown();

    // Frame management
    void BeginFrame();
    void EndFrame();

    // Transform management
    HRESULT SetTransform(D3DTRANSFORMSTATETYPE type, const D3DMATRIX* matrix);
    HRESULT GetTransform(D3DTRANSFORMSTATETYPE type, D3DMATRIX* matrix);

    // Light management
    HRESULT SetLight(DWORD index, const D3DLIGHT8* light);
    HRESULT GetLight(DWORD index, D3DLIGHT8* light);
    HRESULT LightEnable(DWORD index, BOOL enable);
    HRESULT GetLightEnable(DWORD index, BOOL* enable);

    // Material
    HRESULT SetMaterial(const D3DMATERIAL8* material);
    HRESULT GetMaterial(D3DMATERIAL8* material);

    // Render states
    HRESULT SetRenderState(D3DRENDERSTATETYPE state, DWORD value);
    HRESULT GetRenderState(D3DRENDERSTATETYPE state, DWORD* value);

    // Texture stage states
    HRESULT SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value);
    HRESULT GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD* value);

    // Texture binding
    HRESULT SetTexture(DWORD stage, TextureHandle* texture);

    // Viewport
    HRESULT SetViewport(const D3DVIEWPORT8* viewport);
    HRESULT GetViewport(D3DVIEWPORT8* viewport);

    // Clip planes
    HRESULT SetClipPlane(DWORD index, const float* plane);
    HRESULT GetClipPlane(DWORD index, float* plane);

    // Buffer management
    VertexBufferHandle CreateVertexBuffer(const void* data, uint32_t size, DWORD fvf);
    void DestroyVertexBuffer(VertexBufferHandle& vb);

    IndexBufferHandle CreateIndexBuffer(const void* data, uint32_t count, bool is32Bit = false);
    void DestroyIndexBuffer(IndexBufferHandle& ib);

    // Texture management
    TextureHandle CreateTexture2D(uint32_t width, uint32_t height, uint32_t mipLevels,
                                   bgfx::TextureFormat::Enum format, const void* data);
    TextureHandle CreateTextureCube(uint32_t size, uint32_t mipLevels,
                                     bgfx::TextureFormat::Enum format, const void* data);
    void DestroyTexture(TextureHandle& tex);

    // Drawing
    HRESULT SetStreamSource(UINT streamNumber, VertexBufferHandle* vb, UINT stride);
    HRESULT SetIndices(IndexBufferHandle* ib);
    HRESULT SetVertexShader(DWORD fvf);

    HRESULT DrawPrimitive(D3DPRIMITIVETYPE primitiveType, UINT startVertex, UINT primitiveCount);
    HRESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE primitiveType, UINT minVertexIndex,
                                  UINT numVertices, UINT startIndex, UINT primitiveCount);

    // Clear
    HRESULT Clear(DWORD count, const D3DRECT* rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil);

    // State reset
    void Reset();

    // Statistics
    uint32_t GetShaderVariantCount() const;
    uint32_t GetDrawCallCount() const;

private:
    // Apply current state before drawing
    void ApplyState();

    // Update uniforms
    void UpdateUniforms();

    // Get or compile shader for current state
    bgfx::ProgramHandle GetCurrentProgram();

    // Convert D3D primitive type to bgfx
    static uint64_t GetPrimitiveState(D3DPRIMITIVETYPE type);
    static uint32_t GetPrimitiveCount(D3DPRIMITIVETYPE type, uint32_t primitiveCount);

    // Build vertex layout from FVF
    static bgfx::VertexLayout BuildVertexLayout(DWORD fvf);

    // Convert blend modes
    static uint64_t GetBlendState(DWORD srcBlend, DWORD dstBlend, DWORD blendOp);
    static uint64_t GetDepthState(DWORD zEnable, DWORD zWrite, DWORD zFunc);
    static uint64_t GetCullState(DWORD cullMode);

private:
    // Configuration
    RendererConfig m_config;

    // State manager
    StateManager m_stateManager;

    // Shader management
    std::unique_ptr<ShaderCache> m_shaderCache;
    std::unique_ptr<ShaderGenerator> m_shaderGenerator;
    std::unique_ptr<UniformManager> m_uniformManager;

    // Current stream sources
    VertexBufferHandle* m_currentVB = nullptr;
    IndexBufferHandle* m_currentIB = nullptr;
    UINT m_currentStride = 0;

    // View and frame
    bgfx::ViewId m_viewId = 0;
    uint32_t m_frameNumber = 0;
    uint32_t m_drawCallCount = 0;

    // Screen dimensions
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // Initialization state
    bool m_initialized = false;
};

// =============================================================================
// Helper Functions
// =============================================================================

// Matrix operations
D3DMATRIX MatrixIdentity();
D3DMATRIX MatrixMultiply(const D3DMATRIX& a, const D3DMATRIX& b);
D3DMATRIX MatrixTranspose(const D3DMATRIX& m);
D3DMATRIX MatrixInverse(const D3DMATRIX& m);

// Vector operations
D3DVECTOR VectorNormalize(const D3DVECTOR& v);
float VectorLength(const D3DVECTOR& v);
float VectorDot(const D3DVECTOR& a, const D3DVECTOR& b);
D3DVECTOR VectorCross(const D3DVECTOR& a, const D3DVECTOR& b);
D3DVECTOR VectorTransform(const D3DVECTOR& v, const D3DMATRIX& m);

} // namespace dx8bgfx
