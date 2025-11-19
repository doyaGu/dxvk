/*
 * DX8-bgfx-renderer Example: Multi-Texture Blending
 *
 * This example demonstrates:
 * - Multiple texture stages
 * - Texture blending operations (MODULATE, ADD, etc.)
 * - Texture coordinate transformation
 * - Bump mapping setup
 */

#include <dx8bgfx/dx8_renderer.h>
#include <dx8bgfx/dx8_math.h>

#include <cstdio>

using namespace dx8bgfx;

// =============================================================================
// Quad Vertex Data
// =============================================================================

struct MultiTexVertex {
    float x, y, z;
    float nx, ny, nz;
    uint32_t color;
    float u0, v0;  // Texture 0
    float u1, v1;  // Texture 1
};

static MultiTexVertex s_quadVertices[] = {
    {-2.0f, -2.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0xFFFFFFFF, 0.0f, 1.0f, 0.0f, 2.0f},
    { 2.0f, -2.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0xFFFFFFFF, 1.0f, 1.0f, 2.0f, 2.0f},
    { 2.0f,  2.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0xFFFFFFFF, 1.0f, 0.0f, 2.0f, 0.0f},
    {-2.0f,  2.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0xFFFFFFFF, 0.0f, 0.0f, 0.0f, 0.0f},
};

static uint16_t s_quadIndices[] = {
    0, 1, 2, 0, 2, 3
};

// =============================================================================
// Multi-Texture Example
// =============================================================================

class MultiTextureExample {
public:
    bool Init(Renderer& renderer, uint32_t width, uint32_t height) {
        m_renderer = &renderer;
        m_width = width;
        m_height = height;

        // Create vertex buffer with 2 texture coordinates
        DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX2;
        m_vertexBuffer = renderer.CreateVertexBuffer(
            s_quadVertices,
            sizeof(s_quadVertices),
            fvf
        );

        // Create index buffer
        m_indexBuffer = renderer.CreateIndexBuffer(
            s_quadIndices,
            6,
            false
        );

        // Create procedural textures
        CreateCheckerTexture(renderer, 0);  // Base texture
        CreateNoiseTexture(renderer, 1);    // Detail texture

        // Set up view/projection
        D3DVECTOR eye = {0.0f, 0.0f, -5.0f};
        D3DVECTOR at = {0.0f, 0.0f, 0.0f};
        D3DVECTOR up = {0.0f, 1.0f, 0.0f};
        m_viewMatrix = MatrixLookAtLH(eye, at, up);

        float fov = 60.0f * DEG_TO_RAD;
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        m_projMatrix = MatrixPerspectiveFovLH(fov, aspect, 0.1f, 100.0f);

        return true;
    }

    void Shutdown() {
        m_renderer->DestroyVertexBuffer(m_vertexBuffer);
        m_renderer->DestroyIndexBuffer(m_indexBuffer);
        m_renderer->DestroyTexture(m_texture0);
        m_renderer->DestroyTexture(m_texture1);
    }

    void SetBlendMode(int mode) {
        m_blendMode = mode;
    }

    void Render() {
        // Set transforms
        D3DMATRIX world = MatrixIdentity();
        m_renderer->SetTransform(D3DTS_WORLD, &world);
        m_renderer->SetTransform(D3DTS_VIEW, &m_viewMatrix);
        m_renderer->SetTransform(D3DTS_PROJECTION, &m_projMatrix);

        // Disable lighting for this example
        m_renderer->SetRenderState(D3DRS_LIGHTING, FALSE);

        // Set textures
        m_renderer->SetTexture(0, &m_texture0);
        m_renderer->SetTexture(1, &m_texture1);

        // Configure texture stages based on blend mode
        switch (m_blendMode) {
            case 0:  // MODULATE - Multiply textures
                // Stage 0: Select texture
                m_renderer->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
                m_renderer->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                m_renderer->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

                // Stage 1: Modulate with second texture
                m_renderer->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
                m_renderer->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                m_renderer->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
                m_renderer->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
                break;

            case 1:  // ADD - Add textures
                m_renderer->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
                m_renderer->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                m_renderer->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

                m_renderer->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_ADD);
                m_renderer->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                m_renderer->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
                m_renderer->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
                break;

            case 2:  // MODULATE2X - Multiply and brighten
                m_renderer->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
                m_renderer->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                m_renderer->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

                m_renderer->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE2X);
                m_renderer->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                m_renderer->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
                m_renderer->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
                break;

            case 3:  // ADDSIGNED - Add with bias
                m_renderer->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
                m_renderer->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                m_renderer->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

                m_renderer->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_ADDSIGNED);
                m_renderer->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                m_renderer->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
                m_renderer->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
                break;

            case 4:  // BLENDDIFFUSEALPHA - Use vertex alpha for blending
                m_renderer->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
                m_renderer->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                m_renderer->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

                m_renderer->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_BLENDDIFFUSEALPHA);
                m_renderer->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                m_renderer->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
                m_renderer->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
                break;

            case 5:  // DOTPRODUCT3 - Bump mapping style
                m_renderer->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
                m_renderer->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                m_renderer->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

                m_renderer->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DOTPRODUCT3);
                m_renderer->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                m_renderer->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
                m_renderer->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
                break;
        }

        // Alpha stages
        m_renderer->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        m_renderer->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        m_renderer->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        m_renderer->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);

        // Disable stage 2
        m_renderer->SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);

        // Draw
        m_renderer->SetStreamSource(0, &m_vertexBuffer, sizeof(MultiTexVertex));
        m_renderer->SetIndices(&m_indexBuffer);
        m_renderer->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 4, 0, 2);
    }

private:
    void CreateCheckerTexture(Renderer& renderer, int slot) {
        const int size = 64;
        uint32_t* data = new uint32_t[size * size];

        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                bool white = ((x / 8) + (y / 8)) % 2 == 0;
                data[y * size + x] = white ? 0xFFFFFFFF : 0xFF404040;
            }
        }

        if (slot == 0) {
            m_texture0 = renderer.CreateTexture2D(size, size, 1,
                bgfx::TextureFormat::BGRA8, data);
        } else {
            m_texture1 = renderer.CreateTexture2D(size, size, 1,
                bgfx::TextureFormat::BGRA8, data);
        }

        delete[] data;
    }

    void CreateNoiseTexture(Renderer& renderer, int slot) {
        const int size = 64;
        uint32_t* data = new uint32_t[size * size];

        // Simple pseudo-random noise
        unsigned int seed = 12345;
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                seed = seed * 1103515245 + 12345;
                uint8_t val = (seed >> 16) & 0xFF;
                data[y * size + x] = 0xFF000000 | (val << 16) | (val << 8) | val;
            }
        }

        if (slot == 0) {
            m_texture0 = renderer.CreateTexture2D(size, size, 1,
                bgfx::TextureFormat::BGRA8, data);
        } else {
            m_texture1 = renderer.CreateTexture2D(size, size, 1,
                bgfx::TextureFormat::BGRA8, data);
        }

        delete[] data;
    }

private:
    Renderer* m_renderer = nullptr;
    VertexBufferHandle m_vertexBuffer;
    IndexBufferHandle m_indexBuffer;
    TextureHandle m_texture0;
    TextureHandle m_texture1;

    D3DMATRIX m_viewMatrix;
    D3DMATRIX m_projMatrix;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    int m_blendMode = 0;
};

// =============================================================================
// Usage Information
// =============================================================================

void PrintUsage() {
    printf("DX8-bgfx-renderer Multi-Texture Example\n");
    printf("========================================\n");
    printf("\n");
    printf("Blend Modes:\n");
    printf("  0 - MODULATE:      tex0 * tex1\n");
    printf("  1 - ADD:           tex0 + tex1\n");
    printf("  2 - MODULATE2X:    tex0 * tex1 * 2\n");
    printf("  3 - ADDSIGNED:     tex0 + tex1 - 0.5\n");
    printf("  4 - BLENDALPHA:    lerp(tex0, tex1, vertex.a)\n");
    printf("  5 - DOTPRODUCT3:   dot(tex0 - 0.5, tex1 - 0.5)\n");
    printf("\n");
    printf("This demonstrates all major texture blending operations\n");
    printf("available in the DX8 fixed-function pipeline.\n");
}

#ifndef DX8BGFX_NO_MAIN

int main(int argc, char** argv) {
    PrintUsage();
    return 0;
}

#endif
