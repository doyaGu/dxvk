#include "dx8bgfx/dx8_renderer.h"
#include "dx8bgfx/dx8_shader_cache.h"
#include "dx8bgfx/dx8_shader_generator.h"
#include "dx8bgfx/dx8_uniform_manager.h"

#include <cmath>
#include <cstring>

namespace dx8bgfx {

// =============================================================================
// Renderer Implementation
// =============================================================================

Renderer::Renderer() = default;

Renderer::~Renderer() {
    Shutdown();
}

HRESULT Renderer::Init(uint32_t width, uint32_t height, const RendererConfig& config) {
    if (m_initialized) return D3D_OK;

    m_config = config;
    m_width = width;
    m_height = height;

    // Initialize components
    m_shaderCache = std::make_unique<ShaderCache>();
    m_shaderGenerator = std::make_unique<ShaderGenerator>();
    m_uniformManager = std::make_unique<UniformManager>();

    m_shaderCache->Init(config.maxShaderVariants, config.asyncShaderCompilation);
    m_uniformManager->Init();

    // Set default viewport
    D3DVIEWPORT8 viewport = {0, 0, width, height, 0.0f, 1.0f};
    m_stateManager.SetViewport(viewport);

    m_initialized = true;
    return D3D_OK;
}

void Renderer::Shutdown() {
    if (!m_initialized) return;

    m_uniformManager->Shutdown();
    m_shaderCache->Shutdown();

    m_uniformManager.reset();
    m_shaderGenerator.reset();
    m_shaderCache.reset();

    m_initialized = false;
}

void Renderer::BeginFrame() {
    m_drawCallCount = 0;
}

void Renderer::EndFrame() {
    bgfx::frame();
    m_frameNumber++;
    m_shaderCache->OnFrame(m_frameNumber);
}

// =============================================================================
// Transform Management
// =============================================================================

HRESULT Renderer::SetTransform(D3DTRANSFORMSTATETYPE type, const D3DMATRIX* matrix) {
    return m_stateManager.SetTransform(type, matrix);
}

HRESULT Renderer::GetTransform(D3DTRANSFORMSTATETYPE type, D3DMATRIX* matrix) {
    return m_stateManager.GetTransform(type, matrix);
}

// =============================================================================
// Light Management
// =============================================================================

HRESULT Renderer::SetLight(DWORD index, const D3DLIGHT8* light) {
    return m_stateManager.SetLight(index, light);
}

HRESULT Renderer::GetLight(DWORD index, D3DLIGHT8* light) {
    return m_stateManager.GetLight(index, light);
}

HRESULT Renderer::LightEnable(DWORD index, BOOL enable) {
    return m_stateManager.LightEnable(index, enable);
}

HRESULT Renderer::GetLightEnable(DWORD index, BOOL* enable) {
    return m_stateManager.GetLightEnable(index, enable);
}

// =============================================================================
// Material
// =============================================================================

HRESULT Renderer::SetMaterial(const D3DMATERIAL8* material) {
    return m_stateManager.SetMaterial(material);
}

HRESULT Renderer::GetMaterial(D3DMATERIAL8* material) {
    return m_stateManager.GetMaterial(material);
}

// =============================================================================
// Render States
// =============================================================================

HRESULT Renderer::SetRenderState(D3DRENDERSTATETYPE state, DWORD value) {
    return m_stateManager.SetRenderState(state, value);
}

HRESULT Renderer::GetRenderState(D3DRENDERSTATETYPE state, DWORD* value) {
    return m_stateManager.GetRenderState(state, value);
}

// =============================================================================
// Texture Stage States
// =============================================================================

HRESULT Renderer::SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value) {
    return m_stateManager.SetTextureStageState(stage, type, value);
}

HRESULT Renderer::GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD* value) {
    return m_stateManager.GetTextureStageState(stage, type, value);
}

// =============================================================================
// Texture Binding
// =============================================================================

HRESULT Renderer::SetTexture(DWORD stage, TextureHandle* texture) {
    if (texture && bgfx::isValid(texture->handle)) {
        m_stateManager.SetTextureHandle(stage, texture->handle.idx);
    } else {
        m_stateManager.SetTextureHandle(stage, UINT16_MAX);
    }
    return D3D_OK;
}

// =============================================================================
// Viewport
// =============================================================================

HRESULT Renderer::SetViewport(const D3DVIEWPORT8* viewport) {
    if (!viewport) return D3DERR_INVALIDCALL;
    m_stateManager.SetViewport(*viewport);

    bgfx::setViewRect(m_viewId, viewport->X, viewport->Y, viewport->Width, viewport->Height);
    return D3D_OK;
}

HRESULT Renderer::GetViewport(D3DVIEWPORT8* viewport) {
    if (!viewport) return D3DERR_INVALIDCALL;
    *viewport = m_stateManager.GetViewport();
    return D3D_OK;
}

// =============================================================================
// Clip Planes
// =============================================================================

HRESULT Renderer::SetClipPlane(DWORD index, const float* plane) {
    return m_stateManager.SetClipPlane(index, plane);
}

HRESULT Renderer::GetClipPlane(DWORD index, float* plane) {
    return m_stateManager.GetClipPlane(index, plane);
}

// =============================================================================
// Buffer Management
// =============================================================================

VertexBufferHandle Renderer::CreateVertexBuffer(const void* data, uint32_t size, DWORD fvf) {
    VertexBufferHandle vb;
    vb.fvf = fvf;
    vb.layout = BuildVertexLayout(fvf);

    // Calculate vertex count
    uint32_t stride = vb.layout.getStride();
    vb.vertexCount = size / stride;

    // Create bgfx buffer
    const bgfx::Memory* mem = bgfx::copy(data, size);
    vb.handle = bgfx::createVertexBuffer(mem, vb.layout);

    return vb;
}

void Renderer::DestroyVertexBuffer(VertexBufferHandle& vb) {
    if (bgfx::isValid(vb.handle)) {
        bgfx::destroy(vb.handle);
        vb.handle = BGFX_INVALID_HANDLE;
    }
}

IndexBufferHandle Renderer::CreateIndexBuffer(const void* data, uint32_t count, bool is32Bit) {
    IndexBufferHandle ib;
    ib.is32Bit = is32Bit;
    ib.indexCount = count;

    uint32_t size = count * (is32Bit ? 4 : 2);
    const bgfx::Memory* mem = bgfx::copy(data, size);

    uint16_t flags = is32Bit ? BGFX_BUFFER_INDEX32 : 0;
    ib.handle = bgfx::createIndexBuffer(mem, flags);

    return ib;
}

void Renderer::DestroyIndexBuffer(IndexBufferHandle& ib) {
    if (bgfx::isValid(ib.handle)) {
        bgfx::destroy(ib.handle);
        ib.handle = BGFX_INVALID_HANDLE;
    }
}

// =============================================================================
// Texture Management
// =============================================================================

TextureHandle Renderer::CreateTexture2D(uint32_t width, uint32_t height, uint32_t mipLevels,
                                         bgfx::TextureFormat::Enum format, const void* data) {
    TextureHandle tex;
    tex.width = width;
    tex.height = height;
    tex.depth = 1;
    tex.numMips = mipLevels;
    tex.isCube = false;
    tex.is3D = false;

    // Calculate data size (simplified - assumes single mip, uncompressed)
    uint32_t bpp = 4;  // Assume 32-bit format
    uint32_t size = width * height * bpp;

    const bgfx::Memory* mem = data ? bgfx::copy(data, size) : nullptr;
    tex.handle = bgfx::createTexture2D(width, height, mipLevels > 1, 1, format, 0, mem);

    return tex;
}

TextureHandle Renderer::CreateTextureCube(uint32_t size, uint32_t mipLevels,
                                           bgfx::TextureFormat::Enum format, const void* data) {
    TextureHandle tex;
    tex.width = size;
    tex.height = size;
    tex.depth = 1;
    tex.numMips = mipLevels;
    tex.isCube = true;
    tex.is3D = false;

    tex.handle = bgfx::createTextureCube(size, mipLevels > 1, 1, format, 0, nullptr);

    return tex;
}

void Renderer::DestroyTexture(TextureHandle& tex) {
    if (bgfx::isValid(tex.handle)) {
        bgfx::destroy(tex.handle);
        tex.handle = BGFX_INVALID_HANDLE;
    }
}

// =============================================================================
// Stream Sources
// =============================================================================

HRESULT Renderer::SetStreamSource(UINT streamNumber, VertexBufferHandle* vb, UINT stride) {
    if (streamNumber == 0) {
        m_currentVB = vb;
        m_currentStride = stride;
        if (vb) {
            m_stateManager.SetFVF(vb->fvf);
        }
    }
    return D3D_OK;
}

HRESULT Renderer::SetIndices(IndexBufferHandle* ib) {
    m_currentIB = ib;
    return D3D_OK;
}

HRESULT Renderer::SetVertexShader(DWORD fvf) {
    m_stateManager.SetFVF(fvf);
    return D3D_OK;
}

// =============================================================================
// Drawing
// =============================================================================

HRESULT Renderer::DrawPrimitive(D3DPRIMITIVETYPE primitiveType, UINT startVertex, UINT primitiveCount) {
    if (!m_currentVB || !bgfx::isValid(m_currentVB->handle)) {
        return D3DERR_INVALIDCALL;
    }

    ApplyState();

    // Set vertex buffer
    uint32_t numVertices = GetPrimitiveCount(primitiveType, primitiveCount);
    bgfx::setVertexBuffer(0, m_currentVB->handle, startVertex, numVertices);

    // Get program
    bgfx::ProgramHandle program = GetCurrentProgram();

    // Set state
    uint64_t state = GetPrimitiveState(primitiveType);
    state |= GetDepthState(
        m_stateManager.IsLightingEnabled() ? D3DZB_TRUE : D3DZB_FALSE,
        TRUE,
        D3DCMP_LESSEQUAL
    );

    DWORD cullMode = D3DCULL_CCW;
    m_stateManager.GetRenderState(D3DRS_CULLMODE, &cullMode);
    state |= GetCullState(cullMode);

    if (m_stateManager.IsAlphaBlendEnabled()) {
        DWORD srcBlend = D3DBLEND_ONE, dstBlend = D3DBLEND_ZERO, blendOp = D3DBLENDOP_ADD;
        m_stateManager.GetRenderState(D3DRS_SRCBLEND, &srcBlend);
        m_stateManager.GetRenderState(D3DRS_DESTBLEND, &dstBlend);
        m_stateManager.GetRenderState(D3DRS_BLENDOP, &blendOp);
        state |= GetBlendState(srcBlend, dstBlend, blendOp);
    }

    bgfx::setState(state);

    // Submit
    bgfx::submit(m_viewId, program);
    m_drawCallCount++;

    return D3D_OK;
}

HRESULT Renderer::DrawIndexedPrimitive(D3DPRIMITIVETYPE primitiveType, UINT minVertexIndex,
                                        UINT numVertices, UINT startIndex, UINT primitiveCount) {
    if (!m_currentVB || !bgfx::isValid(m_currentVB->handle) ||
        !m_currentIB || !bgfx::isValid(m_currentIB->handle)) {
        return D3DERR_INVALIDCALL;
    }

    ApplyState();

    // Set buffers
    bgfx::setVertexBuffer(0, m_currentVB->handle, minVertexIndex, numVertices);

    uint32_t numIndices = GetPrimitiveCount(primitiveType, primitiveCount);
    if (primitiveType == D3DPT_TRIANGLELIST) {
        numIndices = primitiveCount * 3;
    } else if (primitiveType == D3DPT_TRIANGLESTRIP || primitiveType == D3DPT_TRIANGLEFAN) {
        numIndices = primitiveCount + 2;
    }
    bgfx::setIndexBuffer(m_currentIB->handle, startIndex, numIndices);

    // Get program
    bgfx::ProgramHandle program = GetCurrentProgram();

    // Set state (same as DrawPrimitive)
    uint64_t state = GetPrimitiveState(primitiveType);
    state |= BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z;
    state |= BGFX_STATE_DEPTH_TEST_LESS;

    DWORD cullMode = D3DCULL_CCW;
    m_stateManager.GetRenderState(D3DRS_CULLMODE, &cullMode);
    state |= GetCullState(cullMode);

    if (m_stateManager.IsAlphaBlendEnabled()) {
        DWORD srcBlend = D3DBLEND_ONE, dstBlend = D3DBLEND_ZERO, blendOp = D3DBLENDOP_ADD;
        m_stateManager.GetRenderState(D3DRS_SRCBLEND, &srcBlend);
        m_stateManager.GetRenderState(D3DRS_DESTBLEND, &dstBlend);
        m_stateManager.GetRenderState(D3DRS_BLENDOP, &blendOp);
        state |= GetBlendState(srcBlend, dstBlend, blendOp);
    }

    bgfx::setState(state);

    // Submit
    bgfx::submit(m_viewId, program);
    m_drawCallCount++;

    return D3D_OK;
}

// =============================================================================
// Clear
// =============================================================================

HRESULT Renderer::Clear(DWORD count, const D3DRECT* rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil) {
    uint16_t clearFlags = 0;

    if (flags & 1) {  // D3DCLEAR_TARGET
        clearFlags |= BGFX_CLEAR_COLOR;
    }
    if (flags & 2) {  // D3DCLEAR_ZBUFFER
        clearFlags |= BGFX_CLEAR_DEPTH;
    }
    if (flags & 4) {  // D3DCLEAR_STENCIL
        clearFlags |= BGFX_CLEAR_STENCIL;
    }

    bgfx::setViewClear(m_viewId, clearFlags, color, z, stencil);

    return D3D_OK;
}

// =============================================================================
// Reset
// =============================================================================

void Renderer::Reset() {
    m_stateManager.Reset();
}

// =============================================================================
// Statistics
// =============================================================================

uint32_t Renderer::GetShaderVariantCount() const {
    return m_shaderCache->GetCachedProgramCount();
}

uint32_t Renderer::GetDrawCallCount() const {
    return m_drawCallCount;
}

// =============================================================================
// Private Methods
// =============================================================================

void Renderer::ApplyState() {
    // Update uniforms
    m_uniformManager->UpdateUniforms(m_stateManager);

    // Set textures
    for (DWORD i = 0; i < 8; i++) {
        uint16_t texHandle = m_stateManager.GetTextureHandle(i);
        if (texHandle != UINT16_MAX) {
            bgfx::TextureHandle th = {texHandle};
            m_uniformManager->SetTexture(i, th);
        }
    }

    m_stateManager.ClearDirty();
}

void Renderer::UpdateUniforms() {
    m_uniformManager->UpdateUniforms(m_stateManager);
}

bgfx::ProgramHandle Renderer::GetCurrentProgram() {
    ShaderKey key = m_stateManager.BuildShaderKey();
    return m_shaderCache->GetProgram(key.VS, key.FS);
}

uint64_t Renderer::GetPrimitiveState(D3DPRIMITIVETYPE type) {
    switch (type) {
        case D3DPT_POINTLIST:
            return BGFX_STATE_PT_POINTS;
        case D3DPT_LINELIST:
            return BGFX_STATE_PT_LINES;
        case D3DPT_LINESTRIP:
            return BGFX_STATE_PT_LINESTRIP;
        case D3DPT_TRIANGLELIST:
        case D3DPT_TRIANGLESTRIP:
        case D3DPT_TRIANGLEFAN:
        default:
            return BGFX_STATE_PT_TRISTRIP;  // Default to tristrip
    }
}

uint32_t Renderer::GetPrimitiveCount(D3DPRIMITIVETYPE type, uint32_t primitiveCount) {
    switch (type) {
        case D3DPT_POINTLIST:
            return primitiveCount;
        case D3DPT_LINELIST:
            return primitiveCount * 2;
        case D3DPT_LINESTRIP:
            return primitiveCount + 1;
        case D3DPT_TRIANGLELIST:
            return primitiveCount * 3;
        case D3DPT_TRIANGLESTRIP:
        case D3DPT_TRIANGLEFAN:
            return primitiveCount + 2;
        default:
            return primitiveCount;
    }
}

bgfx::VertexLayout Renderer::BuildVertexLayout(DWORD fvf) {
    bgfx::VertexLayout layout;
    layout.begin();

    // Position
    DWORD posType = fvf & D3DFVF_POSITION_MASK;
    if (posType == D3DFVF_XYZ || posType >= D3DFVF_XYZB1) {
        layout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
    } else if (posType == D3DFVF_XYZRHW) {
        layout.add(bgfx::Attrib::Position, 4, bgfx::AttribType::Float);
    }

    // Blend weights
    UINT blendCount = GetBlendWeightCount(fvf);
    if (blendCount > 0) {
        layout.add(bgfx::Attrib::Weight, blendCount, bgfx::AttribType::Float);
        if (fvf & D3DFVF_LASTBETA_UBYTE4) {
            layout.add(bgfx::Attrib::Indices, 4, bgfx::AttribType::Uint8);
        }
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
    UINT texCount = GetTexCoordCount(fvf);
    for (UINT i = 0; i < texCount; i++) {
        bgfx::Attrib::Enum attrib = static_cast<bgfx::Attrib::Enum>(bgfx::Attrib::TexCoord0 + i);
        layout.add(attrib, 2, bgfx::AttribType::Float);
    }

    layout.end();
    return layout;
}

uint64_t Renderer::GetBlendState(DWORD srcBlend, DWORD dstBlend, DWORD blendOp) {
    auto convertBlend = [](DWORD blend) -> uint64_t {
        switch (blend) {
            case D3DBLEND_ZERO:         return BGFX_STATE_BLEND_ZERO;
            case D3DBLEND_ONE:          return BGFX_STATE_BLEND_ONE;
            case D3DBLEND_SRCCOLOR:     return BGFX_STATE_BLEND_SRC_COLOR;
            case D3DBLEND_INVSRCCOLOR:  return BGFX_STATE_BLEND_INV_SRC_COLOR;
            case D3DBLEND_SRCALPHA:     return BGFX_STATE_BLEND_SRC_ALPHA;
            case D3DBLEND_INVSRCALPHA:  return BGFX_STATE_BLEND_INV_SRC_ALPHA;
            case D3DBLEND_DESTALPHA:    return BGFX_STATE_BLEND_DST_ALPHA;
            case D3DBLEND_INVDESTALPHA: return BGFX_STATE_BLEND_INV_DST_ALPHA;
            case D3DBLEND_DESTCOLOR:    return BGFX_STATE_BLEND_DST_COLOR;
            case D3DBLEND_INVDESTCOLOR: return BGFX_STATE_BLEND_INV_DST_COLOR;
            case D3DBLEND_SRCALPHASAT:  return BGFX_STATE_BLEND_SRC_ALPHA_SAT;
            default:                    return BGFX_STATE_BLEND_ONE;
        }
    };

    return BGFX_STATE_BLEND_FUNC(convertBlend(srcBlend), convertBlend(dstBlend));
}

uint64_t Renderer::GetDepthState(DWORD zEnable, DWORD zWrite, DWORD zFunc) {
    uint64_t state = 0;

    if (zWrite) {
        state |= BGFX_STATE_WRITE_Z;
    }

    if (zEnable) {
        switch (zFunc) {
            case D3DCMP_NEVER:        state |= BGFX_STATE_DEPTH_TEST_NEVER; break;
            case D3DCMP_LESS:         state |= BGFX_STATE_DEPTH_TEST_LESS; break;
            case D3DCMP_EQUAL:        state |= BGFX_STATE_DEPTH_TEST_EQUAL; break;
            case D3DCMP_LESSEQUAL:    state |= BGFX_STATE_DEPTH_TEST_LEQUAL; break;
            case D3DCMP_GREATER:      state |= BGFX_STATE_DEPTH_TEST_GREATER; break;
            case D3DCMP_NOTEQUAL:     state |= BGFX_STATE_DEPTH_TEST_NOTEQUAL; break;
            case D3DCMP_GREATEREQUAL: state |= BGFX_STATE_DEPTH_TEST_GEQUAL; break;
            case D3DCMP_ALWAYS:       state |= BGFX_STATE_DEPTH_TEST_ALWAYS; break;
        }
    }

    return state;
}

uint64_t Renderer::GetCullState(DWORD cullMode) {
    switch (cullMode) {
        case D3DCULL_NONE: return 0;
        case D3DCULL_CW:   return BGFX_STATE_CULL_CW;
        case D3DCULL_CCW:  return BGFX_STATE_CULL_CCW;
        default:           return BGFX_STATE_CULL_CCW;
    }
}

// =============================================================================
// Matrix/Vector Helper Functions
// =============================================================================

D3DMATRIX MatrixIdentity() {
    D3DMATRIX m;
    std::memset(&m, 0, sizeof(m));
    m._11 = m._22 = m._33 = m._44 = 1.0f;
    return m;
}

D3DMATRIX MatrixMultiply(const D3DMATRIX& a, const D3DMATRIX& b) {
    D3DMATRIX result;
    std::memset(&result, 0, sizeof(result));

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }

    return result;
}

D3DMATRIX MatrixTranspose(const D3DMATRIX& m) {
    D3DMATRIX result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.m[i][j] = m.m[j][i];
        }
    }
    return result;
}

D3DMATRIX MatrixInverse(const D3DMATRIX& m) {
    // Simplified - for proper inverse, use a full implementation
    // This is just a placeholder that returns identity
    return MatrixIdentity();
}

D3DVECTOR VectorNormalize(const D3DVECTOR& v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 0.0001f) return {0.0f, 0.0f, 0.0f};
    return {v.x / len, v.y / len, v.z / len};
}

float VectorLength(const D3DVECTOR& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

float VectorDot(const D3DVECTOR& a, const D3DVECTOR& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

D3DVECTOR VectorCross(const D3DVECTOR& a, const D3DVECTOR& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

D3DVECTOR VectorTransform(const D3DVECTOR& v, const D3DMATRIX& m) {
    return {
        v.x * m._11 + v.y * m._21 + v.z * m._31 + m._41,
        v.x * m._12 + v.y * m._22 + v.z * m._32 + m._42,
        v.x * m._13 + v.y * m._23 + v.z * m._33 + m._43
    };
}

} // namespace dx8bgfx
