// =============================================================================
// DX8-BGFX Alpha Blending Example
// =============================================================================
// Demonstrates various alpha blending modes:
// - Standard alpha blending
// - Additive blending
// - Multiplicative blending
// - Alpha testing
//

#include <dx8bgfx/dx8bgfx.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <cmath>

using namespace dx8bgfx;

// Application state
static Renderer* g_renderer = nullptr;
static float g_time = 0.0f;

// Geometry
static std::vector<GeometryGenerator::Vertex> g_cubeVertices;
static std::vector<uint16_t> g_cubeIndices;
static std::vector<GeometryGenerator::Vertex> g_planeVertices;
static std::vector<uint16_t> g_planeIndices;

static bgfx::VertexBufferHandle g_cubeVB = BGFX_INVALID_HANDLE;
static bgfx::IndexBufferHandle g_cubeIB = BGFX_INVALID_HANDLE;
static bgfx::VertexBufferHandle g_planeVB = BGFX_INVALID_HANDLE;
static bgfx::IndexBufferHandle g_planeIB = BGFX_INVALID_HANDLE;

// Initialize geometry
void InitGeometry() {
    GeometryGenerator::GenerateCube(g_cubeVertices, g_cubeIndices);
    GeometryGenerator::GeneratePlane(g_planeVertices, g_planeIndices, 1);

    bgfx::VertexLayout layout = GeometryGenerator::GetGeneratedVertexLayout();

    g_cubeVB = VertexBufferUtils::CreateVertexBuffer(
        g_cubeVertices.data(),
        static_cast<uint32_t>(g_cubeVertices.size()),
        layout
    );
    g_cubeIB = IndexBufferUtils::CreateIndexBuffer16(
        g_cubeIndices.data(),
        static_cast<uint32_t>(g_cubeIndices.size())
    );

    g_planeVB = VertexBufferUtils::CreateVertexBuffer(
        g_planeVertices.data(),
        static_cast<uint32_t>(g_planeVertices.size()),
        layout
    );
    g_planeIB = IndexBufferUtils::CreateIndexBuffer16(
        g_planeIndices.data(),
        static_cast<uint32_t>(g_planeIndices.size())
    );
}

// Draw a cube with transform
void DrawCube(Renderer& renderer, const D3DMATRIX& world) {
    renderer.SetTransform(D3DTS_WORLD, &world);

    DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 | D3DFVF_DIFFUSE;
    renderer.SetFVF(fvf);

    bgfx::setVertexBuffer(0, g_cubeVB);
    bgfx::setIndexBuffer(g_cubeIB);
    renderer.DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST,
        0, 0,
        static_cast<UINT>(g_cubeVertices.size()),
        0,
        static_cast<UINT>(g_cubeIndices.size() / 3)
    );
}

// Draw floor plane
void DrawFloor(Renderer& renderer) {
    D3DMATRIX world;
    float scale = 10.0f;

    world._11 = scale; world._12 = 0;     world._13 = 0;     world._14 = 0;
    world._21 = 0;     world._22 = scale; world._23 = 0;     world._24 = 0;
    world._31 = 0;     world._32 = 0;     world._33 = scale; world._34 = 0;
    world._41 = 0;     world._42 = -1;    world._43 = 0;     world._44 = 1;

    renderer.SetTransform(D3DTS_WORLD, &world);

    DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 | D3DFVF_DIFFUSE;
    renderer.SetFVF(fvf);

    bgfx::setVertexBuffer(0, g_planeVB);
    bgfx::setIndexBuffer(g_planeIB);
    renderer.DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST,
        0, 0,
        static_cast<UINT>(g_planeVertices.size()),
        0,
        static_cast<UINT>(g_planeIndices.size() / 3)
    );
}

// Build rotation matrix
D3DMATRIX BuildCubeMatrix(float x, float y, float z, float rotY, float scale) {
    D3DMATRIX m;
    float c = std::cos(rotY);
    float s = std::sin(rotY);

    m._11 = c * scale;  m._12 = 0;     m._13 = s * scale; m._14 = 0;
    m._21 = 0;          m._22 = scale; m._23 = 0;         m._24 = 0;
    m._31 = -s * scale; m._32 = 0;     m._33 = c * scale; m._34 = 0;
    m._41 = x;          m._42 = y;     m._43 = z;         m._44 = 1;

    return m;
}

// Render frame
void RenderFrame(uint32_t width, uint32_t height) {
    Renderer& renderer = *g_renderer;

    g_time += 0.016f;

    // Clear
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                       0x404060FF, 1.0f, 0);

    // Camera setup
    D3DMATRIX view, proj;

    float eyeX = std::sin(g_time * 0.3f) * 8.0f;
    float eyeY = 5.0f;
    float eyeZ = std::cos(g_time * 0.3f) * 8.0f;

    // Look-at matrix
    float fx = -eyeX, fy = -eyeY + 1.0f, fz = -eyeZ;
    float len = std::sqrt(fx*fx + fy*fy + fz*fz);
    fx /= len; fy /= len; fz /= len;

    float ux = 0, uy = 1, uz = 0;
    float rx = uy*fz - uz*fy, ry = uz*fx - ux*fz, rz = ux*fy - uy*fx;
    len = std::sqrt(rx*rx + ry*ry + rz*rz);
    rx /= len; ry /= len; rz /= len;
    ux = fy*rz - fz*ry; uy = fz*rx - fx*rz; uz = fx*ry - fy*rx;

    view._11 = rx; view._12 = ux; view._13 = fx; view._14 = 0;
    view._21 = ry; view._22 = uy; view._23 = fy; view._24 = 0;
    view._31 = rz; view._32 = uz; view._33 = fz; view._34 = 0;
    view._41 = -(rx*eyeX + ry*eyeY + rz*eyeZ);
    view._42 = -(ux*eyeX + uy*eyeY + uz*eyeZ);
    view._43 = -(fx*eyeX + fy*eyeY + fz*eyeZ);
    view._44 = 1;

    // Perspective
    float fov = 45.0f * 3.14159f / 180.0f;
    float aspect = float(width) / float(height);
    float nearZ = 0.1f, farZ = 100.0f;
    float yScale = 1.0f / std::tan(fov * 0.5f);
    float xScale = yScale / aspect;

    proj._11 = xScale; proj._12 = 0;      proj._13 = 0;                       proj._14 = 0;
    proj._21 = 0;      proj._22 = yScale; proj._23 = 0;                       proj._24 = 0;
    proj._31 = 0;      proj._32 = 0;      proj._33 = farZ/(farZ-nearZ);       proj._34 = 1;
    proj._41 = 0;      proj._42 = 0;      proj._43 = -nearZ*farZ/(farZ-nearZ); proj._44 = 0;

    renderer.SetTransform(D3DTS_VIEW, &view);
    renderer.SetTransform(D3DTS_PROJECTION, &proj);

    // Lighting
    D3DLIGHT8 light = CreateDirectionalLight(0.5f, -1.0f, 0.3f);
    renderer.SetLight(0, &light);
    renderer.LightEnable(0, TRUE);
    renderer.SetRenderState(D3DRS_LIGHTING, TRUE);
    renderer.SetRenderState(D3DRS_AMBIENT, 0x00303030);

    D3DMATERIAL8 material;

    // ==========================================================================
    // Pass 1: Draw opaque objects (floor and solid cubes)
    // ==========================================================================

    ApplySolidRenderStates(renderer);

    // Floor
    material = CreateDefaultMaterial();
    material.Diffuse = {0.4f, 0.4f, 0.5f, 1.0f};
    renderer.SetMaterial(&material);
    DrawFloor(renderer);

    // Opaque red cube (back left)
    material.Diffuse = {0.9f, 0.2f, 0.2f, 1.0f};
    renderer.SetMaterial(&material);
    DrawCube(renderer, BuildCubeMatrix(-3.0f, 0.5f, 3.0f, g_time, 0.8f));

    // ==========================================================================
    // Pass 2: Draw alpha tested objects
    // ==========================================================================

    // Alpha test - objects with sharp cutouts
    ApplyAlphaTestStates(renderer, 128);

    // Yellow cube with alpha test (back right)
    material.Diffuse = {0.9f, 0.9f, 0.2f, 0.5f};
    renderer.SetMaterial(&material);
    DrawCube(renderer, BuildCubeMatrix(3.0f, 0.5f, 3.0f, -g_time * 0.5f, 0.8f));

    renderer.SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

    // ==========================================================================
    // Pass 3: Draw transparent objects (back to front)
    // ==========================================================================

    // Standard alpha blend - semi-transparent
    ApplyTransparentRenderStates(renderer);

    // Blue transparent cube (center back)
    material.Diffuse = {0.2f, 0.4f, 0.9f, 0.5f};
    renderer.SetMaterial(&material);
    DrawCube(renderer, BuildCubeMatrix(0.0f, 0.5f, 4.0f, g_time * 0.7f, 0.8f));

    // Green transparent cube (center)
    material.Diffuse = {0.2f, 0.9f, 0.3f, 0.4f};
    renderer.SetMaterial(&material);
    DrawCube(renderer, BuildCubeMatrix(0.0f, 0.5f, 0.0f, g_time, 0.8f));

    // ==========================================================================
    // Pass 4: Additive blending (glowing objects)
    // ==========================================================================

    ApplyAdditiveBlending(renderer);

    // Cyan additive cube (front left)
    material.Diffuse = {0.0f, 0.8f, 0.8f, 0.6f};
    renderer.SetMaterial(&material);
    DrawCube(renderer, BuildCubeMatrix(-3.0f, 0.5f + std::sin(g_time * 2.0f) * 0.3f, -2.0f, g_time * 1.5f, 0.6f));

    // ==========================================================================
    // Pass 5: Multiplicative blending (darkening)
    // ==========================================================================

    renderer.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
    renderer.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);

    // Dark purple multiply cube (front right)
    material.Diffuse = {0.6f, 0.4f, 0.8f, 1.0f};
    renderer.SetMaterial(&material);
    DrawCube(renderer, BuildCubeMatrix(3.0f, 0.5f, -2.0f, -g_time * 0.8f, 0.6f));

    // ==========================================================================
    // Display info
    // ==========================================================================

    bgfx::dbgTextClear();
    bgfx::dbgTextPrintf(0, 1, 0x0f, "Alpha Blending Demo");
    bgfx::dbgTextPrintf(0, 2, 0x0f, "Red: Opaque | Yellow: Alpha Test | Blue/Green: Alpha Blend");
    bgfx::dbgTextPrintf(0, 3, 0x0f, "Cyan: Additive | Purple: Multiplicative");

    // End frame
    renderer.EndFrame();
    bgfx::frame();
}

// Cleanup
void Cleanup() {
    if (bgfx::isValid(g_cubeVB)) bgfx::destroy(g_cubeVB);
    if (bgfx::isValid(g_cubeIB)) bgfx::destroy(g_cubeIB);
    if (bgfx::isValid(g_planeVB)) bgfx::destroy(g_planeVB);
    if (bgfx::isValid(g_planeIB)) bgfx::destroy(g_planeIB);

    delete g_renderer;
    g_renderer = nullptr;
}

// Main entry point
int main(int argc, char** argv) {
    uint32_t width = 1280;
    uint32_t height = 720;

    bgfx::Init init;
    init.type = bgfx::RendererType::Count;
    init.resolution.width = width;
    init.resolution.height = height;
    init.resolution.reset = BGFX_RESET_VSYNC;

    if (!bgfx::init(init)) {
        return 1;
    }

    bgfx::setDebug(BGFX_DEBUG_TEXT);
    bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));

    g_renderer = new Renderer();
    g_renderer->Initialize(width, height);

    InitGeometry();

    bool running = true;
    while (running) {
        RenderFrame(width, height);
        if (g_time > 30.0f) running = false;
    }

    Cleanup();
    bgfx::shutdown();

    return 0;
}
